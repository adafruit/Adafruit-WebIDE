/* pred.c -- execute the expression tree.
   Copyright (C) 1990, 1991, 1992, 1993, 1994, 2000, 2003,
                 2004, 2005, 2006, 2007, 2008, 2009 Free Software Foundation, Inc.

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

#include <fnmatch.h>
#include <signal.h>
#include <math.h>
#include <pwd.h>
#include <grp.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <assert.h>
#include <stdarg.h>
#include <fcntl.h>
#include <locale.h>
#include <openat.h>
#include <ctype.h>
#include "xalloc.h"
#include "dirname.h"
#include "human.h"
#include "modetype.h"
#include "filemode.h"
#include "wait.h"
#include "printquoted.h"
#include "buildcmd.h"
#include "yesno.h"
#include "listfile.h"
#include "stat-time.h"
#include "dircallback.h"
#include "error.h"
#include "verify.h"

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

#if !defined(SIGCHLD) && defined(SIGCLD)
#define SIGCHLD SIGCLD
#endif



#if HAVE_DIRENT_H
# include <dirent.h>
# define NAMLEN(dirent) strlen((dirent)->d_name)
#else
# define dirent direct
# define NAMLEN(dirent) (dirent)->d_namlen
# if HAVE_SYS_NDIR_H
#  include <sys/ndir.h>
# endif
# if HAVE_SYS_DIR_H
#  include <sys/dir.h>
# endif
# if HAVE_NDIR_H
#  include <ndir.h>
# endif
#endif

#ifdef CLOSEDIR_VOID
/* Fake a return value. */
#define CLOSEDIR(d) (closedir (d), 0)
#else
#define CLOSEDIR(d) closedir (d)
#endif




/* Get or fake the disk device blocksize.
   Usually defined by sys/param.h (if at all).  */
#ifndef DEV_BSIZE
# ifdef BSIZE
#  define DEV_BSIZE BSIZE
# else /* !BSIZE */
#  define DEV_BSIZE 4096
# endif /* !BSIZE */
#endif /* !DEV_BSIZE */

/* Extract or fake data from a `struct stat'.
   ST_BLKSIZE: Preferred I/O blocksize for the file, in bytes.
   ST_NBLOCKS: Number of blocks in the file, including indirect blocks.
   ST_NBLOCKSIZE: Size of blocks used when calculating ST_NBLOCKS.  */
#ifndef HAVE_STRUCT_STAT_ST_BLOCKS
# define ST_BLKSIZE(statbuf) DEV_BSIZE
# if defined _POSIX_SOURCE || !defined BSIZE /* fileblocks.c uses BSIZE.  */
#  define ST_NBLOCKS(statbuf) \
  (S_ISREG ((statbuf).st_mode) \
   || S_ISDIR ((statbuf).st_mode) \
   ? (statbuf).st_size / ST_NBLOCKSIZE + ((statbuf).st_size % ST_NBLOCKSIZE != 0) : 0)
# else /* !_POSIX_SOURCE && BSIZE */
#  define ST_NBLOCKS(statbuf) \
  (S_ISREG ((statbuf).st_mode) \
   || S_ISDIR ((statbuf).st_mode) \
   ? st_blocks ((statbuf).st_size) : 0)
# endif /* !_POSIX_SOURCE && BSIZE */
#else /* HAVE_STRUCT_STAT_ST_BLOCKS */
/* Some systems, like Sequents, return st_blksize of 0 on pipes. */
# define ST_BLKSIZE(statbuf) ((statbuf).st_blksize > 0 \
			       ? (statbuf).st_blksize : DEV_BSIZE)
# if defined hpux || defined __hpux__ || defined __hpux
/* HP-UX counts st_blocks in 1024-byte units.
   This loses when mixing HP-UX and BSD file systems with NFS.  */
#  define ST_NBLOCKSIZE 1024
# else /* !hpux */
#  if defined _AIX && defined _I386
/* AIX PS/2 counts st_blocks in 4K units.  */
#   define ST_NBLOCKSIZE (4 * 1024)
#  else /* not AIX PS/2 */
#   if defined _CRAY
#    define ST_NBLOCKS(statbuf) \
  (S_ISREG ((statbuf).st_mode) \
   || S_ISDIR ((statbuf).st_mode) \
   ? (statbuf).st_blocks * ST_BLKSIZE(statbuf)/ST_NBLOCKSIZE : 0)
#   endif /* _CRAY */
#  endif /* not AIX PS/2 */
# endif /* !hpux */
#endif /* HAVE_STRUCT_STAT_ST_BLOCKS */

#ifndef ST_NBLOCKS
# define ST_NBLOCKS(statbuf) \
  (S_ISREG ((statbuf).st_mode) \
   || S_ISDIR ((statbuf).st_mode) \
   ? (statbuf).st_blocks : 0)
#endif

#ifndef ST_NBLOCKSIZE
# define ST_NBLOCKSIZE 512
#endif


#undef MAX
#define MAX(a, b) ((a) > (b) ? (a) : (b))

static boolean match_lname PARAMS((const char *pathname, struct stat *stat_buf, struct predicate *pred_ptr, boolean ignore_case));

static char *format_date PARAMS((struct timespec ts, int kind));
static char *ctime_format PARAMS((struct timespec ts));

#ifdef	DEBUG
struct pred_assoc
{
  PRED_FUNC pred_func;
  char *pred_name;
};

struct pred_assoc pred_table[] =
{
  {pred_amin, "amin    "},
  {pred_and, "and     "},
  {pred_anewer, "anewer  "},
  {pred_atime, "atime   "},
  {pred_closeparen, ")       "},
  {pred_cmin, "cmin    "},
  {pred_cnewer, "cnewer  "},
  {pred_comma, ",       "},
  {pred_ctime, "ctime   "},
  {pred_delete, "delete  "},
  {pred_empty, "empty   "},
  {pred_exec, "exec    "},
  {pred_execdir, "execdir "},
  {pred_executable, "executable "},
  {pred_false, "false   "},
  {pred_fprint, "fprint  "},
  {pred_fprint0, "fprint0 "},
  {pred_fprintf, "fprintf "},
  {pred_fstype, "fstype  "},
  {pred_gid, "gid     "},
  {pred_group, "group   "},
  {pred_ilname, "ilname  "},
  {pred_iname, "iname   "},
  {pred_inum, "inum    "},
  {pred_ipath, "ipath   "},
  {pred_links, "links   "},
  {pred_lname, "lname   "},
  {pred_ls, "ls      "},
  {pred_mmin, "mmin    "},
  {pred_mtime, "mtime   "},
  {pred_name, "name    "},
  {pred_negate, "not     "},
  {pred_newer, "newer   "},
  {pred_newerXY, "newerXY   "},
  {pred_nogroup, "nogroup "},
  {pred_nouser, "nouser  "},
  {pred_ok, "ok      "},
  {pred_okdir, "okdir   "},
  {pred_openparen, "(       "},
  {pred_or, "or      "},
  {pred_path, "path    "},
  {pred_perm, "perm    "},
  {pred_print, "print   "},
  {pred_print0, "print0  "},
  {pred_prune, "prune   "},
  {pred_quit, "quit    "},
  {pred_readable, "readable    "},
  {pred_regex, "regex   "},
  {pred_samefile,"samefile "},
  {pred_size, "size    "},
  {pred_true, "true    "},
  {pred_type, "type    "},
  {pred_uid, "uid     "},
  {pred_used, "used    "},
  {pred_user, "user    "},
  {pred_writable, "writable "},
  {pred_xtype, "xtype   "},
  {0, "none    "}
};
#endif

/* Returns ts1 - ts2 */
static double ts_difference(struct timespec ts1,
			    struct timespec ts2)
{
  double d =  difftime(ts1.tv_sec, ts2.tv_sec)
    + (1.0e-9 * (ts1.tv_nsec - ts2.tv_nsec));
  return d;
}


static int
compare_ts(struct timespec ts1,
	   struct timespec ts2)
{
  if ((ts1.tv_sec == ts2.tv_sec) &&
      (ts1.tv_nsec == ts2.tv_nsec))
    {
      return 0;
    }
  else
    {
      double diff = ts_difference(ts1, ts2);
      return diff < 0.0 ? -1 : +1;
    }
}

/* Predicate processing routines.

   PATHNAME is the full pathname of the file being checked.
   *STAT_BUF contains information about PATHNAME.
   *PRED_PTR contains information for applying the predicate.

   Return true if the file passes this predicate, false if not. */


/* pred_timewindow
 *
 * Returns true if THE_TIME is
 * COMP_GT: after the specified time
 * COMP_LT: before the specified time
 * COMP_EQ: after the specified time but by not more than WINDOW seconds.
 */
static boolean
pred_timewindow(struct timespec ts, struct predicate const *pred_ptr, int window)
{
  switch (pred_ptr->args.reftime.kind)
    {
    case COMP_GT:
      return compare_ts(ts, pred_ptr->args.reftime.ts) > 0;

    case COMP_LT:
      return compare_ts(ts, pred_ptr->args.reftime.ts) < 0;

    case COMP_EQ:
      {
	/* consider "find . -mtime 0".
	 *
	 * Here, the origin is exactly 86400 seconds before the start
	 * of the program (since -daystart was not specified).   This
	 * function will be called with window=86400 and
	 * pred_ptr->args.reftime.ts as the origin.  Hence a file
	 * created the instant the program starts will show a time
	 * difference (value of delta) of 86400.   Similarly, a file
	 * created exactly 24h ago would be the newest file which was
	 * _not_ created today.   So, if delta is 0.0, the file
	 * was not created today.  If the delta is 86400, the file
	 * was created this instant.
	 */
	double delta = ts_difference(ts, pred_ptr->args.reftime.ts);
	return (delta > 0.0 && delta <= window);
      }
    }
  assert (0);
  abort ();
}


boolean
pred_amin (const char *pathname, struct stat *stat_buf, struct predicate *pred_ptr)
{
  (void) &pathname;
  return pred_timewindow(get_stat_atime(stat_buf), pred_ptr, 60);
}

boolean
pred_and (const char *pathname, struct stat *stat_buf, struct predicate *pred_ptr)
{
  if (pred_ptr->pred_left == NULL
      || apply_predicate(pathname, stat_buf, pred_ptr->pred_left))
    {
      return apply_predicate(pathname, stat_buf, pred_ptr->pred_right);
    }
  else
    return false;
}

boolean
pred_anewer (const char *pathname, struct stat *stat_buf, struct predicate *pred_ptr)
{
  (void) &pathname;
  assert (COMP_GT == pred_ptr->args.reftime.kind);
  return compare_ts(get_stat_atime(stat_buf), pred_ptr->args.reftime.ts) > 0;
}

boolean
pred_atime (const char *pathname, struct stat *stat_buf, struct predicate *pred_ptr)
{
  (void) &pathname;
  return pred_timewindow(get_stat_atime(stat_buf), pred_ptr, DAYSECS);
}

boolean
pred_closeparen (const char *pathname, struct stat *stat_buf, struct predicate *pred_ptr)
{
  (void) &pathname;
  (void) &stat_buf;
  (void) &pred_ptr;

  return true;
}

boolean
pred_cmin (const char *pathname, struct stat *stat_buf, struct predicate *pred_ptr)
{
  (void) pathname;
  return pred_timewindow(get_stat_ctime(stat_buf), pred_ptr, 60);
}

boolean
pred_cnewer (const char *pathname, struct stat *stat_buf, struct predicate *pred_ptr)
{
  (void) pathname;

  assert (COMP_GT == pred_ptr->args.reftime.kind);
  return compare_ts(get_stat_ctime(stat_buf), pred_ptr->args.reftime.ts) > 0;
}

boolean
pred_comma (const char *pathname, struct stat *stat_buf, struct predicate *pred_ptr)
{
  if (pred_ptr->pred_left != NULL)
    {
      apply_predicate(pathname, stat_buf,pred_ptr->pred_left);
    }
  return apply_predicate(pathname, stat_buf, pred_ptr->pred_right);
}

boolean
pred_ctime (const char *pathname, struct stat *stat_buf, struct predicate *pred_ptr)
{
  (void) &pathname;
  return pred_timewindow(get_stat_ctime(stat_buf), pred_ptr, DAYSECS);
}

static boolean
perform_delete(int flags)
{
  return 0 == unlinkat(state.cwd_dir_fd, state.rel_pathname, flags);
}


boolean
pred_delete (const char *pathname, struct stat *stat_buf, struct predicate *pred_ptr)
{
  (void) pred_ptr;
  (void) stat_buf;
  if (strcmp (state.rel_pathname, "."))
    {
      int flags=0;
      if (state.have_stat && S_ISDIR(stat_buf->st_mode))
	flags |= AT_REMOVEDIR;
      if (perform_delete(flags))
	{
	  return true;
	}
      else
	{
	  if (EISDIR == errno)
	    {
	      if ((flags & AT_REMOVEDIR) == 0)
		{
		  /* unlink() operation failed because we should have done rmdir(). */
		  flags |= AT_REMOVEDIR;
		  if (perform_delete(flags))
		    return true;
		}
	    }
	}
      error (0, errno, _("cannot delete %s"),
	     safely_quote_err_filename(0, pathname));
      /* Previously I had believed that having the -delete action
       * return false provided the user with control over whether an
       * error message is issued.  While this is true, the policy of
       * not affecting the exit status is contrary to the POSIX
       * requirement that diagnostic messages are accompanied by a
       * nonzero exit status.  While -delete is not a POSIX option and
       * we can therefore opt not to follow POSIX in this case, that
       * seems somewhat arbitrary and confusing.  So, as of
       * findutils-4.3.11, we also set the exit status in this case.
       */
      state.exit_status = 1;
      return false;
    }
  else
    {
      /* nothing to do. */
      return true;
    }
}

boolean
pred_empty (const char *pathname, struct stat *stat_buf, struct predicate *pred_ptr)
{
  (void) pathname;
  (void) pred_ptr;

  if (S_ISDIR (stat_buf->st_mode))
    {
      int fd;
      DIR *d;
      struct dirent *dp;
      boolean empty = true;

      errno = 0;
      if ((fd = openat(state.cwd_dir_fd, state.rel_pathname, O_RDONLY
#if defined O_LARGEFILE
			|O_LARGEFILE
#endif
		       )) < 0)
	{
	  error (0, errno, "%s", safely_quote_err_filename(0, pathname));
	  state.exit_status = 1;
	  return false;
	}
      d = fdopendir (fd);
      if (d == NULL)
	{
	  error (0, errno, "%s", safely_quote_err_filename(0, pathname));
	  state.exit_status = 1;
	  return false;
	}
      for (dp = readdir (d); dp; dp = readdir (d))
	{
	  if (dp->d_name[0] != '.'
	      || (dp->d_name[1] != '\0'
		  && (dp->d_name[1] != '.' || dp->d_name[2] != '\0')))
	    {
	      empty = false;
	      break;
	    }
	}
      if (CLOSEDIR (d))
	{
	  error (0, errno, "%s", safely_quote_err_filename(0, pathname));
	  state.exit_status = 1;
	  return false;
	}
      return (empty);
    }
  else if (S_ISREG (stat_buf->st_mode))
    return (stat_buf->st_size == 0);
  else
    return (false);
}

static boolean
new_impl_pred_exec (int dir_fd, const char *pathname,
		    struct stat *stat_buf,
		    struct predicate *pred_ptr,
		    const char *prefix, size_t pfxlen)
{
  struct exec_val *execp = &pred_ptr->args.exec_vec;
  size_t len = strlen(pathname);

  (void) stat_buf;
  execp->dir_fd = dir_fd;
  if (execp->multiple)
    {
      /* Push the argument onto the current list.
       * The command may or may not be run at this point,
       * depending on the command line length limits.
       */
      bc_push_arg(&execp->ctl,
		  &execp->state,
		  pathname, len+1,
		  prefix, pfxlen,
		  0);

      /* remember that there are pending execdirs. */
      state.execdirs_outstanding = true;

      /* POSIX: If the primary expression is punctuated by a plus
       * sign, the primary shall always evaluate as true
       */
      return true;
    }
  else
    {
      int i;

      for (i=0; i<execp->num_args; ++i)
	{
	  bc_do_insert(&execp->ctl,
		       &execp->state,
		       execp->replace_vec[i],
		       strlen(execp->replace_vec[i]),
		       prefix, pfxlen,
		       pathname, len,
		       0);
	}

      /* Actually invoke the command. */
      return  execp->ctl.exec_callback(&execp->ctl,
					&execp->state);
    }
}


boolean
pred_exec (const char *pathname, struct stat *stat_buf, struct predicate *pred_ptr)
{
  return new_impl_pred_exec(get_start_dirfd(),
			    pathname, stat_buf, pred_ptr, NULL, 0);
}

boolean
pred_execdir (const char *pathname, struct stat *stat_buf, struct predicate *pred_ptr)
{
   const char *prefix = (state.rel_pathname[0] == '/') ? NULL : "./";
   (void) &pathname;
   return new_impl_pred_exec (get_current_dirfd(),
			      state.rel_pathname, stat_buf, pred_ptr,
			      prefix, (prefix ? 2 : 0));
}

boolean
pred_false (const char *pathname, struct stat *stat_buf, struct predicate *pred_ptr)
{
  (void) &pathname;
  (void) &stat_buf;
  (void) &pred_ptr;


  return (false);
}

boolean
pred_fls (const char *pathname, struct stat *stat_buf, struct predicate *pred_ptr)
{
  FILE * stream = pred_ptr->args.printf_vec.stream;
  list_file (pathname, state.cwd_dir_fd, state.rel_pathname, stat_buf,
	     options.start_time.tv_sec,
	     options.output_block_size,
	     pred_ptr->literal_control_chars, stream);
  return true;
}

boolean
pred_fprint (const char *pathname, struct stat *stat_buf, struct predicate *pred_ptr)
{
  (void) &pathname;
  (void) &stat_buf;

  print_quoted(pred_ptr->args.printf_vec.stream,
	       pred_ptr->args.printf_vec.quote_opts,
	       pred_ptr->args.printf_vec.dest_is_tty,
	       "%s\n",
	       pathname);
  return true;
}

boolean
pred_fprint0 (const char *pathname, struct stat *stat_buf, struct predicate *pred_ptr)
{
  FILE * fp = pred_ptr->args.printf_vec.stream;

  (void) &stat_buf;

  fputs (pathname, fp);
  putc (0, fp);
  return true;
}



static char*
mode_to_filetype(mode_t m)
{
#define HANDLE_TYPE(t,letter) if (m==t) { return letter; }
#ifdef S_IFREG
  HANDLE_TYPE(S_IFREG,  "f");	/* regular file */
#endif
#ifdef S_IFDIR
  HANDLE_TYPE(S_IFDIR,  "d");	/* directory */
#endif
#ifdef S_IFLNK
  HANDLE_TYPE(S_IFLNK,  "l");	/* symbolic link */
#endif
#ifdef S_IFSOCK
  HANDLE_TYPE(S_IFSOCK, "s");	/* Unix domain socket */
#endif
#ifdef S_IFBLK
  HANDLE_TYPE(S_IFBLK,  "b");	/* block device */
#endif
#ifdef S_IFCHR
  HANDLE_TYPE(S_IFCHR,  "c");	/* character device */
#endif
#ifdef S_IFIFO
  HANDLE_TYPE(S_IFIFO,  "p");	/* FIFO */
#endif
#ifdef S_IFDOOR
  HANDLE_TYPE(S_IFDOOR, "D");	/* Door (e.g. on Solaris) */
#endif
  return "U";			/* Unknown */
}

static double
file_sparseness(const struct stat *p)
{
#if defined HAVE_STRUCT_STAT_ST_BLOCKS
  if (0 == p->st_size)
    {
      if (0 == p->st_blocks)
	return 1.0;
      else
	return p->st_blocks < 0 ? -HUGE_VAL : HUGE_VAL;
    }
  else
    {
      double blklen = file_blocksize(p) * (double)p->st_blocks;
      return blklen / p->st_size;
    }
#else
  return 1.0;
#endif
}



static void
checked_fprintf(struct format_val *dest, const char *fmt, ...)
{
  int rv;
  va_list ap;

  va_start(ap, fmt);
  rv = vfprintf(dest->stream, fmt, ap);
  if (rv < 0)
    nonfatal_file_error(dest->filename);
}


static void
checked_print_quoted (struct format_val *dest,
			   const char *format, const char *s)
{
  int rv = print_quoted(dest->stream, dest->quote_opts, dest->dest_is_tty,
			format, s);
  if (rv < 0)
    nonfatal_file_error(dest->filename);
}


static void
checked_fwrite(void *p, size_t siz, size_t nmemb, struct format_val *dest)
{
  int items_written = fwrite(p, siz, nmemb, dest->stream);
  if (items_written < nmemb)
    nonfatal_file_error(dest->filename);
}

static void
checked_fflush(struct format_val *dest)
{
  if (0 != fflush(dest->stream))
    {
      nonfatal_file_error(dest->filename);
    }
}

static void
do_fprintf(struct format_val *dest,
	   struct segment *segment,
	   const char *pathname,
	   const struct stat *stat_buf)
{
  char hbuf[LONGEST_HUMAN_READABLE + 1];
  const char *cp;

  switch (segment->segkind)
    {
    case KIND_PLAIN:	/* Plain text string (no % conversion). */
      /* trusted */
      checked_fwrite(segment->text, 1, segment->text_len, dest);
      break;

    case KIND_STOP:		/* Terminate argument and flush output. */
      /* trusted */
      checked_fwrite(segment->text, 1, segment->text_len, dest);
      checked_fflush(dest);
      break;

    case KIND_FORMAT:
      switch (segment->format_char[0])
	{
	case 'a':		/* atime in `ctime' format. */
	  /* UNTRUSTED, probably unexploitable */
	  checked_fprintf (dest, segment->text, ctime_format (get_stat_atime(stat_buf)));
	  break;
	case 'b':		/* size in 512-byte blocks */
	  /* UNTRUSTED, probably unexploitable */
	  checked_fprintf (dest, segment->text,
			   human_readable ((uintmax_t) ST_NBLOCKS (*stat_buf),
					   hbuf, human_ceiling,
					   ST_NBLOCKSIZE, 512));
	  break;
	case 'c':		/* ctime in `ctime' format */
	  /* UNTRUSTED, probably unexploitable */
	  checked_fprintf (dest, segment->text, ctime_format (get_stat_ctime(stat_buf)));
	  break;
	case 'd':		/* depth in search tree */
	  /* UNTRUSTED, probably unexploitable */
	  checked_fprintf (dest, segment->text, state.curdepth);
	  break;
	case 'D':		/* Device on which file exists (stat.st_dev) */
	  /* trusted */
	  checked_fprintf (dest, segment->text,
			   human_readable ((uintmax_t) stat_buf->st_dev, hbuf,
					   human_ceiling, 1, 1));
	  break;
	case 'f':		/* base name of path */
	  /* sanitised */
	  {
	    char *base = base_name (pathname);
	    checked_print_quoted (dest, segment->text, base);
	    free (base);
	  }
	  break;
	case 'F':		/* file system type */
	  /* trusted */
	  checked_print_quoted (dest, segment->text, filesystem_type (stat_buf, pathname));
	  break;
	case 'g':		/* group name */
	  /* trusted */
	  /* (well, the actual group is selected by the user but
	   * its name was selected by the system administrator)
	   */
	  {
	    struct group *g;

	    g = getgrgid (stat_buf->st_gid);
	    if (g)
	      {
		segment->text[segment->text_len] = 's';
		checked_fprintf (dest, segment->text, g->gr_name);
		break;
	      }
	    else
	      {
		/* Do nothing. */
		/*FALLTHROUGH*/
	      }
	  }
	  /*FALLTHROUGH*/ /*...sometimes, so 'G' case.*/

	case 'G':		/* GID number */
	  /* UNTRUSTED, probably unexploitable */
	  checked_fprintf (dest, segment->text,
			   human_readable ((uintmax_t) stat_buf->st_gid, hbuf,
					   human_ceiling, 1, 1));
	  break;
	case 'h':		/* leading directories part of path */
	  /* sanitised */
	  {
	    cp = strrchr (pathname, '/');
	    if (cp == NULL)	/* No leading directories. */
	      {
		/* If there is no slash in the pathname, we still
		 * print the string because it contains characters
		 * other than just '%s'.  The %h expands to ".".
		 */
		checked_print_quoted (dest, segment->text, ".");
	      }
	    else
	      {
		char *s = strdup(pathname);
		s[cp - pathname] = 0;
		checked_print_quoted (dest, segment->text, s);
		free(s);
	      }
	  }
	  break;

	case 'H':		/* ARGV element file was found under */
	  /* trusted */
	  {
	    char *s = xmalloc(state.starting_path_length+1);
	    memcpy(s, pathname, state.starting_path_length);
	    s[state.starting_path_length] = 0;
	    checked_fprintf (dest, segment->text, s);
	    free(s);
	  }
	  break;

	case 'i':		/* inode number */
	  /* UNTRUSTED, but not exploitable I think */
	  checked_fprintf (dest, segment->text,
			   human_readable ((uintmax_t) stat_buf->st_ino, hbuf,
					   human_ceiling,
					   1, 1));
	  break;
	case 'k':		/* size in 1K blocks */
	  /* UNTRUSTED, but not exploitable I think */
	  checked_fprintf (dest, segment->text,
			   human_readable ((uintmax_t) ST_NBLOCKS (*stat_buf),
					   hbuf, human_ceiling,
					   ST_NBLOCKSIZE, 1024));
	  break;
	case 'l':		/* object of symlink */
	  /* sanitised */
#ifdef S_ISLNK
	  {
	    char *linkname = 0;

	    if (S_ISLNK (stat_buf->st_mode))
	      {
		linkname = get_link_name_at (pathname, state.cwd_dir_fd, state.rel_pathname);
		if (linkname == 0)
		  state.exit_status = 1;
	      }
	    if (linkname)
	      {
		checked_print_quoted (dest, segment->text, linkname);
		free (linkname);
	      }
	    else
	      {
		/* We still need to honour the field width etc., so this is
		 * not a no-op.
		 */
		checked_print_quoted (dest, segment->text, "");
	      }
	  }
#endif				/* S_ISLNK */
	  break;

	case 'M':		/* mode as 10 chars (eg., "-rwxr-x--x" */
	  /* UNTRUSTED, probably unexploitable */
	  {
	    char modestring[16] ;
	    filemodestring (stat_buf, modestring);
	    modestring[10] = '\0';
	    checked_fprintf (dest, segment->text, modestring);
	  }
	  break;

	case 'm':		/* mode as octal number (perms only) */
	  /* UNTRUSTED, probably unexploitable */
	  {
	    /* Output the mode portably using the traditional numbers,
	       even if the host unwisely uses some other numbering
	       scheme.  But help the compiler in the common case where
	       the host uses the traditional numbering scheme.  */
	    mode_t m = stat_buf->st_mode;
	    boolean traditional_numbering_scheme =
	      (S_ISUID == 04000 && S_ISGID == 02000 && S_ISVTX == 01000
	       && S_IRUSR == 00400 && S_IWUSR == 00200 && S_IXUSR == 00100
	       && S_IRGRP == 00040 && S_IWGRP == 00020 && S_IXGRP == 00010
	       && S_IROTH == 00004 && S_IWOTH == 00002 && S_IXOTH == 00001);
	    checked_fprintf (dest, segment->text,
		     (traditional_numbering_scheme
		      ? m & MODE_ALL
		      : ((m & S_ISUID ? 04000 : 0)
			 | (m & S_ISGID ? 02000 : 0)
			 | (m & S_ISVTX ? 01000 : 0)
			 | (m & S_IRUSR ? 00400 : 0)
			 | (m & S_IWUSR ? 00200 : 0)
			 | (m & S_IXUSR ? 00100 : 0)
			 | (m & S_IRGRP ? 00040 : 0)
			 | (m & S_IWGRP ? 00020 : 0)
			 | (m & S_IXGRP ? 00010 : 0)
			 | (m & S_IROTH ? 00004 : 0)
			 | (m & S_IWOTH ? 00002 : 0)
			 | (m & S_IXOTH ? 00001 : 0))));
	  }
	  break;

	case 'n':		/* number of links */
	  /* UNTRUSTED, probably unexploitable */
	  checked_fprintf (dest, segment->text,
		   human_readable ((uintmax_t) stat_buf->st_nlink,
				   hbuf,
				   human_ceiling,
				   1, 1));
	  break;

	case 'p':		/* pathname */
	  /* sanitised */
	  checked_print_quoted (dest, segment->text, pathname);
	  break;

	case 'P':		/* pathname with ARGV element stripped */
	  /* sanitised */
	  if (state.curdepth > 0)
	    {
	      cp = pathname + state.starting_path_length;
	      if (*cp == '/')
		/* Move past the slash between the ARGV element
		   and the rest of the pathname.  But if the ARGV element
		   ends in a slash, we didn't add another, so we've
		   already skipped past it.  */
		cp++;
	    }
	  else
	    {
	      cp = "";
	    }
	  checked_print_quoted (dest, segment->text, cp);
	  break;

	case 's':		/* size in bytes */
	  /* UNTRUSTED, probably unexploitable */
	  checked_fprintf (dest, segment->text,
		   human_readable ((uintmax_t) stat_buf->st_size,
				   hbuf, human_ceiling, 1, 1));
	  break;

	case 'S':		/* sparseness */
	  /* UNTRUSTED, probably unexploitable */
	  checked_fprintf (dest, segment->text, file_sparseness(stat_buf));;
	  break;

	case 't':		/* mtime in `ctime' format */
	  /* UNTRUSTED, probably unexploitable */
	  checked_fprintf (dest, segment->text,
			   ctime_format (get_stat_mtime(stat_buf)));
	  break;

	case 'u':		/* user name */
	  /* trusted */
	  /* (well, the actual user is selected by the user on systems
	   * where chown is not restricted, but the user name was
	   * selected by the system administrator)
	   */
	  {
	    struct passwd *p;

	    p = getpwuid (stat_buf->st_uid);
	    if (p)
	      {
		segment->text[segment->text_len] = 's';
		checked_fprintf (dest, segment->text, p->pw_name);
		break;
	      }
	    /* else fallthru */
	  }
	  /* FALLTHROUGH*/ /* .. to case U */

	case 'U':		/* UID number */
	  /* UNTRUSTED, probably unexploitable */
	  checked_fprintf (dest, segment->text,
			   human_readable ((uintmax_t) stat_buf->st_uid, hbuf,
					   human_ceiling, 1, 1));
	  break;

	  /* %Y: type of file system entry like `ls -l`:
	   *     (d,-,l,s,p,b,c,n) n=nonexistent(symlink)
	   */
	case 'Y':		/* in case of symlink */
	  /* trusted */
	  {
#ifdef S_ISLNK
	    if (S_ISLNK (stat_buf->st_mode))
	      {
		struct stat sbuf;
		/* If we would normally follow links, do not do so.
		 * If we would normally not follow links, do so.
		 */
		if ((following_links() ? lstat : stat)
		    (state.rel_pathname, &sbuf) != 0)
		  {
		    if ( errno == ENOENT )
		      {
			checked_fprintf (dest, segment->text, "N");
			break;
		      }
		    else if ( errno == ELOOP )
		      {
			checked_fprintf (dest, segment->text, "L");
			break;
		      }
		    else
		      {
			checked_fprintf (dest, segment->text, "?");
			error (0, errno, "%s",
			       safely_quote_err_filename(0, pathname));
			/* exit_status = 1;
			   return ; */
			break;
		      }
		  }
		checked_fprintf (dest, segment->text,
				 mode_to_filetype(sbuf.st_mode & S_IFMT));
	      }
#endif /* S_ISLNK */
	    else
	      {
		checked_fprintf (dest, segment->text,
				 mode_to_filetype(stat_buf->st_mode & S_IFMT));
	      }
	  }
	  break;

	case 'y':
	  /* trusted */
	  {
	    checked_fprintf (dest, segment->text,
			     mode_to_filetype(stat_buf->st_mode & S_IFMT));
	  }
	  break;
	}
      /* end of KIND_FORMAT case */
      break;
    }
}

boolean
pred_fprintf (const char *pathname, struct stat *stat_buf, struct predicate *pred_ptr)
{
  struct format_val *dest = &pred_ptr->args.printf_vec;
  struct segment *segment;

  for (segment = dest->segment; segment; segment = segment->next)
    {
      if ( (KIND_FORMAT == segment->segkind) && segment->format_char[1]) /* Component of date. */
	{
	  struct timespec ts;
	  int valid = 0;

	  switch (segment->format_char[0])
	    {
	    case 'A':
	      ts = get_stat_atime(stat_buf);
	      valid = 1;
	      break;
	    case 'B':
	      ts = get_stat_birthtime(stat_buf);
	      if ('@' == segment->format_char[1])
		valid = 1;
	      else
		valid = (ts.tv_nsec >= 0);
	      break;
	    case 'C':
	      ts = get_stat_ctime(stat_buf);
	      valid = 1;
	      break;
	    case 'T':
	      ts = get_stat_mtime(stat_buf);
	      valid = 1;
	      break;
	    default:
	      assert (0);
	      abort ();
	    }
	  /* We trust the output of format_date not to contain
	   * nasty characters, though the value of the date
	   * is itself untrusted data.
	   */
	  if (valid)
	    {
	      /* trusted */
	      checked_fprintf (dest, segment->text,
			       format_date (ts, segment->format_char[1]));
	    }
	  else
	    {
	      /* The specified timestamp is not available, output
	       * nothing for the timestamp, but use the rest (so that
	       * for example find foo -printf '[%Bs] %p\n' can print
	       * "[] foo").
	       */
	      /* trusted */
	      checked_fprintf (dest, segment->text, "");
	    }
	}
      else
	{
	  /* Print a segment which is not a date. */
	  do_fprintf(dest, segment, pathname, stat_buf);
	}
    }
  return true;
}

boolean
pred_fstype (const char *pathname, struct stat *stat_buf, struct predicate *pred_ptr)
{
  (void) pathname;

  if (strcmp (filesystem_type (stat_buf, pathname), pred_ptr->args.str) == 0)
    return true;
  else
    return false;
}

boolean
pred_gid (const char *pathname, struct stat *stat_buf, struct predicate *pred_ptr)
{
  (void) pathname;

  switch (pred_ptr->args.numinfo.kind)
    {
    case COMP_GT:
      if (stat_buf->st_gid > pred_ptr->args.numinfo.l_val)
	return (true);
      break;
    case COMP_LT:
      if (stat_buf->st_gid < pred_ptr->args.numinfo.l_val)
	return (true);
      break;
    case COMP_EQ:
      if (stat_buf->st_gid == pred_ptr->args.numinfo.l_val)
	return (true);
      break;
    }
  return (false);
}

boolean
pred_group (const char *pathname, struct stat *stat_buf, struct predicate *pred_ptr)
{
  (void) pathname;

  if (pred_ptr->args.gid == stat_buf->st_gid)
    return (true);
  else
    return (false);
}

boolean
pred_ilname (const char *pathname, struct stat *stat_buf, struct predicate *pred_ptr)
{
  return match_lname (pathname, stat_buf, pred_ptr, true);
}

/* Common code between -name, -iname.  PATHNAME is being visited, STR
   is name to compare basename against, and FLAGS are passed to
   fnmatch.  Recall that 'find / -name /' is one of the few times where a '/'
   in the -name must actually find something. */
static boolean
pred_name_common (const char *pathname, const char *str, int flags)
{
  boolean b;
  /* We used to use last_component() here, but that would not allow us to modify the
   * input string, which is const.   We could optimise by duplicating the string only
   * if we need to modify it, and I'll do that if there is a measurable
   * performance difference on a machine built after 1990...
   */
  char *base = base_name (pathname);
  /* remove trailing slashes, but leave  "/" or "//foo" unchanged. */
  strip_trailing_slashes(base);

  /* FNM_PERIOD is not used here because POSIX requires that it not be.
   * See http://standards.ieee.org/reading/ieee/interp/1003-2-92_int/pasc-1003.2-126.html
   */
  b = fnmatch (str, base, flags) == 0;
  free (base);
  return b;
}

boolean
pred_iname (const char *pathname, struct stat *stat_buf, struct predicate *pred_ptr)
{
  (void) stat_buf;
  return pred_name_common (pathname, pred_ptr->args.str, FNM_CASEFOLD);
}

boolean
pred_inum (const char *pathname, struct stat *stat_buf, struct predicate *pred_ptr)
{
  (void) pathname;

  switch (pred_ptr->args.numinfo.kind)
    {
    case COMP_GT:
      if (stat_buf->st_ino > pred_ptr->args.numinfo.l_val)
	return (true);
      break;
    case COMP_LT:
      if (stat_buf->st_ino < pred_ptr->args.numinfo.l_val)
	return (true);
      break;
    case COMP_EQ:
      if (stat_buf->st_ino == pred_ptr->args.numinfo.l_val)
	return (true);
      break;
    }
  return (false);
}

boolean
pred_ipath (const char *pathname, struct stat *stat_buf, struct predicate *pred_ptr)
{
  (void) stat_buf;

  if (fnmatch (pred_ptr->args.str, pathname, FNM_CASEFOLD) == 0)
    return (true);
  return (false);
}

boolean
pred_links (const char *pathname, struct stat *stat_buf, struct predicate *pred_ptr)
{
  (void) pathname;

  switch (pred_ptr->args.numinfo.kind)
    {
    case COMP_GT:
      if (stat_buf->st_nlink > pred_ptr->args.numinfo.l_val)
	return (true);
      break;
    case COMP_LT:
      if (stat_buf->st_nlink < pred_ptr->args.numinfo.l_val)
	return (true);
      break;
    case COMP_EQ:
      if (stat_buf->st_nlink == pred_ptr->args.numinfo.l_val)
	return (true);
      break;
    }
  return (false);
}

boolean
pred_lname (const char *pathname, struct stat *stat_buf, struct predicate *pred_ptr)
{
  return match_lname (pathname, stat_buf, pred_ptr, false);
}

static boolean
match_lname (const char *pathname, struct stat *stat_buf, struct predicate *pred_ptr, boolean ignore_case)
{
  boolean ret = false;
#ifdef S_ISLNK
  if (S_ISLNK (stat_buf->st_mode))
    {
      char *linkname = get_link_name_at (pathname, state.cwd_dir_fd, state.rel_pathname);
      if (linkname)
	{
	  if (fnmatch (pred_ptr->args.str, linkname,
		       ignore_case ? FNM_CASEFOLD : 0) == 0)
	    ret = true;
	  free (linkname);
	}
    }
#endif /* S_ISLNK */
  return ret;
}

boolean
pred_ls (const char *pathname, struct stat *stat_buf, struct predicate *pred_ptr)
{
  return pred_fls(pathname, stat_buf, pred_ptr);
}

boolean
pred_mmin (const char *pathname, struct stat *stat_buf, struct predicate *pred_ptr)
{
  (void) &pathname;
  return pred_timewindow(get_stat_mtime(stat_buf), pred_ptr, 60);
}

boolean
pred_mtime (const char *pathname, struct stat *stat_buf, struct predicate *pred_ptr)
{
  (void) pathname;
  return pred_timewindow(get_stat_mtime(stat_buf), pred_ptr, DAYSECS);
}

boolean
pred_name (const char *pathname, struct stat *stat_buf, struct predicate *pred_ptr)
{
  (void) stat_buf;
  return pred_name_common (pathname, pred_ptr->args.str, 0);
}

boolean
pred_negate (const char *pathname, struct stat *stat_buf, struct predicate *pred_ptr)
{
  return !apply_predicate(pathname, stat_buf, pred_ptr->pred_right);
}

boolean
pred_newer (const char *pathname, struct stat *stat_buf, struct predicate *pred_ptr)
{
  (void) pathname;

  assert (COMP_GT == pred_ptr->args.reftime.kind);
  return compare_ts(get_stat_mtime(stat_buf), pred_ptr->args.reftime.ts) > 0;
}

boolean
pred_newerXY (const char *pathname, struct stat *stat_buf, struct predicate *pred_ptr)
{
  struct timespec ts;
  boolean collected = false;

  assert (COMP_GT == pred_ptr->args.reftime.kind);

  switch (pred_ptr->args.reftime.xval)
    {
    case XVAL_TIME:
      assert (pred_ptr->args.reftime.xval != XVAL_TIME);
      return false;

    case XVAL_ATIME:
      ts = get_stat_atime(stat_buf);
      collected = true;
      break;

    case XVAL_BIRTHTIME:
      ts = get_stat_birthtime(stat_buf);
      collected = true;
      if (ts.tv_nsec < 0);
	{
	  /* XXX: Cannot determine birth time.  Warn once. */
	  error(0, 0, _("Warning: cannot determine birth time of file %s"),
		safely_quote_err_filename(0, pathname));
	  return false;
	}
      break;

    case XVAL_CTIME:
      ts = get_stat_ctime(stat_buf);
      collected = true;
      break;

    case XVAL_MTIME:
      ts = get_stat_mtime(stat_buf);
      collected = true;
      break;
    }

  assert (collected);
  return compare_ts(ts, pred_ptr->args.reftime.ts) > 0;
}

boolean
pred_nogroup (const char *pathname, struct stat *stat_buf, struct predicate *pred_ptr)
{
  (void) pathname;
  (void) pred_ptr;

#ifdef CACHE_IDS
  extern char *gid_unused;

  return gid_unused[(unsigned) stat_buf->st_gid];
#else
  return getgrgid (stat_buf->st_gid) == NULL;
#endif
}

boolean
pred_nouser (const char *pathname, struct stat *stat_buf, struct predicate *pred_ptr)
{
#ifdef CACHE_IDS
  extern char *uid_unused;
#endif

  (void) pathname;
  (void) pred_ptr;

#ifdef CACHE_IDS
  return uid_unused[(unsigned) stat_buf->st_uid];
#else
  return getpwuid (stat_buf->st_uid) == NULL;
#endif
}


static boolean
is_ok(const char *program, const char *arg)
{
  fflush (stdout);
  /* The draft open standard requires that, in the POSIX locale,
     the last non-blank character of this prompt be '?'.
     The exact format is not specified.
     This standard does not have requirements for locales other than POSIX
  */
  /* XXX: printing UNTRUSTED data here. */
  fprintf (stderr, _("< %s ... %s > ? "), program, arg);
  fflush (stderr);
  return yesno();
}

boolean
pred_ok (const char *pathname, struct stat *stat_buf, struct predicate *pred_ptr)
{
  if (is_ok(pred_ptr->args.exec_vec.replace_vec[0], pathname))
    return new_impl_pred_exec (get_start_dirfd(),
			       pathname, stat_buf, pred_ptr, NULL, 0);
  else
    return false;
}

boolean
pred_okdir (const char *pathname, struct stat *stat_buf, struct predicate *pred_ptr)
{
  const char *prefix = (state.rel_pathname[0] == '/') ? NULL : "./";
  if (is_ok(pred_ptr->args.exec_vec.replace_vec[0], pathname))
    return new_impl_pred_exec (get_current_dirfd(),
			       state.rel_pathname, stat_buf, pred_ptr,
			       prefix, (prefix ? 2 : 0));
  else
    return false;
}

boolean
pred_openparen (const char *pathname, struct stat *stat_buf, struct predicate *pred_ptr)
{
  (void) pathname;
  (void) stat_buf;
  (void) pred_ptr;
  return true;
}

boolean
pred_or (const char *pathname, struct stat *stat_buf, struct predicate *pred_ptr)
{
  if (pred_ptr->pred_left == NULL
      || !apply_predicate(pathname, stat_buf, pred_ptr->pred_left))
    {
      return apply_predicate(pathname, stat_buf, pred_ptr->pred_right);
    }
  else
    return true;
}

boolean
pred_path (const char *pathname, struct stat *stat_buf, struct predicate *pred_ptr)
{
  (void) stat_buf;
  if (fnmatch (pred_ptr->args.str, pathname, 0) == 0)
    return (true);
  return (false);
}

boolean
pred_perm (const char *pathname, struct stat *stat_buf, struct predicate *pred_ptr)
{
  mode_t mode = stat_buf->st_mode;
  mode_t perm_val = pred_ptr->args.perm.val[S_ISDIR (mode) != 0];
  (void) pathname;
  switch (pred_ptr->args.perm.kind)
    {
    case PERM_AT_LEAST:
      return (mode & perm_val) == perm_val;
      break;

    case PERM_ANY:
      /* True if any of the bits set in the mask are also set in the file's mode.
       *
       *
       * Otherwise, if onum is prefixed by a hyphen, the primary shall
       * evaluate as true if at least all of the bits specified in
       * onum that are also set in the octal mask 07777 are set.
       *
       * Eric Blake's interpretation is that the mode argument is zero,

       */
      if (0 == perm_val)
	return true;		/* Savannah bug 14748; we used to return false */
      else
	return (mode & perm_val) != 0;
      break;

    case PERM_EXACT:
      return (mode & MODE_ALL) == perm_val;
      break;

    default:
      abort ();
      break;
    }
}


struct access_check_args
{
  const char *filename;
  int access_type;
  int cb_errno;
};


static int
access_callback(void *context)
{
  int rv;
  struct access_check_args *args = context;
  if ((rv = access(args->filename, args->access_type)) < 0)
    args->cb_errno = errno;
  return rv;
}

static int
can_access(int access_type)
{
  struct access_check_args args;
  args.filename = state.rel_pathname;
  args.access_type = access_type;
  args.cb_errno = 0;
  return 0 == run_in_dir(state.cwd_dir_fd, access_callback, &args);
}


boolean
pred_executable (const char *pathname, struct stat *stat_buf, struct predicate *pred_ptr)
{
  (void) pathname;
  (void) stat_buf;
  (void) pred_ptr;

  return can_access(X_OK);
}

boolean
pred_readable (const char *pathname, struct stat *stat_buf, struct predicate *pred_ptr)
{
  (void) pathname;
  (void) stat_buf;
  (void) pred_ptr;

  return can_access(R_OK);
}

boolean
pred_writable (const char *pathname, struct stat *stat_buf, struct predicate *pred_ptr)
{
  (void) pathname;
  (void) stat_buf;
  (void) pred_ptr;

  return can_access(W_OK);
}

boolean
pred_print (const char *pathname, struct stat *stat_buf, struct predicate *pred_ptr)
{
  (void) stat_buf;
  (void) pred_ptr;

  print_quoted(pred_ptr->args.printf_vec.stream,
	       pred_ptr->args.printf_vec.quote_opts,
	       pred_ptr->args.printf_vec.dest_is_tty,
	       "%s\n", pathname);
  return true;
}

boolean
pred_print0 (const char *pathname, struct stat *stat_buf, struct predicate *pred_ptr)
{
  return pred_fprint0(pathname, stat_buf, pred_ptr);
}

boolean
pred_prune (const char *pathname, struct stat *stat_buf, struct predicate *pred_ptr)
{
  (void) pathname;
  (void) pred_ptr;

  if (options.do_dir_first == true) { /* no effect with -depth */
    assert (state.have_stat);
    if (stat_buf != NULL &&
	S_ISDIR(stat_buf->st_mode))
      state.stop_at_current_level = true;
  }

  /* findutils used to return options.do_dir_first here, so that -prune
   * returns true only if -depth is not in effect.   But POSIX requires
   * that -prune always evaluate as true.
   */
  return true;
}

boolean
pred_quit (const char *pathname, struct stat *stat_buf, struct predicate *pred_ptr)
{
  (void) pathname;
  (void) stat_buf;
  (void) pred_ptr;

  /* Run any cleanups.  This includes executing any command lines
   * we have partly built but not executed.
   */
  cleanup();

  /* Since -exec and friends don't leave child processes running in the
   * background, there is no need to wait for them here.
   */
  exit(state.exit_status);	/* 0 for success, etc. */
}

boolean
pred_regex (const char *pathname, struct stat *stat_buf, struct predicate *pred_ptr)
{
  int len = strlen (pathname);
(void) stat_buf;
  if (re_match (pred_ptr->args.regex, pathname, len, 0,
		(struct re_registers *) NULL) == len)
    return (true);
  return (false);
}

boolean
pred_size (const char *pathname, struct stat *stat_buf, struct predicate *pred_ptr)
{
  uintmax_t f_val;

  (void) pathname;
  f_val = ((stat_buf->st_size / pred_ptr->args.size.blocksize)
	   + (stat_buf->st_size % pred_ptr->args.size.blocksize != 0));
  switch (pred_ptr->args.size.kind)
    {
    case COMP_GT:
      if (f_val > pred_ptr->args.size.size)
	return (true);
      break;
    case COMP_LT:
      if (f_val < pred_ptr->args.size.size)
	return (true);
      break;
    case COMP_EQ:
      if (f_val == pred_ptr->args.size.size)
	return (true);
      break;
    }
  return (false);
}

boolean
pred_samefile (const char *pathname, struct stat *stat_buf, struct predicate *pred_ptr)
{
  /* Potential optimisation: because of the loop protection, we always
   * know the device of the current directory, hence the device number
   * of the file we're currently considering.  If -L is not in effect,
   * and the device number of the file we're looking for is not the
   * same as the device number of the current directory, this
   * predicate cannot return true.  Hence there would be no need to
   * stat the file we're looking at.
   */
  (void) pathname;

  /* We will often still have an fd open on the file under consideration,
   * but that's just to ensure inode number stability by maintaining
   * a reference to it; we don't need the file for anything else.
   */
  return stat_buf->st_ino == pred_ptr->args.samefileid.ino
    &&   stat_buf->st_dev == pred_ptr->args.samefileid.dev;
}

boolean
pred_true (const char *pathname, struct stat *stat_buf, struct predicate *pred_ptr)
{
  (void) pathname;
  (void) stat_buf;
  (void) pred_ptr;
  return true;
}

boolean
pred_type (const char *pathname, struct stat *stat_buf, struct predicate *pred_ptr)
{
  mode_t mode;
  mode_t type = pred_ptr->args.type;

  assert (state.have_type);

  if (0 == state.type)
    {
      /* This can sometimes happen with broken NFS servers.
       * See Savannah bug #16378.
       */
      return false;
    }

  (void) pathname;

  if (state.have_stat)
     mode = stat_buf->st_mode;
  else
     mode = state.type;

#ifndef S_IFMT
  /* POSIX system; check `mode' the slow way. */
  if ((S_ISBLK (mode) && type == S_IFBLK)
      || (S_ISCHR (mode) && type == S_IFCHR)
      || (S_ISDIR (mode) && type == S_IFDIR)
      || (S_ISREG (mode) && type == S_IFREG)
#ifdef S_IFLNK
      || (S_ISLNK (mode) && type == S_IFLNK)
#endif
#ifdef S_IFIFO
      || (S_ISFIFO (mode) && type == S_IFIFO)
#endif
#ifdef S_IFSOCK
      || (S_ISSOCK (mode) && type == S_IFSOCK)
#endif
#ifdef S_IFDOOR
      || (S_ISDOOR (mode) && type == S_IFDOOR)
#endif
      )
#else /* S_IFMT */
  /* Unix system; check `mode' the fast way. */
  if ((mode & S_IFMT) == type)
#endif /* S_IFMT */
    return (true);
  else
    return (false);
}

boolean
pred_uid (const char *pathname, struct stat *stat_buf, struct predicate *pred_ptr)
{
  (void) pathname;
  switch (pred_ptr->args.numinfo.kind)
    {
    case COMP_GT:
      if (stat_buf->st_uid > pred_ptr->args.numinfo.l_val)
	return (true);
      break;
    case COMP_LT:
      if (stat_buf->st_uid < pred_ptr->args.numinfo.l_val)
	return (true);
      break;
    case COMP_EQ:
      if (stat_buf->st_uid == pred_ptr->args.numinfo.l_val)
	return (true);
      break;
    }
  return (false);
}

boolean
pred_used (const char *pathname, struct stat *stat_buf, struct predicate *pred_ptr)
{
  struct timespec delta, at, ct;

  (void) pathname;

  /* TODO: this needs to be retested carefully (manually, if necessary) */
  at = get_stat_atime(stat_buf);
  ct = get_stat_ctime(stat_buf);
  delta.tv_sec  = at.tv_sec  - ct.tv_sec;
  delta.tv_nsec = at.tv_nsec - ct.tv_nsec;
  if (delta.tv_nsec < 0)
    {
      delta.tv_nsec += 1000000000;
      delta.tv_sec  -=          1;
    }
  return pred_timewindow(delta, pred_ptr, DAYSECS);
}

boolean
pred_user (const char *pathname, struct stat *stat_buf, struct predicate *pred_ptr)
{
  (void) pathname;
  if (pred_ptr->args.uid == stat_buf->st_uid)
    return (true);
  else
    return (false);
}

boolean
pred_xtype (const char *pathname, struct stat *stat_buf, struct predicate *pred_ptr)
{
  struct stat sbuf;		/* local copy, not stat_buf because we're using a different stat method */
  int (*ystat) (const char*, struct stat *p);

  /* If we would normally stat the link itself, stat the target instead.
   * If we would normally follow the link, stat the link itself instead.
   */
  if (following_links())
    ystat = optionp_stat;
  else
    ystat = optionl_stat;

  set_stat_placeholders(&sbuf);
  if ((*ystat) (state.rel_pathname, &sbuf) != 0)
    {
      if (following_links() && errno == ENOENT)
	{
	  /* If we failed to follow the symlink,
	   * fall back on looking at the symlink itself.
	   */
	  /* Mimic behavior of ls -lL. */
	  return (pred_type (pathname, stat_buf, pred_ptr));
	}
      else
	{
	  error (0, errno, "%s", safely_quote_err_filename(0, pathname));
	  state.exit_status = 1;
	}
      return false;
    }
  /* Now that we have our stat() information, query it in the same
   * way that -type does.
   */
  return (pred_type (pathname, &sbuf, pred_ptr));
}

/*  1) fork to get a child; parent remembers the child pid
    2) child execs the command requested
    3) parent waits for child; checks for proper pid of child

    Possible returns:

    ret		errno	status(h)   status(l)

    pid		x	signal#	    0177	stopped
    pid		x	exit arg    0		term by _exit
    pid		x	0	    signal #	term by signal
    -1		EINTR				parent got signal
    -1		other				some other kind of error

    Return true only if the pid matches, status(l) is
    zero, and the exit arg (status high) is 0.
    Otherwise return false, possibly printing an error message. */


static boolean
prep_child_for_exec (boolean close_stdin, int dir_fd)
{
  boolean ok = true;
  if (close_stdin)
    {
      const char inputfile[] = "/dev/null";

      if (close(0) < 0)
	{
	  error(0, errno, _("Cannot close standard input"));
	  ok = false;
	}
      else
	{
	  if (open(inputfile, O_RDONLY
#if defined O_LARGEFILE
		   |O_LARGEFILE
#endif
		   ) < 0)
	    {
	      /* This is not entirely fatal, since
	       * executing the child with a closed
	       * stdin is almost as good as executing it
	       * with its stdin attached to /dev/null.
	       */
	      error (0, errno, "%s", safely_quote_err_filename(0, inputfile));
	      /* do not set ok=false, it is OK to continue anyway. */
	    }
	}
    }

  /* Even if DebugSearch is set, don't announce our change of
   * directory, since we're not going to emit a subsequent
   * announcement of a call to stat() anyway, as we're about to exec
   * something.
   */
  if (dir_fd != AT_FDCWD)
    {
      assert (dir_fd >= 0);
      if (0 != fchdir(dir_fd))
	{
	  /* If we cannot execute our command in the correct directory,
	   * we should not execute it at all.
	   */
	  error(0, errno, _("Failed to change directory"));
	  ok = false;
	}
    }
  return ok;
}



int
launch (const struct buildcmd_control *ctl,
	struct buildcmd_state *buildstate)
{
  int wait_status;
  pid_t child_pid;
  static int first_time = 1;
  const struct exec_val *execp = buildstate->usercontext;

  if (!execp->use_current_dir)
    {
      assert (starting_desc >= 0);
      assert (execp->dir_fd == starting_desc);
    }


  /* Null terminate the arg list.  */
  bc_push_arg (ctl, buildstate, (char *) NULL, 0, NULL, 0, false);

  /* Make sure output of command doesn't get mixed with find output. */
  fflush (stdout);
  fflush (stderr);

  /* Make sure to listen for the kids.  */
  if (first_time)
    {
      first_time = 0;
      signal (SIGCHLD, SIG_DFL);
    }

  child_pid = fork ();
  if (child_pid == -1)
    error (1, errno, _("cannot fork"));
  if (child_pid == 0)
    {
      /* We are the child. */
      assert (starting_desc >= 0);
      if (!prep_child_for_exec(execp->close_stdin, execp->dir_fd))
	{
	  _exit(1);
	}

      execvp (buildstate->cmd_argv[0], buildstate->cmd_argv);
      error (0, errno, "%s",
	     safely_quote_err_filename(0, buildstate->cmd_argv[0]));
      _exit (1);
    }


  /* In parent; set up for next time. */
  bc_clear_args(ctl, buildstate);


  while (waitpid (child_pid, &wait_status, 0) == (pid_t) -1)
    {
      if (errno != EINTR)
	{
	  error (0, errno, _("error waiting for %s"),
		 safely_quote_err_filename(0, buildstate->cmd_argv[0]));
	  state.exit_status = 1;
	  return 0;		/* FAIL */
	}
    }

  if (WIFSIGNALED (wait_status))
    {
      error (0, 0, _("%s terminated by signal %d"),
	     quotearg_n_style(0, options.err_quoting_style,
			      buildstate->cmd_argv[0]),
	     WTERMSIG (wait_status));

      if (execp->multiple)
	{
	  /* -exec   \; just returns false if the invoked command fails.
	   * -exec {} + returns true if the invoked command fails, but
	   *            sets the program exit status.
	   */
	  state.exit_status = 1;
	}

      return 1;			/* OK */
    }

  if (0 == WEXITSTATUS (wait_status))
    {
      return 1;			/* OK */
    }
  else
    {
      if (execp->multiple)
	{
	  /* -exec   \; just returns false if the invoked command fails.
	   * -exec {} + returns true if the invoked command fails, but
	   *            sets the program exit status.
	   */
	  state.exit_status = 1;
	}
      return 0;			/* FAIL */
    }

}


static boolean
scan_for_digit_differences(const char *p, const char *q,
			   size_t *first, size_t *n)
{
  bool seen = false;
  size_t i;

  for (i=0; p[i] && q[i]; i++)
    {
      if (p[i] != q[i])
	{
	  if (!isdigit((unsigned char)q[i]) || !isdigit ((unsigned char)q[i]))
	    return false;

	  if (!seen)
	    {
	      *first = i;
	      *n = 1;
	      seen = 1;
	    }
	  else
	    {
	      if (i-*first == *n)
		{
		  /* Still in the first sequence of differing digits. */
		  ++*n;
		}
	      else
		{
		  /* More than one differing contiguous character sequence. */
		  return false;
		}
	    }
	}
    }
  if (p[i] || q[i])
    {
      /* strings are different lengths. */
      return false;
    }
  return true;
}


static char*
do_time_format (const char *fmt, const struct tm *p, const char *ns, size_t ns_size)
{
  static char *buf = NULL;
  static size_t buf_size;
  char *timefmt = NULL;
  struct tm altered_time;


  /* If the format expands to nothing (%p in some locales, for
   * example), strftime can return 0.  We actually want to distinguish
   * the error case where the buffer is too short, so we just prepend
   * an otherwise uninteresting character to prevent the no-output
   * case.
   */
  timefmt = xmalloc (strlen(fmt) + 2u);
  sprintf (timefmt, "_%s", fmt);

  /* altered_time is a similar time, but in which both
   * digits of the seconds field are different.
   */
  altered_time = *p;
  if (altered_time.tm_sec >= 11)
    altered_time.tm_sec -= 11;
  else
    altered_time.tm_sec += 11;

  /* If we call strftime() with buf_size=0, the program will coredump
   * on Solaris, since it unconditionally writes the terminating null
   * character.
   */
  buf_size = 1u;
  buf = xmalloc (buf_size);
  while (true)
    {
      /* I'm not sure that Solaris will return 0 when the buffer is too small.
       * Therefore we do not check for (buf_used != 0) as the termination
       * condition.
       */
      size_t buf_used = strftime (buf, buf_size, timefmt, p);
      if (buf_used		/* Conforming POSIX system */
	  && (buf_used < buf_size)) /* Solaris workaround */
	{
	  char *altbuf;
	  size_t i = 0, n = 0;
	  size_t final_len = (buf_used
			      + 1u /* for \0 */
			      + ns_size);
	  buf = xrealloc (buf, final_len);
	  altbuf = xmalloc (final_len);
	  strftime (altbuf, buf_size, timefmt, &altered_time);

	  /* Find the seconds digits; they should be the only changed part.
	   * In theory the result of the two formatting operations could differ in
	   * more than just one sequence of decimal digits (for example %X might
	   * in theory return a spelled-out time like "thirty seconds past noon").
	   * When that happens, we just avoid inserting the nanoseconds field.
	   */
	  if (scan_for_digit_differences (buf, altbuf, &i, &n)
	      && (2==n) && !isdigit((unsigned char)buf[i+n]))
	    {
	      const size_t end_of_seconds = i + n;
	      const size_t suffix_len = buf_used-(end_of_seconds)+1;

	      /* Move the tail (including the \0).  Note that this
	       * is a move of an overlapping memory block, so we
	       * must use memmove instead of memcpy.  Then insert
	       * the nanoseconds (but not its trailing \0).
	       */
	      assert (end_of_seconds + ns_size + suffix_len == final_len);
	      memmove (buf+end_of_seconds+ns_size,
		       buf+end_of_seconds,
		       suffix_len);
	      memcpy (buf+i+n, ns, ns_size);
	    }
	  else
	    {
	      /* No seconds digits.  No need to insert anything. */
	    }
	  /* The first character of buf is the underscore, which we actually
	   * don't want.
	   */
	  free (timefmt);
	  return buf+1;
	}
      else
	{
	  buf = x2nrealloc (buf, &buf_size, 2u);
	}
    }
}



/* Return a static string formatting the time WHEN according to the
 * strftime format character KIND.
 *
 * This function contains a number of assertions.  These look like
 * runtime checks of the results of computations, which would be a
 * problem since external events should not be tested for with
 * "assert" (instead you should use "if").  However, they are not
 * really runtime checks.  The assertions actually exist to verify
 * that the various buffers are correctly sized.
 */
static char *
format_date (struct timespec ts, int kind)
{
  /* In theory, we use an extra 10 characters for 9 digits of
   * nanoseconds and 1 for the decimal point.  However, the real
   * world is more complex than that.
   *
   * For example, some systems return junk in the tv_nsec part of
   * st_birthtime.  An example of this is the NetBSD-4.0-RELENG kernel
   * (at Sat Mar 24 18:46:46 2007) running a NetBSD-3.1-RELEASE
   * runtime and examining files on an msdos filesytem.  So for that
   * reason we set NS_BUF_LEN to 32, which is simply "long enough" as
   * opposed to "exactly the right size".  Note that the behaviour of
   * NetBSD appears to be a result of the use of uninitialised data,
   * as it's not 100% reproducible (more like 25%).
   */
  enum {
    NS_BUF_LEN = 32,
    DATE_LEN_PERCENT_APLUS=21	/* length of result of %A+ (it's longer than %c)*/
  };
  static char buf[128u+10u + MAX(DATE_LEN_PERCENT_APLUS,
			    MAX (LONGEST_HUMAN_READABLE + 2, NS_BUF_LEN+64+200))];
  char ns_buf[NS_BUF_LEN]; /* -.9999999990 (- sign can happen!)*/
  int  charsprinted, need_ns_suffix;
  struct tm *tm;
  char fmt[6];

  /* human_readable() assumes we pass a buffer which is at least as
   * long as LONGEST_HUMAN_READABLE.  We use an assertion here to
   * ensure that no nasty unsigned overflow happend in our calculation
   * of the size of buf.  Do the assertion here rather than in the
   * code for %@ so that we find the problem quickly if it exists.  If
   * you want to submit a patch to move this into the if statement, go
   * ahead, I'll apply it.  But include performance timings
   * demonstrating that the performance difference is actually
   * measurable.
   */
  verify (sizeof(buf) >= LONGEST_HUMAN_READABLE);

  charsprinted = 0;
  need_ns_suffix = 0;

  /* Format the main part of the time. */
  if (kind == '+')
    {
      strcpy (fmt, "%F+%T");
      need_ns_suffix = 1;
    }
  else
    {
      fmt[0] = '%';
      fmt[1] = kind;
      fmt[2] = '\0';

      /* %a, %c, and %t are handled in ctime_format() */
      switch (kind)
	{
	case 'S':
	case 'T':
	case 'X':
	case '@':
	  need_ns_suffix = 1;
	  break;
	default:
	  need_ns_suffix = 0;
	  break;
	}
    }

  if (need_ns_suffix)
    {
      /* Format the nanoseconds part.  Leave a trailing zero to
       * discourage people from writing scripts which extract the
       * fractional part of the timestamp by using column offsets.
       * The reason for discouraging this is that in the future, the
       * granularity may not be nanoseconds.
       */
      charsprinted = snprintf(ns_buf, NS_BUF_LEN, ".%09ld0", (long int)ts.tv_nsec);
      assert (charsprinted < NS_BUF_LEN);
    }
  else
    {
      charsprinted = 0;
      ns_buf[0] = 0;
    }

  if (kind != '@')
    {
      tm = localtime (&ts.tv_sec);
      if (tm)
	{
	  char *s = do_time_format (fmt, tm, ns_buf, charsprinted);
	  if (s)
	    return s;
	}
    }

  /* If we get to here, either the format was %@, or we have fallen back to it
   * because strftime failed.
   */
  if (1)
    {
      uintmax_t w = ts.tv_sec;
      size_t used, len, remaining;

      /* XXX: note that we are negating an unsigned type which is the
       * widest possible unsigned type.
       */
      char *p = human_readable (ts.tv_sec < 0 ? -w : w, buf + 1,
				human_ceiling, 1, 1);
      assert (p > buf);
      assert (p < (buf + (sizeof buf)));
      if (ts.tv_sec < 0)
	*--p = '-'; /* XXX: Ugh, relying on internal details of human_readable(). */

      /* Add the nanoseconds part.  Because we cannot enforce a
       * particlar implementation of human_readable, we cannot assume
       * any particular value for (p-buf).  So we need to be careful
       * that there is enough space remaining in the buffer.
       */
      if (need_ns_suffix)
	{
	  len = strlen(p);
	  used = (p-buf) + len;	/* Offset into buf of current end */
	  assert (sizeof buf > used); /* Ensure we can perform subtraction safely. */
	  remaining = sizeof buf - used - 1u; /* allow space for NUL */

	  if (strlen(ns_buf) >= remaining)
	    {
	      error(0, 0,
		    "charsprinted=%ld but remaining=%lu: ns_buf=%s",
		    (long)charsprinted, (unsigned long)remaining, ns_buf);
	    }
	  assert (strlen(ns_buf) < remaining);
	  strcat(p, ns_buf);
	}
      return p;
    }
}

static const char *weekdays[] =
  {
    "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"
  };
static char * months[] =
  {
    "Jan", "Feb", "Mar", "Apr", "May", "Jun",
    "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
  };


static char *
ctime_format (struct timespec ts)
{
  const struct tm * ptm;
#define TIME_BUF_LEN 1024u
  static char resultbuf[TIME_BUF_LEN];
  int nout;

  ptm = localtime(&ts.tv_sec);
  if (ptm)
    {
      assert (ptm->tm_wday >=  0);
      assert (ptm->tm_wday <   7);
      assert (ptm->tm_mon  >=  0);
      assert (ptm->tm_mon  <  12);
      assert (ptm->tm_hour >=  0);
      assert (ptm->tm_hour <  24);
      assert (ptm->tm_min  <  60);
      assert (ptm->tm_sec  <= 61); /* allows 2 leap seconds. */

      /* wkday mon mday hh:mm:ss.nnnnnnnnn yyyy */
      nout = snprintf(resultbuf, TIME_BUF_LEN,
		      "%3s %3s %2d %02d:%02d:%02d.%010ld %04d",
		      weekdays[ptm->tm_wday],
		      months[ptm->tm_mon],
		      ptm->tm_mday,
		      ptm->tm_hour,
		      ptm->tm_min,
		      ptm->tm_sec,
		      (long int)ts.tv_nsec,
		      1900 + ptm->tm_year);

      assert (nout < TIME_BUF_LEN);
      return resultbuf;
    }
  else
    {
      /* The time cannot be represented as a struct tm.
	 Output it as an integer.  */
      return format_date (ts, '@');
    }
}

/* Copy STR into BUF and trim blanks from the end of BUF.
   Return BUF. */

static char *
blank_rtrim (str, buf)
     char *str;
     char *buf;
{
  int i;

  if (str == NULL)
    return (NULL);
  strcpy (buf, str);
  i = strlen (buf) - 1;
  while ((i >= 0) && ((buf[i] == ' ') || buf[i] == '\t'))
    i--;
  buf[++i] = '\0';
  return (buf);
}

/* Print out the predicate list starting at NODE. */
void
print_list (FILE *fp, struct predicate *node)
{
  struct predicate *cur;
  char name[256];

  cur = node;
  while (cur != NULL)
    {
      fprintf (fp, "[%s] ", blank_rtrim (cur->p_name, name));
      cur = cur->pred_next;
    }
  fprintf (fp, "\n");
}

/* Print out the predicate list starting at NODE. */
static void
print_parenthesised(FILE *fp, struct predicate *node)
{
  int parens = 0;

  if (node)
    {
      if ((pred_is(node, pred_or) || pred_is(node, pred_and))
	  && node->pred_left == NULL)
	{
	  /* We print "<nothing> or  X" as just "X"
	   * We print "<nothing> and X" as just "X"
	   */
	  print_parenthesised(fp, node->pred_right);
	}
      else
	{
	  if (node->pred_left || node->pred_right)
	    parens = 1;

	  if (parens)
	    fprintf(fp, "%s", " ( ");
	  print_optlist(fp, node);
	  if (parens)
	    fprintf(fp, "%s", " ) ");
	}
    }
}

void
print_optlist (FILE *fp, const struct predicate *p)
{
  if (p)
    {
      print_parenthesised(fp, p->pred_left);
      fprintf (fp,
	       "%s%s",
	       p->need_stat ? "[call stat] " : "",
	       p->need_type ? "[need type] " : "");
      print_predicate(fp, p);
      fprintf(fp, " [%g] ", p->est_success_rate);
      if (options.debug_options & DebugSuccessRates)
	{
	  fprintf(fp, "[%ld/%ld", p->perf.successes, p->perf.visits);
	  if (p->perf.visits)
	    {
	      double real_rate = (double)p->perf.successes / (double)p->perf.visits;
	      fprintf(fp, "=%g] ", real_rate);
	    }
	  else
	    {
	      fprintf(fp, "=_] ");
	    }
	}
      print_parenthesised(fp, p->pred_right);
    }
}

void show_success_rates(const struct predicate *p)
{
  if (options.debug_options & DebugSuccessRates)
    {
      fprintf(stderr, "Predicate success rates after completion:\n");
      print_optlist(stderr, p);
      fprintf(stderr, "\n");
    }
}




#ifdef _NDEBUG
/* If _NDEBUG is defined, the assertions will do nothing.   Hence
 * there is no point in having a function body for pred_sanity_check()
 * if that preprocessor macro is defined.
 */
void
pred_sanity_check(const struct predicate *predicates)
{
  /* Do nothing, since assert is a no-op with _NDEBUG set */
  return;
}
#else
void
pred_sanity_check(const struct predicate *predicates)
{
  const struct predicate *p;

  for (p=predicates; p != NULL; p=p->pred_next)
    {
      /* All predicates must do something. */
      assert (p->pred_func != NULL);

      /* All predicates must have a parser table entry. */
      assert (p->parser_entry != NULL);

      /* If the parser table tells us that just one predicate function is
       * possible, verify that that is still the one that is in effect.
       * If the parser has NULL for the predicate function, that means that
       * the parse_xxx function fills it in, so we can't check it.
       */
      if (p->parser_entry->pred_func)
	{
	  assert (p->parser_entry->pred_func == p->pred_func);
	}

      switch (p->parser_entry->type)
	{
	  /* Options all take effect during parsing, so there should
	   * be no predicate entries corresponding to them.  Hence we
	   * should not see any ARG_OPTION or ARG_POSITIONAL_OPTION
	   * items.
	   *
	   * This is a silly way of coding this test, but it prevents
	   * a compiler warning (i.e. otherwise it would think that
	   * there would be case statements missing).
	   */
	case ARG_OPTION:
	case ARG_POSITIONAL_OPTION:
	  assert (p->parser_entry->type != ARG_OPTION);
	  assert (p->parser_entry->type != ARG_POSITIONAL_OPTION);
	  break;

	case ARG_ACTION:
	  assert(p->side_effects); /* actions have side effects. */
	  if (!pred_is(p, pred_prune) && !pred_is(p, pred_quit))
	    {
	      /* actions other than -prune and -quit should
	       * inhibit the default -print
	       */
	      assert (p->no_default_print);
	    }
	  break;

	/* We happen to know that the only user of ARG_SPECIAL_PARSE
	 * is a test, so handle it like ARG_TEST.
	 */
	case ARG_SPECIAL_PARSE:
	case ARG_TEST:
	case ARG_PUNCTUATION:
	case ARG_NOOP:
	  /* Punctuation and tests should have no side
	   * effects and not inhibit default print.
	   */
	  assert (!p->no_default_print);
	  assert (!p->side_effects);
	  break;
	}
    }
}
#endif
