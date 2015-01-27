/* find -- search for files in a directory hierarchy (fts version)
   Copyright (C) 1990, 91, 92, 93, 94, 2000, 2003, 2004, 2005, 2006,
                 2007, 2008 Free Software Foundation, Inc.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

/* This file was written by James Youngman, based on find.c.
   
   GNU find was written by Eric Decker <cire@cisco.com>,
   with enhancements by David MacKenzie <djm@gnu.org>,
   Jay Plett <jay@silence.princeton.nj.us>,
   and Tim Wood <axolotl!tim@toad.com>.
   The idea for -print0 and xargs -0 came from
   Dan Bernstein <brnstnd@kramden.acf.nyu.edu>.  
*/


#include <config.h>
#include "defs.h"


#define USE_SAFE_CHDIR 1
#undef  STAT_MOUNTPOINTS


#include <errno.h>
#include <assert.h>

#include <fcntl.h>
#include <sys/stat.h>

#include <unistd.h>

#include "xalloc.h"
#include "closeout.h"
#include <modetype.h>
#include "quotearg.h"
#include "quote.h"
#include "fts_.h"
#include "openat.h"
#include "save-cwd.h"
#include "xgetcwd.h"
#include "error.h"
#include "dircallback.h"

#ifdef HAVE_LOCALE_H
#include <locale.h>
#endif

#if ENABLE_NLS
# include <libintl.h>
# define _(Text) gettext (Text)
#else
# define _(Text) Text
#define textdomain(Domain)
#define bindtextdomain(Package, Directory)
#endif
#ifdef gettext_noop
# define N_(String) gettext_noop (String)
#else
/* See locate.c for explanation as to why not use (String) */
# define N_(String) String
#endif


static void set_close_on_exec(int fd)
{
#if defined F_GETFD && defined FD_CLOEXEC
  int flags;
  flags = fcntl(fd, F_GETFD);
  if (flags >= 0)
    {
      flags |= FD_CLOEXEC;
      fcntl(fd, F_SETFD, flags);
    }
#endif
}



/* FTS_TIGHT_CYCLE_CHECK tries to work around Savannah bug #17877
 * (but actually using it doesn't fix the bug).
 */
static int ftsoptions = FTS_NOSTAT|FTS_TIGHT_CYCLE_CHECK;

static int prev_depth = INT_MIN; /* fts_level can be < 0 */
static int curr_fd = -1;

int get_current_dirfd(void)
{
  if (ftsoptions & FTS_CWDFD)
    {
      assert (curr_fd != -1);
      assert ( (AT_FDCWD == curr_fd) || (curr_fd >= 0) );
      
      if (AT_FDCWD == curr_fd)
	return starting_desc;
      else
	return curr_fd;
    }
  else
    {
      return AT_FDCWD;
    }
}

static void left_dir(void)
{
  if (ftsoptions & FTS_CWDFD)
    {
      if (curr_fd >= 0)
	{
	  close(curr_fd);
	  curr_fd = -1;
	}
    }
  else
    {
      /* do nothing. */
    }
}

/*
 * Signal that we are now inside a directory pointed to by dir_fd.
 * The caller can't tell if this is the first time this happens, so 
 * we have to be careful not to call dup() more than once 
 */
static void inside_dir(int dir_fd)
{
  if (ftsoptions & FTS_CWDFD)
    {
      assert (dir_fd == AT_FDCWD || dir_fd >= 0);
      
      state.cwd_dir_fd = dir_fd;
      if (curr_fd < 0)
	{
	  if (AT_FDCWD == dir_fd)
	    {
	      curr_fd = AT_FDCWD;
	    }
	  else if (dir_fd >= 0)
	    {
	      curr_fd = dup(dir_fd);
	      set_close_on_exec(curr_fd);
	    }
	  else 
	    {
	      /* curr_fd is invalid, but dir_fd is also invalid.
	       * This should not have happened.
	       */
	      assert (curr_fd >= 0 || dir_fd >= 0);
	    }
	}
    }
  else
    {
      /* FTS_CWDFD is not in use.  We can always assume that 
       * AT_FDCWD refers to the directory we are currentl searching.
       *
       * Therefore there is nothing to do.
       */
    }
}



#ifdef STAT_MOUNTPOINTS
static void init_mounted_dev_list(void);
#endif

/* We have encountered an error which should affect the exit status.
 * This is normally used to change the exit status from 0 to 1.
 * However, if the exit status is already 2 for example, we don't want to 
 * reduce it to 1.
 */
static void
error_severity(int level)
{
  if (state.exit_status < level)
    state.exit_status = level;
}


#define STRINGIFY(X) #X
#define HANDLECASE(N) case N: return #N;

static char *
get_fts_info_name(int info)
{
  static char buf[10];
  switch (info)
    {
      HANDLECASE(FTS_D);
      HANDLECASE(FTS_DC);
      HANDLECASE(FTS_DEFAULT);
      HANDLECASE(FTS_DNR);
      HANDLECASE(FTS_DOT);
      HANDLECASE(FTS_DP);
      HANDLECASE(FTS_ERR);
      HANDLECASE(FTS_F);
      HANDLECASE(FTS_INIT);
      HANDLECASE(FTS_NS);
      HANDLECASE(FTS_NSOK);
      HANDLECASE(FTS_SL);
      HANDLECASE(FTS_SLNONE);
      HANDLECASE(FTS_W);
    default:
      sprintf(buf, "[%d]", info);
      return buf;
    }
}

static void
visit(FTS *p, FTSENT *ent, struct stat *pstat)
{
  struct predicate *eval_tree;
  
  state.curdepth = ent->fts_level;
  state.have_stat = (ent->fts_info != FTS_NS) && (ent->fts_info != FTS_NSOK);
  state.rel_pathname = ent->fts_accpath;
  state.cwd_dir_fd   = p->fts_cwd_fd;

  /* Apply the predicates to this path. */
  eval_tree = get_eval_tree();
  apply_predicate(ent->fts_path, pstat, eval_tree);

  /* Deal with any side effects of applying the predicates. */
  if (state.stop_at_current_level)
    {
      fts_set(p, ent, FTS_SKIP);
    }
}

static const char*
partial_quotearg_n(int n, char *s, size_t len, enum quoting_style style)
{
  if (0 == len)
    {
      return quotearg_n_style(n, style, "");
    }
  else
    {
      char saved;
      const char *result;
      
      saved = s[len];
      s[len] = 0;
      result = quotearg_n_style(n, style, s);
      s[len] = saved;
      return result;
    }
}


/* We've detected a file system loop.   This is caused by one of 
 * two things:
 *
 * 1. Option -L is in effect and we've hit a symbolic link that 
 *    points to an ancestor.  This is harmless.  We won't traverse the 
 *    symbolic link.
 *
 * 2. We have hit a real cycle in the directory hierarchy.  In this 
 *    case, we issue a diagnostic message (POSIX requires this) and we
 *    skip that directory entry.
 */
static void
issue_loop_warning(FTSENT * ent)
{
  if (S_ISLNK(ent->fts_statp->st_mode))
    {
      error(0, 0,
	    _("Symbolic link %s is part of a loop in the directory hierarchy; we have already visited the directory to which it points."),
	    safely_quote_err_filename(0, ent->fts_path));
    }
  else
    {
      /* We have found an infinite loop.  POSIX requires us to
       * issue a diagnostic.  Usually we won't get to here
       * because when the leaf optimisation is on, it will cause
       * the subdirectory to be skipped.  If /a/b/c/d is a hard
       * link to /a/b, then the link count of /a/b/c is 2,
       * because the ".." entry of /a/b/c/d points to /a, not
       * to /a/b/c.
       */
      error(0, 0,
	    _("File system loop detected; "
	      "%s is part of the same file system loop as %s."),
	    safely_quote_err_filename(0, ent->fts_path),
	    partial_quotearg_n(1,
			       ent->fts_cycle->fts_path,
			       ent->fts_cycle->fts_pathlen,
			       options.err_quoting_style));
    }
}

/* 
 * Return true if NAME corresponds to a file which forms part of a 
 * symbolic link loop.  The command 
 *      rm -f a b; ln -s a b; ln -s b a 
 * produces such a loop.
 */
static boolean 
symlink_loop(const char *name)
{
  struct stat stbuf;
  int rv;
  if (following_links())
    rv = stat(name, &stbuf);
  else
    rv = lstat(name, &stbuf);
  return (0 != rv) && (ELOOP == errno);
}

  
static int
complete_execdirs_cb(void *context)
{
  (void) context;
  /* By the tme this callback is called, the current directory is correct. */
  complete_pending_execdirs(AT_FDCWD);
  return 0;
}

static void
show_outstanding_execdirs(FILE *fp)
{
  if (options.debug_options & DebugExec)
    {
      int seen=0;
      struct predicate *p;
      p = get_eval_tree();
      fprintf(fp, "Outstanding execdirs:");

      while (p)
	{
	  const char *pfx;
	  
	  if (pred_is(p, pred_execdir))
	    pfx = "-execdir";
	  else if (pred_is(p, pred_okdir))
	    pfx = "-okdir";
	  else
	    pfx = NULL;
	  if (pfx)
	    {
	      int i;
	      const struct exec_val *execp = &p->args.exec_vec;
	      ++seen;
	      
	      fprintf(fp, "%s ", pfx);
	      if (execp->multiple)
		fprintf(fp, "multiple ");
	      fprintf(fp, "%d args: ", execp->state.cmd_argc);
	      for (i=0; i<execp->state.cmd_argc; ++i)
		{
		  fprintf(fp, "%s ", execp->state.cmd_argv[i]);
		}
	      fprintf(fp, "\n");
	    }
	  p = p->pred_next;
	}
      if (!seen)
	fprintf(fp, " none\n");
    }
  else
    {
      /* No debug output is wanted. */
    }
}




static void
consider_visiting(FTS *p, FTSENT *ent)
{
  struct stat statbuf;
  mode_t mode;
  int ignore, isdir;
  
  if (options.debug_options & DebugSearch)
    fprintf(stderr,
	    "consider_visiting: fts_info=%-6s, fts_level=%2d, prev_depth=%d "
            "fts_path=%s, fts_accpath=%s\n",
	    get_fts_info_name(ent->fts_info),
            (int)ent->fts_level, prev_depth,
	    quotearg_n_style(0, options.err_quoting_style, ent->fts_path),
	    quotearg_n_style(1, options.err_quoting_style, ent->fts_accpath));
  
  if (ent->fts_info == FTS_DP)
    {
      left_dir();
    }
  else if (ent->fts_level > prev_depth || ent->fts_level==0)
    {
      left_dir();
    }
  inside_dir(p->fts_cwd_fd);
  prev_depth = ent->fts_level;

  
  /* Cope with various error conditions. */
  if (ent->fts_info == FTS_ERR
      || ent->fts_info == FTS_DNR)
    {
      error(0, ent->fts_errno, "%s",
	    safely_quote_err_filename(0, ent->fts_path));
      error_severity(1);
      return;
    }
  else if (ent->fts_info == FTS_DC)
    {
      issue_loop_warning(ent);
      error_severity(1);
      return;
    }
  else if (ent->fts_info == FTS_SLNONE)
    {
      /* fts_read() claims that ent->fts_accpath is a broken symbolic
       * link.  That would be fine, but if this is part of a symbolic
       * link loop, we diagnose the problem and also ensure that the
       * eventual return value is nonzero.   Note that while the path 
       * we stat is local (fts_accpath), we print the full path name 
       * of the file (fts_path) in the error message.
       */
      if (symlink_loop(ent->fts_accpath))
	{
	  error(0, ELOOP, "%s", safely_quote_err_filename(0, ent->fts_path));
	  error_severity(1);
	  return;
	}
    }
  else if (ent->fts_info == FTS_NS)
    {
      if (ent->fts_level == 0)
	{
	  /* e.g., nonexistent starting point */
	  error(0, ent->fts_errno, "%s",
		safely_quote_err_filename(0, ent->fts_path));
	  error_severity(1);	/* remember problem */
	  return;
	}
      else
	{
	  /* The following if statement fixes Savannah bug #19605
	   * (failure to diagnose a symbolic link loop)
	   */
	  if (symlink_loop(ent->fts_accpath))
	    {
	      error(0, ELOOP, "%s",
		    safely_quote_err_filename(0, ent->fts_path));
	      error_severity(1);
	      return;
	    }
	}
    }
  
  /* Cope with the usual cases. */
  if (ent->fts_info == FTS_NSOK
      || ent->fts_info == FTS_NS /* e.g. symlink loop */)
    {
      assert (!state.have_stat);
      assert (!state.have_type);
      state.type = mode = 0;
    }
  else
    {
      state.have_stat = true;
      state.have_type = true;
      statbuf = *(ent->fts_statp);
      state.type = mode = statbuf.st_mode;
      
      if (00000 == mode)
	{
	  /* Savannah bug #16378. */
	  error(0, 0, _("Warning: file %s appears to have mode 0000"),
		quotearg_n_style(0, options.err_quoting_style, ent->fts_path));
	}
    }

  if (mode)
    {
      if (!digest_mode(mode, ent->fts_path, ent->fts_name, &statbuf, 0))
	return;
    }

  /* examine this item. */
  ignore = 0;
  isdir = S_ISDIR(mode)
    || (FTS_D  == ent->fts_info)
    || (FTS_DP == ent->fts_info)
    || (FTS_DC == ent->fts_info);

  if (isdir && (ent->fts_info == FTS_NSOK))
    {
      /* This is a directory, but fts did not stat it, so
       * presumably would not be planning to search its
       * children.  Force a stat of the file so that the
       * children can be checked.
       */
      fts_set(p, ent, FTS_AGAIN);
      return;
    }

  if (options.maxdepth >= 0)
    {
      if (ent->fts_level >= options.maxdepth)
	{
	  fts_set(p, ent, FTS_SKIP); /* descend no further */
	  
	  if (ent->fts_level > options.maxdepth) 
	    ignore = 1;		/* don't even look at this one */
	}
    }

  if ( (ent->fts_info == FTS_D) && !options.do_dir_first )
    {
      /* this is the preorder visit, but user said -depth */ 
      ignore = 1;
    }
  else if ( (ent->fts_info == FTS_DP) && options.do_dir_first )
    {
      /* this is the postorder visit, but user didn't say -depth */ 
      ignore = 1;
    }
  else if (ent->fts_level < options.mindepth)
    {
      ignore = 1;
    }

  if (!ignore)
    {
      visit(p, ent, &statbuf);
    }

  /* XXX: if we allow a build-up of pending arguments for "-execdir foo {} +" 
   * we need to execute them in the same directory as we found the item.  
   * If we are trying to do "find a -execdir echo {} +", we will need to 
   * echo 
   *      a while in the original working directory
   *      b while in a
   *      c while in b (just before leaving b)
   *
   * These restrictions are hard to satisfy while using fts().   The reason is
   * that it doesn't tell us just before we leave a directory.  For the moment, 
   * we punt and don't allow the arguments to build up.
   */
  if (state.execdirs_outstanding)
    {
      show_outstanding_execdirs(stderr);
      run_in_dir(p->fts_cwd_fd, complete_execdirs_cb, NULL);
    }

  if (ent->fts_info == FTS_DP)
    {
      /* we're leaving a directory. */
      state.stop_at_current_level = false;
    }
}



static void
find(char *arg)
{
  char * arglist[2];
  FTS *p;
  FTSENT *ent;
  

  state.starting_path_length = strlen(arg);
  inside_dir(AT_FDCWD);

  arglist[0] = arg;
  arglist[1] = NULL;
  
  switch (options.symlink_handling)
    {
    case SYMLINK_ALWAYS_DEREF:
      ftsoptions |= FTS_COMFOLLOW|FTS_LOGICAL;
      break;
	  
    case SYMLINK_DEREF_ARGSONLY:
      ftsoptions |= FTS_COMFOLLOW|FTS_PHYSICAL;
      break;
	  
    case SYMLINK_NEVER_DEREF:
      ftsoptions |= FTS_PHYSICAL;
      break;
    }

  if (options.stay_on_filesystem)
    ftsoptions |= FTS_XDEV;
      
  p = fts_open(arglist, ftsoptions, NULL);
  if (NULL == p)
    {
      error (0, errno, _("cannot search %s"),
	     safely_quote_err_filename(0, arg));
    }
  else
    {
      while ( (ent=fts_read(p)) != NULL )
	{
	  state.have_stat = false;
	  state.have_type = false;
	  state.type = 0;
	  consider_visiting(p, ent);
	}
      fts_close(p);
      p = NULL;
    }
}


static void 
process_all_startpoints(int argc, char *argv[])
{
  int i;

  /* figure out how many start points there are */
  for (i = 0; i < argc && !looks_like_expression(argv[i], true); i++)
    {
      state.starting_path_length = strlen(argv[i]); /* TODO: is this redundant? */
      find(argv[i]);
    }

  if (i == 0)
    {
      /* 
       * We use a temporary variable here because some actions modify 
       * the path temporarily.  Hence if we use a string constant, 
       * we get a coredump.  The best example of this is if we say 
       * "find -printf %H" (note, not "find . -printf %H").
       */
      char defaultpath[2] = ".";
      find(defaultpath);
    }
}




int
main (int argc, char **argv)
{
  int end_of_leading_options = 0; /* First arg after any -H/-L etc. */
  struct predicate *eval_tree;

  program_name = argv[0];
  state.exit_status = 0;
  state.execdirs_outstanding = false;
  state.cwd_dir_fd = AT_FDCWD;

  /* Set the option defaults before we do the locale initialisation as
   * check_nofollow() needs to be executed in the POSIX locale.
   */
  set_option_defaults(&options);
  
#ifdef HAVE_SETLOCALE
  setlocale (LC_ALL, "");
#endif

  bindtextdomain (PACKAGE, LOCALEDIR);
  textdomain (PACKAGE);
  atexit (close_stdout);

  /* Check for -P, -H or -L options.  Also -D and -O, which are 
   * both GNU extensions.
   */
  end_of_leading_options = process_leading_options(argc, argv);
  
  if (options.debug_options & DebugStat)
    options.xstat = debug_stat;

#ifdef DEBUG
  fprintf (stderr, "cur_day_start = %s", ctime (&options.cur_day_start));
#endif /* DEBUG */


  /* We are now processing the part of the "find" command line 
   * after the -H/-L options (if any).
   */
  eval_tree = build_expression_tree(argc, argv, end_of_leading_options);

  /* safely_chdir() needs to check that it has ended up in the right place. 
   * To avoid bailing out when something gets automounted, it checks if 
   * the target directory appears to have had a directory mounted on it as
   * we chdir()ed.  The problem with this is that in order to notice that 
   * a file system was mounted, we would need to lstat() all the mount points.
   * That strategy loses if our machine is a client of a dead NFS server.
   *
   * Hence if safely_chdir() and wd_sanity_check() can manage without needing 
   * to know the mounted device list, we do that.  
   */
  if (!options.open_nofollow_available)
    {
#ifdef STAT_MOUNTPOINTS
      init_mounted_dev_list();
#endif
    }
  

  starting_desc = open (".", O_RDONLY
#if defined O_LARGEFILE
			|O_LARGEFILE
#endif
			);
  if (0 <= starting_desc && fchdir (starting_desc) != 0)
    {
      close (starting_desc);
      starting_desc = -1;
    }
  if (starting_desc < 0)
    {
      starting_dir = xgetcwd ();
      if (! starting_dir)
	error (1, errno, _("cannot get current directory"));
    }


  process_all_startpoints(argc-end_of_leading_options, argv+end_of_leading_options);
  
  /* If "-exec ... {} +" has been used, there may be some 
   * partially-full command lines which have been built, 
   * but which are not yet complete.   Execute those now.
   */
  show_success_rates(eval_tree);
  cleanup();
  return state.exit_status;
}

boolean
is_fts_enabled(int *fts_options)
{
  /* this version of find (i.e. this main()) uses fts. */
  *fts_options = ftsoptions;
  return true;
}
