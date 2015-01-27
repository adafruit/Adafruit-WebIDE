/* util.c -- functions for initializing new tree elements, and other things.
   Copyright (C) 1990, 91, 92, 93, 94, 2000, 2003, 2004, 2005,
                 2008 Free Software Foundation, Inc.

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

#include <config.h>
#include "defs.h"

#include <fcntl.h>
#ifdef HAVE_SYS_UTSNAME_H
#include <sys/utsname.h>
#endif
#include <sys/time.h>
#include <ctype.h>
#include <string.h>
#include <limits.h>
#include <errno.h>
#include <assert.h>

#include "xalloc.h"
#include "quotearg.h"
#include "timespec.h"
#include "error.h"
#include "verify.h"
#include "openat.h"

#if ENABLE_NLS
# include <libintl.h>
# define _(Text) gettext (Text)
#else
# define _(Text) Text
#endif
#ifdef gettext_noop
# define N_(String) gettext_noop (String)
#else
/* See locate.c for explanation as to why not use (String) */
# define N_(String) String
#endif


struct debug_option_assoc
{
  char *name;
  int    val;
  char *docstring;
};
static struct debug_option_assoc debugassoc[] = 
  {
    { "help", DebugHelp, "Explain the various -D options" },
    { "tree", DebugExpressionTree, "Display the expression tree" },
    { "search",DebugSearch, "Navigate the directory tree verbosely" },
    { "stat", DebugStat, "Trace calls to stat(2) and lstat(2)" },
    { "rates", DebugSuccessRates, "Indicate how often each predicate succeeded" },
    { "opt",  DebugExpressionTree|DebugTreeOpt, "Show diagnostic information relating to optimisation" },
    { "exec", DebugExec,  "Show diagnostic information relating to -exec, -execdir, -ok and -okdir" }
  };
#define N_DEBUGASSOC (sizeof(debugassoc)/sizeof(debugassoc[0]))




/* Add a primary of predicate type PRED_FUNC (described by ENTRY) to the predicate input list.

   Return a pointer to the predicate node just inserted.

   Fills in the following cells of the new predicate node:

   pred_func	    PRED_FUNC
   args(.str)	    NULL
   p_type	    PRIMARY_TYPE
   p_prec	    NO_PREC

   Other cells that need to be filled in are defaulted by
   get_new_pred_chk_op, which is used to insure that the prior node is
   either not there at all (we are the very first node) or is an
   operator. */

struct predicate *
insert_primary_withpred (const struct parser_table *entry, PRED_FUNC pred_func)
{
  struct predicate *new_pred;

  new_pred = get_new_pred_chk_op (entry);
  new_pred->pred_func = pred_func;
  new_pred->p_name = entry->parser_name;
  new_pred->args.str = NULL;
  new_pred->p_type = PRIMARY_TYPE;
  new_pred->p_prec = NO_PREC;
  return new_pred;
}

/* Add a primary described by ENTRY to the predicate input list.

   Return a pointer to the predicate node just inserted.

   Fills in the following cells of the new predicate node:

   pred_func	    PRED_FUNC
   args(.str)	    NULL
   p_type	    PRIMARY_TYPE
   p_prec	    NO_PREC

   Other cells that need to be filled in are defaulted by
   get_new_pred_chk_op, which is used to insure that the prior node is
   either not there at all (we are the very first node) or is an
   operator. */
struct predicate *
insert_primary (const struct parser_table *entry)
{
  assert (entry->pred_func != NULL);
  return insert_primary_withpred(entry, entry->pred_func);
}



static void 
show_valid_debug_options(FILE *fp, int full)
{
  int i;
  if (full)
    {
      fprintf(fp, "Valid arguments for -D:\n");
      for (i=0; i<N_DEBUGASSOC; ++i)
	{
	  fprintf(fp, "%-10s %s\n",
		  debugassoc[i].name,
		  debugassoc[i].docstring);
	}
    }
  else
    {
      for (i=0; i<N_DEBUGASSOC; ++i)
	{
	  fprintf(fp, "%s%s", (i>0 ? "|" : ""), debugassoc[i].name);
	}
    }
}

void
usage (FILE *fp, int status, char *msg)
{
  if (msg)
    fprintf (fp, "%s: %s\n", program_name, msg);
  
  fprintf (fp, _("Usage: %s [-H] [-L] [-P] [-Olevel] [-D "), program_name);
  show_valid_debug_options(fp, 0);
  fprintf (fp, _("] [path...] [expression]\n"));
  if (0 != status)
    exit (status);
}

void 
set_stat_placeholders(struct stat *p)
{
#if HAVE_STRUCT_STAT_ST_BIRTHTIME
  p->st_birthtime = 0;
#endif
#if HAVE_STRUCT_STAT_ST_BIRTHTIMENSEC
  p->st_birthtimensec = 0;
#endif
#if HAVE_STRUCT_STAT_ST_BIRTHTIMESPEC_TV_NSEC
  p->st_birthtimespec.tv_nsec = -1;
#endif
#if HAVE_STRUCT_STAT_ST_BIRTHTIMESPEC_TV_SEC
  p->st_birthtimespec.tv_sec = 0;
#endif
}


/* Get the stat information for a file, if it is 
 * not already known. 
 */
int
get_statinfo (const char *pathname, const char *name, struct stat *p)
{
  /* Set markers in fields so we have a good idea if the implementation
   * didn't bother to set them (e.g., NetBSD st_birthtimespec for MS-DOS 
   * files)
   */
  if (!state.have_stat)
    {
      set_stat_placeholders(p);
      if (0 == (*options.xstat) (name, p))
	{
	  if (00000 == p->st_mode)
	    {
	      /* Savannah bug #16378. */
	      error(0, 0, _("Warning: file %s appears to have mode 0000"),
		    quotearg_n_style(0, options.err_quoting_style, name));
	    }
	}
      else
	{
	  if (!options.ignore_readdir_race || (errno != ENOENT) )
	    {
	      error (0, errno, "%s",
		     safely_quote_err_filename(0, pathname));
	      state.exit_status = 1;
	    }
	  return -1;
	}
    }
  state.have_stat = true;
  state.have_type = true;
  state.type = p->st_mode;

  return 0;
}


/* Get the stat/type information for a file, if it is 
 * not already known. 
 */
int
get_info (const char *pathname,
	  struct stat *p,
	  struct predicate *pred_ptr)
{
  boolean todo = false;
  
  /* If we need the full stat info, or we need the type info but don't 
   * already have it, stat the file now.
   */
  if (pred_ptr->need_stat)
    todo = true;
  else if ((pred_ptr->need_type && (0 == state.have_type)))
    todo = true;
  
  if (todo)
    return get_statinfo(pathname, state.rel_pathname, p);
  else
    return 0;
}

/* Determine if we can use O_NOFOLLOW.
 */
#if defined O_NOFOLLOW
boolean 
check_nofollow(void)
{
  struct utsname uts;
  float  release;

  if (0 == O_NOFOLLOW)
    {
      return false;
    }
  
  if (0 == uname(&uts))
    {
      /* POSIX requires that atof() ignore "unrecognised suffixes". */
      release = atof(uts.release);
      
      if (0 == strcmp("Linux", uts.sysname))
	{
	  /* Linux kernels 2.1.126 and earlier ignore the O_NOFOLLOW flag. */
	  return release >= 2.2; /* close enough */
	}
      else if (0 == strcmp("FreeBSD", uts.sysname)) 
	{
	  /* FreeBSD 3.0-CURRENT and later support it */
	  return release >= 3.1;
	}
    }

  /* Well, O_NOFOLLOW was defined, so we'll try to use it. */
  return true;
}
#endif



/* Examine the predicate list for instances of -execdir or -okdir
 * which have been terminated with '+' (build argument list) rather
 * than ';' (singles only).  If there are any, run them (this will
 * have no effect if there are no arguments waiting).
 */
static void
do_complete_pending_execdirs(struct predicate *p, int dir_fd)
{
  if (NULL == p)
    return;
  
  assert (state.execdirs_outstanding);
  
  do_complete_pending_execdirs(p->pred_left, dir_fd);
  
  if (pred_is(p, pred_execdir) || pred_is(p, pred_okdir))
    {
      /* It's an exec-family predicate.  p->args.exec_val is valid. */
      if (p->args.exec_vec.multiple)
	{
	  struct exec_val *execp = &p->args.exec_vec;
	  
	  /* This one was terminated by '+' and so might have some
	   * left... Run it if necessary.
	   */
	  if (execp->state.todo)
	    {
	      /* There are not-yet-executed arguments. */
	      launch (&execp->ctl, &execp->state);
	    }
	}
    }

  do_complete_pending_execdirs(p->pred_right, dir_fd);
}

void
complete_pending_execdirs(int dir_fd)
{
  if (state.execdirs_outstanding)
    {
      do_complete_pending_execdirs(get_eval_tree(), dir_fd);
      state.execdirs_outstanding = false;
    }
}



/* Examine the predicate list for instances of -exec which have been
 * terminated with '+' (build argument list) rather than ';' (singles
 * only).  If there are any, run them (this will have no effect if
 * there are no arguments waiting).
 */
void
complete_pending_execs(struct predicate *p)
{
  if (NULL == p)
    return;
  
  complete_pending_execs(p->pred_left);
  
  /* It's an exec-family predicate then p->args.exec_val is valid
   * and we can check it. 
   */
  /* XXX: what about pred_ok() ? */
  if (pred_is(p, pred_exec) && p->args.exec_vec.multiple)
    {
      struct exec_val *execp = &p->args.exec_vec;
      
      /* This one was terminated by '+' and so might have some
       * left... Run it if necessary.  Set state.exit_status if
       * there are any problems.
       */
      if (execp->state.todo)
	{
	  /* There are not-yet-executed arguments. */
	  launch (&execp->ctl, &execp->state);
	}
    }

  complete_pending_execs(p->pred_right);
}

static void
traverse_tree(struct predicate *tree,
			  void (*callback)(struct predicate*))
{
  if (tree->pred_left)
    traverse_tree(tree->pred_left, callback);

  callback(tree);
  
  if (tree->pred_right)
    traverse_tree(tree->pred_right, callback);
}

static void
flush_and_close_output_files(struct predicate *p)
{
  if (pred_is(p, pred_fprint)
      || pred_is(p, pred_fprintf)
      || pred_is(p, pred_fls)
      || pred_is(p, pred_fprint0))
    {
      FILE *f = p->args.printf_vec.stream;
      bool failed;
      
      if (f == stdout || f == stderr)
	failed = fflush(p->args.printf_vec.stream) == EOF;
      else
	failed = fclose(p->args.printf_vec.stream) == EOF;
     
      if (failed)
	  nonfatal_file_error(p->args.printf_vec.filename);
    }
  else if (pred_is(p, pred_print))
    {
      if (fflush(p->args.printf_vec.stream) == EOF)
	{
	  nonfatal_file_error(p->args.printf_vec.filename);
	}
    }
  else if (pred_is(p, pred_ls) || pred_is(p, pred_print0))
    {
      if (fflush(stdout) == EOF)
	{
	  /* XXX: migrate to printf_vec. */
	  nonfatal_file_error("standard output");
	}
    }
}

/* Complete any outstanding commands.
 */
void 
cleanup(void)
{
  struct predicate *eval_tree = get_eval_tree();
  if (eval_tree)
    {
      traverse_tree(eval_tree, complete_pending_execs);
      complete_pending_execdirs(get_current_dirfd());
      traverse_tree(eval_tree, flush_and_close_output_files);
    }
}

/* Savannah bug #16378 manifests as an assertion failure in pred_type()
 * when an NFS server returns st_mode with value 0 (of course the stat(2)
 * system call is itself returning 0 in this case). 
 */
#undef DEBUG_SV_BUG_16378
#if defined DEBUG_SV_BUG_16378
static int hook_fstatat(int fd, const char *name, struct stat *p, int flags)
{
  static int warned = 0;

  if (!warned)
    {
      /* No use of _() here; no point asking translators to translate a debug msg */
      error(0, 0,
	    "Warning: some debug code is enabled for Savannah bug #16378; "
	    "this should not occur in released versions of findutils!");
      warned = 1;
    }
  
  if (0 == strcmp(name, "./mode0file")
      || 0 == strcmp(name, "mode0file")) 
    {
      time_t now = time(NULL);
      long day = 86400;
      
      p->st_rdev = 0;
      p->st_dev = 0x300;
      p->st_ino = 0;
      p->st_mode = 0;		/* SV bug #16378 */
      p->st_nlink = 1;
      p->st_uid = geteuid();
      p->st_gid = 0;
      p->st_size = 42;
      p->st_blksize = 32768;
      p->st_atime = now-1*day;
      p->st_mtime = now-2*day;
      p->st_ctime = now-3*day;

      return 0;
    }
  return fstatat(fd, name, p, flags);
}

# undef  fstatat
# define fstatat(fd,name,p,flags) hook_fstatat((fd),(name),(p),(flags))
#endif


static int
fallback_stat(const char *name, struct stat *p, int prev_rv)
{
  /* Our original stat() call failed.  Perhaps we can't follow a
   * symbolic link.  If that might be the problem, lstat() the link. 
   * Otherwise, admit defeat. 
   */
  switch (errno)
    {
    case ENOENT:
    case ENOTDIR:
      if (options.debug_options & DebugStat)
	fprintf(stderr, "fallback_stat(): stat(%s) failed; falling back on lstat()\n", name);
      return fstatat(state.cwd_dir_fd, name, p, AT_SYMLINK_NOFOLLOW);

    case EACCES:
    case EIO:
    case ELOOP:
    case ENAMETOOLONG:
#ifdef EOVERFLOW
    case EOVERFLOW:	    /* EOVERFLOW is not #defined on UNICOS. */
#endif
    default:
      return prev_rv;	       
    }
}


/* optionh_stat() implements the stat operation when the -H option is
 * in effect.
 * 
 * If the item to be examined is a command-line argument, we follow
 * symbolic links.  If the stat() call fails on the command-line item,
 * we fall back on the properties of the symbolic link.
 *
 * If the item to be examined is not a command-line argument, we
 * examine the link itself.
 */
int 
optionh_stat(const char *name, struct stat *p)
{
  if (AT_FDCWD != state.cwd_dir_fd)
    assert (state.cwd_dir_fd >= 0);
  set_stat_placeholders(p);
  if (0 == state.curdepth) 
    {
      /* This file is from the command line; deference the link (if it
       * is a link).  
       */
      int rv;
      rv = fstatat(state.cwd_dir_fd, name, p, 0);
      if (0 == rv)
	return 0;		/* success */
      else
	return fallback_stat(name, p, rv);
    }
  else
    {
      /* Not a file on the command line; do not dereference the link.
       */
      return fstatat(state.cwd_dir_fd, name, p, AT_SYMLINK_NOFOLLOW);
    }
}

/* optionl_stat() implements the stat operation when the -L option is
 * in effect.  That option makes us examine the thing the symbolic
 * link points to, not the symbolic link itself.
 */
int 
optionl_stat(const char *name, struct stat *p)
{
  int rv;
  if (AT_FDCWD != state.cwd_dir_fd)
    assert (state.cwd_dir_fd >= 0);
  
  set_stat_placeholders(p);
  rv = fstatat(state.cwd_dir_fd, name, p, 0);
  if (0 == rv)
    return 0;			/* normal case. */
  else
    return fallback_stat(name, p, rv);
}

/* optionp_stat() implements the stat operation when the -P option is
 * in effect (this is also the default).  That option makes us examine
 * the symbolic link itself, not the thing it points to.
 */
int 
optionp_stat(const char *name, struct stat *p)
{
  assert ((state.cwd_dir_fd >= 0) || (state.cwd_dir_fd==AT_FDCWD));
  set_stat_placeholders(p);
  return fstatat(state.cwd_dir_fd, name, p, AT_SYMLINK_NOFOLLOW);
}


static uintmax_t stat_count = 0u;

int
debug_stat (const char *file, struct stat *bufp)
{
  ++stat_count;
  fprintf (stderr, "debug_stat (%s)\n", file);

  switch (options.symlink_handling)
    {
    case SYMLINK_ALWAYS_DEREF:
      return optionl_stat(file, bufp);
    case SYMLINK_DEREF_ARGSONLY:
      return optionh_stat(file, bufp);
    case SYMLINK_NEVER_DEREF:
      return optionp_stat(file, bufp);
    }
  /*NOTREACHED*/
  assert (0);
  return -1;
}


int
following_links(void)
{
  switch (options.symlink_handling)
    {
    case SYMLINK_ALWAYS_DEREF:
      return 1;
    case SYMLINK_DEREF_ARGSONLY:
      return (state.curdepth == 0);
    case SYMLINK_NEVER_DEREF:
    default:
      return 0;
    }
}


/* Take a "mode" indicator and fill in the files of 'state'.
 */
int
digest_mode(mode_t mode,
	    const char *pathname,
	    const char *name,
	    struct stat *pstat,
	    boolean leaf)
{
  /* If we know the type of the directory entry, and it is not a
   * symbolic link, we may be able to avoid a stat() or lstat() call.
   */
  if (mode)
    {
      if (S_ISLNK(mode) && following_links())
	{
	  /* mode is wrong because we should have followed the symlink. */
	  if (get_statinfo(pathname, name, pstat) != 0)
	    return 0;
	  mode = state.type = pstat->st_mode;
	  state.have_type = true;
	}
      else
	{
	  state.have_type = true;
	  pstat->st_mode = state.type = mode;
	}
    }
  else
    {
      /* Mode is not yet known; may have to stat the file unless we 
       * can deduce that it is not a directory (which is all we need to 
       * know at this stage)
       */
      if (leaf)
	{
	  state.have_stat = false;
	  state.have_type = false;;
	  state.type = 0;
	}
      else
	{
	  if (get_statinfo(pathname, name, pstat) != 0)
	    return 0;
	  
	  /* If -L is in effect and we are dealing with a symlink,
	   * st_mode is the mode of the pointed-to file, while mode is
	   * the mode of the directory entry (S_IFLNK).  Hence now
	   * that we have the stat information, override "mode".
	   */
	  state.type = pstat->st_mode;
	  state.have_type = true;
	}
    }

  /* success. */
  return 1;
}


/* Return true if there are no predicates with no_default_print in
   predicate list PRED, false if there are any.
   Returns true if default print should be performed */

boolean
default_prints (struct predicate *pred)
{
  while (pred != NULL)
    {
      if (pred->no_default_print)
	return (false);
      pred = pred->pred_next;
    }
  return (true);
}

boolean 
looks_like_expression(const char *arg, boolean leading)
{
  switch (arg[0])
    {
    case '-':
      if (arg[1])		/* "-foo" is an expression.  */
	return true;
      else
	return false;		/* Just "-" is a filename. */
      break;
      
    case ')':
    case ',':
      if (arg[1])
	return false;		/* )x and ,z are not expressions */
      else
	return !leading;	/* A leading ) or , is not either */
      
      /* ( and ! are part of an expression, but (2 and !foo are
       * filenames.
       */
    case '!':
    case '(':
      if (arg[1])
	return false;
      else
	return true;

    default:
      return false;
    }
}

static void
process_debug_options(char *arg)
{
  const char *p;
  char *token_context = NULL;
  const char delimiters[] = ",";
  boolean empty = true;
  size_t i;
  
  p = strtok_r(arg, delimiters, &token_context);
  while (p)
    {
      empty = false;

      for (i=0; i<N_DEBUGASSOC; ++i)
	{
	  if (0 == strcmp(debugassoc[i].name, p))
	    {
	      options.debug_options |= debugassoc[i].val;
	      break;
	    }
	}
      if (i >= N_DEBUGASSOC)
	{
	  error(0, 0, _("Ignoring unrecognised debug flag %s"),
		quotearg_n_style(0, options.err_quoting_style, arg));
	}
      p = strtok_r(NULL, delimiters, &token_context);
    }
  if (empty)
    {
      error(1, 0, _("Empty argument to the -D option."));
    }
  else if (options.debug_options & DebugHelp) 
    {
      show_valid_debug_options(stdout, 1);
      exit(0);
    }
}

static void
process_optimisation_option(const char *arg)
{
  if (0 == arg[0])
    {
      error(1, 0, _("The -O option must be immediately followed by a decimal integer"));
    }
  else 
    {
      unsigned long opt_level;
      char *end;

      if (!isdigit( (unsigned char) arg[0] ))
	{
	  error(1, 0, _("Please specify a decimal number immediately after -O"));
	}
      else 
	{
	  int prev_errno = errno;
	  errno  = 0;
	  
	  opt_level = strtoul(arg, &end, 10);
	  if ( (0==opt_level) && (end==arg) )
	    {
	      error(1, 0, _("Please specify a decimal number immediately after -O"));
	    }
	  else if (*end)
	    {
	      /* unwanted trailing characters. */
	      error(1, 0, _("Invalid optimisation level %s"), arg);
	    }
	  else if ( (ULONG_MAX==opt_level) && errno)
	    {
	      error(1, errno, _("Invalid optimisation level %s"), arg);
	    }
	  else if (opt_level > USHRT_MAX)
	    {
	      /* tricky to test, as on some platforms USHORT_MAX and ULONG_MAX
	       * can have the same value, though this is unusual.
	       */
	      error(1, 0, _("Optimisation level %lu is too high.  "
			    "If you want to find files very quickly, "
			    "consider using GNU locate."),
		    opt_level);
	    }
	  else
	    {
	      options.optimisation_level = opt_level;
	      errno = prev_errno;
	    }
	}
    }
}

int
process_leading_options(int argc, char *argv[])
{
  int i, end_of_leading_options;
  
  for (i=1; (end_of_leading_options = i) < argc; ++i)
    {
      if (0 == strcmp("-H", argv[i]))
	{
	  /* Meaning: dereference symbolic links on command line, but nowhere else. */
	  set_follow_state(SYMLINK_DEREF_ARGSONLY);
	}
      else if (0 == strcmp("-L", argv[i]))
	{
	  /* Meaning: dereference all symbolic links. */
	  set_follow_state(SYMLINK_ALWAYS_DEREF);
	}
      else if (0 == strcmp("-P", argv[i]))
	{
	  /* Meaning: never dereference symbolic links (default). */
	  set_follow_state(SYMLINK_NEVER_DEREF);
	}
      else if (0 == strcmp("--", argv[i]))
	{
	  /* -- signifies the end of options. */
	  end_of_leading_options = i+1;	/* Next time start with the next option */
	  break;
	}
      else if (0 == strcmp("-D", argv[i]))
	{
	  process_debug_options(argv[i+1]);
	  ++i;			/* skip the argument too. */
	}
      else if (0 == strncmp("-O", argv[i], 2))
	{
	  process_optimisation_option(argv[i]+2);
	}
      else
	{
	  /* Hmm, must be one of 
	   * (a) A path name
	   * (b) A predicate
	   */
	  end_of_leading_options = i; /* Next time start with this option */
	  break;
	}
    }
  return end_of_leading_options;
}

static struct timespec 
now(void)
{
  struct timespec retval;
  struct timeval tv;
  time_t t;
  
  if (0 == gettimeofday(&tv, NULL))
    {
      retval.tv_sec  = tv.tv_sec;
      retval.tv_nsec = tv.tv_usec * 1000; /* convert unit from microseconds to nanoseconds */
      return retval;
    }
  t = time(NULL);
  assert (t != (time_t)-1);
  retval.tv_sec = t;
  retval.tv_nsec = 0;
  return retval;
}

void 
set_option_defaults(struct options *p)
{
  if (getenv("POSIXLY_CORRECT"))
    p->posixly_correct = true;
  else
    p->posixly_correct = false;
  
  /* We call check_nofollow() before setlocale() because the numbers 
   * for which we check (in the results of uname) definitiely have "."
   * as the decimal point indicator even under locales for which that 
   * is not normally true.   Hence atof() would do the wrong thing 
   * if we call it after setlocale().
   */
#ifdef O_NOFOLLOW
  p->open_nofollow_available = check_nofollow();
#else
  p->open_nofollow_available = false;
#endif
  
  p->regex_options = RE_SYNTAX_EMACS;
  
  if (isatty(0))
    {
      p->warnings = true;
      p->literal_control_chars = false;
    }
  else
    {
      p->warnings = false;
      p->literal_control_chars = false; /* may change */
    }
  if (p->posixly_correct)
    {
      p->warnings = false;
    }
  
  p->do_dir_first = true;
  p->explicit_depth = false;
  p->maxdepth = p->mindepth = -1;

  p->start_time = now();
  p->cur_day_start.tv_sec = p->start_time.tv_sec - DAYSECS;
  p->cur_day_start.tv_nsec = p->start_time.tv_nsec;

  p->full_days = false;
  p->stay_on_filesystem = false;
  p->ignore_readdir_race = false;

  if (p->posixly_correct)
    p->output_block_size = 512;
  else
    p->output_block_size = 1024;

  p->debug_options = 0uL;
  p->optimisation_level = 0;
  
  if (getenv("FIND_BLOCK_SIZE"))
    {
      error (1, 0, _("The environment variable FIND_BLOCK_SIZE is not supported, the only thing that affects the block size is the POSIXLY_CORRECT environment variable"));
    }

#if LEAF_OPTIMISATION
  /* The leaf optimisation is enabled. */
  p->no_leaf_check = false;
#else
  /* The leaf optimisation is disabled. */
  p->no_leaf_check = true;
#endif

  set_follow_state(SYMLINK_NEVER_DEREF); /* The default is equivalent to -P. */

  p->err_quoting_style = locale_quoting_style;
}


/* get_start_dirfd
 *
 * Returns the fd for the directory we started in.
 */
int get_start_dirfd(void)
{
  return starting_desc;
}

/* apply_predicate
 *
 */
boolean
apply_predicate(const char *pathname, struct stat *stat_buf, struct predicate *p)
{
  ++p->perf.visits;

  if (p->need_stat || p->need_type)
    {
      /* We may need a stat here. */
      if (get_info(pathname, stat_buf, p) != 0)
	    return false;
    }
  if ((p->pred_func)(pathname, stat_buf, p))
    {
      ++(p->perf.successes);
      return true;
    }
  else
    {
      return false;
    }
}


/* safely_quote_err_filename
 *
 */
const char *
safely_quote_err_filename (int n, char const *arg)
{
  return quotearg_n_style (n, options.err_quoting_style, arg);
}

/* report_file_err
 */
static void
report_file_err(int exitval, int errno_value, const char *name)
{
  /* It is important that the errno value is passed in as a function
   * argument before we call safely_quote_err_filename(), because otherwise 
   * we might find that safely_quote_err_filename() changes errno.
   */
  if (state.exit_status < 1)
    state.exit_status = 1;

  error (exitval, errno_value, "%s", safely_quote_err_filename(0, name));
}

/* fatal_file_error
 *
 */
void
fatal_file_error(const char *name)
{
  report_file_err(1, errno, name);
  /*NOTREACHED*/
  abort();
}

void
nonfatal_file_error(const char *name)
{
  report_file_err(0, errno, name);
}

