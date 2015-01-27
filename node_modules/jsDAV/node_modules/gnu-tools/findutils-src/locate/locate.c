/* locate -- search databases for filenames that match patterns
   Copyright (C) 1994, 1996, 1998, 1999, 2000, 2003,
                 2004, 2005, 2006, 2007, 2008 Free Software Foundation, Inc.

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

/* Usage: locate [options] pattern...

   Scan a pathname list for the full pathname of a file, given only
   a piece of the name (possibly containing shell globbing metacharacters).
   The list has been processed with front-compression, which reduces
   the list size by a factor of 4-5.
   Recognizes two database formats, old and new.  The old format is
   bigram coded, which reduces space by a further 20-25% and uses the
   following encoding of the database bytes:

   0-28		likeliest differential counts + offset (14) to make nonnegative
   30		escape code for out-of-range count to follow in next halfword
   128-255      bigram codes (the 128 most common, as determined by `updatedb')
   32-127       single character (printable) ASCII remainder

   Earlier versions of GNU locate used to use a novel two-tiered
   string search technique, which was described in Usenix ;login:, Vol
   8, No 1, February/March, 1983, p. 8.

   However, latterly code changes to provide additional functionality
   became dificult to make with the existing reading scheme, and so 
   we no longer perform the matching as efficiently as we used to (that is, 
   we no longer use the same algorithm).

   The old algorithm was:

      First, match a metacharacter-free subpattern and a partial
      pathname BACKWARDS to avoid full expansion of the pathname list.
      The time savings is 40-50% over forward matching, which cannot
      efficiently handle overlapped search patterns and compressed
      path remainders.
      
      Then, match the actual shell glob pattern (if in this form)
      against the candidate pathnames using the slower shell filename
      matching routines.


   Written by James A. Woods <jwoods@adobe.com>.
   Modified by David MacKenzie <djm@gnu.org>.  
   Additional work by James Youngman and Bas van Gompel.
*/

#include <config.h>

#include <stdio.h>
#include <signal.h>
#include <ctype.h>
#include <sys/types.h>
#include <grp.h>		/* for setgroups() */
#include <sys/stat.h>
#include <time.h>
#include <fnmatch.h>
#include <getopt.h>
#include <xstrtol.h>

#include <stdbool.h>		/* for bool/boolean */

/* The presence of unistd.h is assumed by gnulib these days, so we 
 * might as well assume it too. 
 */
/* We need <unistd.h> for isatty(). */
#include <unistd.h>

#include <fcntl.h>

#define NDEBUG
#include <assert.h>
#include <string.h>


#ifdef STDC_HEADERS
#include <stdlib.h>
#endif

#include <errno.h>

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
#define ngettext(singular,plural,n) ((1==n) ? singular : plural)
#endif
#ifdef gettext_noop
# define N_(String) gettext_noop (String)
#else
/* We used to use (String) instead of just String, but apparently ISO C
 * doesn't allow this (at least, that's what HP said when someone reported
 * this as a compiler bug).  This is HP case number 1205608192.  See
 * also http://gcc.gnu.org/bugzilla/show_bug.cgi?id=11250 (which references 
 * ANSI 3.5.7p14-15).  The Intel icc compiler also rejects constructs
 * like: static const char buf[] = ("string");
 */
# define N_(String) String
#endif

#include "locatedb.h"
#include "xalloc.h"
#include "error.h"
#include "human.h"
#include "dirname.h"
#include "closeout.h"
#include "nextelem.h"
#include "regex.h"
#include "quote.h"
#include "quotearg.h"
#include "printquoted.h"
#include "regextype.h"
#include "findutils-version.h"

/* Note that this evaluates Ch many times.  */
#ifdef _LIBC
# define TOUPPER(Ch) toupper (Ch)
# define TOLOWER(Ch) tolower (Ch)
#else
# define TOUPPER(Ch) (islower (Ch) ? toupper (Ch) : (Ch))
# define TOLOWER(Ch) (isupper (Ch) ? tolower (Ch) : (Ch))
#endif

/* typedef enum {false, true} boolean; */

/* Warn if a database is older than this.  8 days allows for a weekly
   update that takes up to a day to perform.  */
static unsigned int warn_number_units = 8;

/* Printable name of units used in WARN_SECONDS */
static const char warn_name_units[] = N_("days");
#define SECONDS_PER_UNIT (60 * 60 * 24)

enum visit_result
  {
    VISIT_CONTINUE = 1,  /* please call the next visitor */
    VISIT_ACCEPTED = 2,  /* accepted, call no futher callbacks for this file */
    VISIT_REJECTED = 4,  /* rejected, process next file. */
    VISIT_ABORT    = 8   /* rejected, process no more files. */
  };

enum ExistenceCheckType 
  {
    ACCEPT_EITHER,		/* Corresponds to lack of -E/-e option */
    ACCEPT_EXISTING,		/* Corresponds to option -e */
    ACCEPT_NON_EXISTING		/* Corresponds to option -E */
  };

/* Check for existence of files before printing them out? */
enum ExistenceCheckType check_existence = ACCEPT_EITHER;

static int follow_symlinks = 1;

/* What to separate the results with. */
static int separator = '\n';

static struct quoting_options * quote_opts = NULL;
static bool stdout_is_a_tty;
static bool print_quoted_filename;
static bool results_were_filtered;

static const char *selected_secure_db = NULL;


/* Change the number of days old the database can be 
 * before we complain about it. 
 */
static void
set_max_db_age(const char *s) 
{
  char *end;
  unsigned long int val;
  /* XXX: we ignore the case where the input is negative, which is allowed(!). */

  if (0 == *s)
    {
      error(1, 0,
	    _("The argument for option --max-database-age must not be empty"));
    }
  
  
  /* We have to set errno here, otherwise when the function returns ULONG_MAX,
   * we would not be able to tell if that is the correct answer, or whether it
   * signifies an error.
   */
  errno = 0;
  val = strtoul(s, &end, 10);
  
  /* Diagnose number too large, non-numbes and trailing junk. */
  if ((ULONG_MAX == val && ERANGE == errno) ||
      (0 == val && EINVAL == errno))
    {
      error(1, errno,
	    _("Invalid argument %s for option --max-database-age"),
	    quotearg_n_style(0, locale_quoting_style, s));
    }
  else if (*end)
    {
      /* errno wasn't set, don't print its message */
      error(1, 0,
	    _("Invalid argument %s for option --max-database-age"),
	    quotearg_n_style(0, locale_quoting_style, s));
    }
  else
    {
      warn_number_units = val;
    }
}



/* Read in a 16-bit int, high byte first (network byte order).  */

static short
get_short (FILE *fp)
{

  register short x;

  x = (signed char) fgetc (fp) << 8;
  x |= (fgetc (fp) & 0xff);
  return x;
}

const char * const metacharacters = "*?[]\\";

/* Return nonzero if S contains any shell glob characters.
 */
static int 
contains_metacharacter(const char *s)
{
  if (NULL == strpbrk(s, metacharacters))
    return 0;
  else
    return 1;
}

/* locate_read_str()
 *
 * Read bytes from FP into the buffer at offset OFFSET in (*BUF),
 * until we reach DELIMITER or end-of-file.   We reallocate the buffer 
 * as necessary, altering (*BUF) and (*SIZ) as appropriate.  No assumption 
 * is made regarding the content of the data (i.e. the implementation is 
 * 8-bit clean, the only delimiter is DELIMITER).
 *
 * Written Fri May 23 18:41:16 2003 by James Youngman, because getstr() 
 * has been removed from gnulib.  
 *
 * We call the function locate_read_str() to avoid a name clash with the curses
 * function getstr().
 */
static int
locate_read_str(char **buf, size_t *siz, FILE *fp, int delimiter, int offs)
{
  char * p = NULL;
  size_t sz = 0;
  int nread;
  size_t needed;

  nread = getdelim(&p, &sz, delimiter, fp);
  if (nread >= 0)
    {
      assert(p != NULL);
      
      needed = offs + nread + 1u;
      if (needed > (*siz))
	{
	  char *pnew = realloc(*buf, needed);
	  if (NULL == pnew)
	    {
	      return -1;	/* FAIL */
	    }
	  else
	    {
	      *siz = needed;
	      *buf = pnew;
	    }
	}
      memcpy((*buf)+offs, p, nread);
      free(p);
    }
  return nread;
}


struct locate_limits
{
  uintmax_t limit;
  uintmax_t items_accepted;
};
static struct locate_limits limits;


struct locate_stats
{
  uintmax_t compressed_bytes;
  uintmax_t total_filename_count;
  uintmax_t total_filename_length;
  uintmax_t whitespace_count;
  uintmax_t newline_count;
  uintmax_t highbit_filename_count;
};
static struct locate_stats statistics;


struct regular_expression
{
  struct re_pattern_buffer regex; /* for --regex */
};


struct process_data
{
  int c;			/* An input byte.  */
  char itemcount;	        /* Indicates we're at the beginning of an slocate db. */
  int count; /* The length of the prefix shared with the previous database entry.  */
  int len;
  char *original_filename;	/* The current input database entry. */
  size_t pathsize;		/* Amount allocated for it.  */
  char *munged_filename;	/* path or basename(path) */
  FILE *fp;			/* The pathname database.  */
  const char *dbfile;		/* Its name, or "<stdin>" */
  int  slocatedb_format;	/* Allows us to cope with slocate's format variant */
  GetwordEndianState endian_state;
  /* for the old database format,
     the first and second characters of the most common bigrams.  */
  char bigram1[128];
  char bigram2[128];
};


typedef int (*visitfunc)(struct process_data *procdata,
			 void *context);

struct visitor
{
  visitfunc      inspector;
  void *         context;
  struct visitor *next;
};


static struct visitor *inspectors = NULL;
static struct visitor *lastinspector = NULL;
static struct visitor *past_pat_inspector = NULL;

static inline int visit(const struct visitor *p,
			int accept_flags,
			struct process_data *procdata,
			const struct visitor * const stop)
{
  register int result = accept_flags;
  while ( (accept_flags & result) && (stop != p) )
    {
      result = (p->inspector)(procdata, p->context);
      p = p->next;
    }
  return result;
}

/* 0 or 1 pattern(s) */
static int
process_simple(struct process_data *procdata)
{
  return visit(inspectors, (VISIT_CONTINUE|VISIT_ACCEPTED), procdata, NULL);
}

/* Accept if any pattern matches. */
static int
process_or (struct process_data *procdata)
{
  int result;
  
  result = visit(inspectors, (VISIT_CONTINUE|VISIT_REJECTED), procdata, past_pat_inspector);
  if (result == VISIT_CONTINUE)
    result = VISIT_REJECTED;
  if (result & (VISIT_ABORT | VISIT_REJECTED))
    return result;

  result = visit(past_pat_inspector, VISIT_CONTINUE, procdata, NULL);
  if (VISIT_CONTINUE == result)
    return VISIT_ACCEPTED;
  else
    return result;
}

/* Accept if all pattern match. */
static int
process_and (struct process_data *procdata)
{
  int result;
  
  result = visit(inspectors, (VISIT_CONTINUE|VISIT_ACCEPTED), procdata, past_pat_inspector);
  if (result == VISIT_CONTINUE)
    result = VISIT_REJECTED;
  if (result & (VISIT_ABORT | VISIT_REJECTED))
    return result;

  result = visit(past_pat_inspector, VISIT_CONTINUE, procdata, NULL);
  if (VISIT_CONTINUE == result)
    return VISIT_ACCEPTED;
  else
    return result;
}

typedef int (*processfunc)(struct process_data *procdata);

static processfunc mainprocessor = NULL;

static void
add_visitor(visitfunc fn, void *context)
{
  struct visitor *p = xmalloc(sizeof(struct visitor));
  p->inspector = fn;
  p->context   = context;
  p->next = NULL;
  
  if (NULL == lastinspector)
    {
      lastinspector = inspectors = p;
    }
  else
    {
      lastinspector->next = p;
      lastinspector = p;
    }
}

static int
visit_justprint_quoted(struct process_data *procdata, void *context)
{
  (void) context;
  print_quoted (stdout, quote_opts, stdout_is_a_tty,
		"%s", 
		procdata->original_filename);
  putchar(separator);
  return VISIT_CONTINUE;
}

static int
visit_justprint_unquoted(struct process_data *procdata, void *context)
{
  (void) context;
  fputs(procdata->original_filename, stdout);
  putchar(separator);
  return VISIT_CONTINUE;
}

static void
toolong (struct process_data *procdata)
{
  error (1, 0,
	 _("locate database %s contains a "
	   "filename longer than locate can handle"),
	 procdata->dbfile);
}

static void
extend (struct process_data *procdata, size_t siz1, size_t siz2)
{
  /* Figure out if the addition operation is safe before performing it. */
  if (SIZE_MAX - siz1 < siz2)
    {
      toolong (procdata);
    }
  else if (procdata->pathsize < (siz1+siz2))
    {
      procdata->pathsize = siz1+siz2;
      procdata->original_filename = x2nrealloc (procdata->original_filename,
						&procdata->pathsize,
						1);
    }
}

static int
visit_old_format(struct process_data *procdata, void *context)
{
  register size_t i;
  (void) context;

  if (EOF == procdata->c)
    return VISIT_ABORT;

  /* Get the offset in the path where this path info starts.  */
  if (procdata->c == LOCATEDB_OLD_ESCAPE)
    {
      int minval, maxval;
      int word;
      
      procdata->count -= LOCATEDB_OLD_OFFSET;
      minval = (0       - procdata->count);
      if (procdata->count >= 0)
	maxval = (procdata->len - procdata->count);
      else
	maxval = (procdata->len - 0);
      word = getword(procdata->fp, procdata->dbfile,
		     minval, maxval, &procdata->endian_state);
      procdata->count += word;
      assert(procdata->count >= 0);
    }
  else
    {
      procdata->count += (procdata->c - LOCATEDB_OLD_OFFSET);
      assert(procdata->count >= 0);
    }

  /* Overlay the old path with the remainder of the new.  Read 
   * more data until we get to the next filename.
   */
  for (i=procdata->count;
       (procdata->c = getc (procdata->fp)) > LOCATEDB_OLD_ESCAPE;)
    {
      if (EOF == procdata->c)
	break;

      if (procdata->c < 0200)
	{
	  /* An ordinary character. */	  
	  extend (procdata, i, 1u);
	  procdata->original_filename[i++] = procdata->c;
	}
      else
	{
	  /* Bigram markers have the high bit set. */
	  extend (procdata, i, 2u);
	  procdata->c &= 0177;
	  procdata->original_filename[i++] = procdata->bigram1[procdata->c];
	  procdata->original_filename[i++] = procdata->bigram2[procdata->c];
	}
    }

  /* Consider the case where we executed the loop body zero times; we
   * still need space for the terminating null byte. 
   */
  extend (procdata, i, 1u);
  procdata->original_filename[i] = 0;
  procdata->len = i;
  procdata->munged_filename = procdata->original_filename;
  
  return VISIT_CONTINUE;
}

static int
visit_locate02_format(struct process_data *procdata, void *context)
{
  register char *s;
  int nread;
  (void) context;

  if (procdata->slocatedb_format)
    {
      if (procdata->itemcount == 0)
	{
	  ungetc(procdata->c, procdata->fp);
	  procdata->count = 0;
	  procdata->len = 0;
	}
      else if (procdata->itemcount == 1)
	{
	  procdata->count = procdata->len-1;
	}
      else
	{
	  if (procdata->c == LOCATEDB_ESCAPE)
	    procdata->count += (short)get_short (procdata->fp);
	  else if (procdata->c > 127)
	    procdata->count += procdata->c - 256;
	  else
	    procdata->count += procdata->c;
	}
    }
  else
    {
      if (procdata->c == LOCATEDB_ESCAPE)
	procdata->count += (short)get_short (procdata->fp);
      else if (procdata->c > 127)
	procdata->count += procdata->c - 256;
      else
	procdata->count += procdata->c;
    }

  if (procdata->count > procdata->len || procdata->count < 0)
    {
      /* This should not happen generally , but since we're
       * reading in data which is outside our control, we
       * cannot prevent it.
       */
      error(1, 0, _("locate database %s is corrupt or invalid"),
	    quotearg_n_style(0, locale_quoting_style, procdata->dbfile));
    }
 
  /* Overlay the old path with the remainder of the new.  */
  nread = locate_read_str (&procdata->original_filename,
			   &procdata->pathsize,
			   procdata->fp, 0, procdata->count);
  if (nread < 0)
    return VISIT_ABORT;
  procdata->c = getc (procdata->fp);
  procdata->len = procdata->count + nread;
  s = procdata->original_filename + procdata->len - 1; /* Move to the last char in path.  */
  assert (s[0] != '\0');
  assert (s[1] == '\0'); /* Our terminator.  */
  assert (s[2] == '\0'); /* Added by locate_read_str.  */

  procdata->munged_filename = procdata->original_filename;
  
  if (procdata->slocatedb_format)
    {
      /* Don't increment indefinitely, it might overflow. */
      if (procdata->itemcount < 6)
	{
	  ++(procdata->itemcount);
	}
    }
  

  return VISIT_CONTINUE;
}

static int
visit_basename(struct process_data *procdata, void *context)
{
  (void) context;
  procdata->munged_filename = last_component (procdata->original_filename);

  return VISIT_CONTINUE;
}


/* visit_existing_follow implements -L -e */
static int
visit_existing_follow(struct process_data *procdata, void *context)
{
  struct stat st;
  (void) context;

  /* munged_filename has been converted in some way (to lower case,
   * or is just the base name of the file), and original_filename has not.  
   * Hence only original_filename is still actually the name of the file 
   * whose existence we would need to check.
   */
  if (stat(procdata->original_filename, &st) != 0)
    {
      return VISIT_REJECTED;
    }
  else
    {
      return VISIT_CONTINUE;
    }
}

/* visit_non_existing_follow implements -L -E */
static int
visit_non_existing_follow(struct process_data *procdata, void *context)
{
  struct stat st;
  (void) context;

  /* munged_filename has been converted in some way (to lower case,
   * or is just the base name of the file), and original_filename has not.  
   * Hence only original_filename is still actually the name of the file 
   * whose existence we would need to check.
   */
  if (stat(procdata->original_filename, &st) == 0)
    {
      return VISIT_REJECTED;
    }
  else
    {
      return VISIT_CONTINUE;
    }
}

/* visit_existing_nofollow implements -P -e */
static int
visit_existing_nofollow(struct process_data *procdata, void *context)
{
  struct stat st;
  (void) context;

  /* munged_filename has been converted in some way (to lower case,
   * or is just the base name of the file), and original_filename has not.  
   * Hence only original_filename is still actually the name of the file 
   * whose existence we would need to check.
   */
  if (lstat(procdata->original_filename, &st) != 0)
    {
      return VISIT_REJECTED;
    }
  else
    {
      return VISIT_CONTINUE;
    }
}

/* visit_non_existing_nofollow implements -P -E */
static int
visit_non_existing_nofollow(struct process_data *procdata, void *context)
{
  struct stat st;
  (void) context;

  /* munged_filename has been converted in some way (to lower case,
   * or is just the base name of the file), and original_filename has not.  
   * Hence only original_filename is still actually the name of the file 
   * whose existence we would need to check.
   */
  if (lstat(procdata->original_filename, &st) == 0)
    {
      return VISIT_REJECTED;
    }
  else
    {
      return VISIT_CONTINUE;
    }
}

static int
visit_substring_match_nocasefold_wide(struct process_data *procdata, void *context)
{
  const char *pattern = context;

  if (NULL != mbsstr(procdata->munged_filename, pattern))
    return VISIT_ACCEPTED;
  else
    return VISIT_REJECTED;
}

static int
visit_substring_match_nocasefold_narrow(struct process_data *procdata, void *context)
{
  const char *pattern = context;
  assert(MB_CUR_MAX == 1);
  if (NULL != strstr(procdata->munged_filename, pattern))
    return VISIT_ACCEPTED;
  else
    return VISIT_REJECTED;
}

static int
visit_substring_match_casefold_wide(struct process_data *procdata, void *context)
{
  const char *pattern = context;

  if (NULL != mbscasestr(procdata->munged_filename, pattern))
    return VISIT_ACCEPTED;
  else
    return VISIT_REJECTED;
}


static int
visit_substring_match_casefold_narrow(struct process_data *procdata, void *context)
{
  const char *pattern = context;

  assert(MB_CUR_MAX == 1);
  if (NULL != strcasestr(procdata->munged_filename, pattern))
    return VISIT_ACCEPTED;
  else
    return VISIT_REJECTED;
}


static int
visit_globmatch_nofold(struct process_data *procdata, void *context)
{
  const char *glob = context;
  if (fnmatch(glob, procdata->munged_filename, 0) != 0)
    return VISIT_REJECTED;
  else
    return VISIT_ACCEPTED;
}


static int
visit_globmatch_casefold(struct process_data *procdata, void *context)
{
  const char *glob = context;
  if (fnmatch(glob, procdata->munged_filename, FNM_CASEFOLD) != 0)
    return VISIT_REJECTED;
  else
    return VISIT_ACCEPTED;
}


static int
visit_regex(struct process_data *procdata, void *context)
{
  struct regular_expression *p = context;
  const size_t len = strlen(procdata->munged_filename);

  int rv = re_search (&p->regex, procdata->munged_filename,
		      len, 0, len, 
		      (struct re_registers *) NULL);
  if (rv < 0)
    {
      return VISIT_REJECTED;	/* no match (-1), or internal error (-2) */
    }
  else 
    {
      return VISIT_ACCEPTED;	/* match */
    }
}


static int
visit_stats(struct process_data *procdata, void *context)
{
  struct locate_stats *p = context;
  size_t len = strlen(procdata->original_filename);
  const char *s;
  int highbit, whitespace, newline;
  
  ++(p->total_filename_count);
  p->total_filename_length += len;
  
  highbit = whitespace = newline = 0;
  for (s=procdata->original_filename; *s; ++s)
    {
      if ( (int)(*s) & 128 )
	highbit = 1;
      if ('\n' == *s)
	{
	  newline = whitespace = 1;
	}
      else if (isspace((unsigned char)*s))
	{
	  whitespace = 1;
	}
    }

  if (highbit)
    ++(p->highbit_filename_count);
  if (whitespace)
    ++(p->whitespace_count);
  if (newline)
    ++(p->newline_count);

  return VISIT_CONTINUE;
}


static int
visit_limit(struct process_data *procdata, void *context)
{
  struct locate_limits *p = context;

  (void) procdata;

  if (++p->items_accepted >= p->limit)
    return VISIT_ABORT;
  else
    return VISIT_CONTINUE;
}

static int
visit_count(struct process_data *procdata, void *context)
{
  struct locate_limits *p = context;

  (void) procdata;

  ++p->items_accepted;
  return VISIT_CONTINUE;
}

/* Emit the statistics.
 */
static void
print_stats(int argc, size_t database_file_size)
{
  char hbuf1[LONGEST_HUMAN_READABLE + 1];
  char hbuf2[LONGEST_HUMAN_READABLE + 1];
  char hbuf3[LONGEST_HUMAN_READABLE + 1];
  char hbuf4[LONGEST_HUMAN_READABLE + 1];
  
  printf(ngettext("Locate database size: %s byte\n",
		  "Locate database size: %s bytes\n",
		  database_file_size),
	 human_readable ((uintmax_t) database_file_size,
			 hbuf1, human_ceiling, 1, 1));
  
  printf( (results_were_filtered ? 
	   _("Matching Filenames: %s\n") :
	   _("All Filenames: %s\n")),
	  human_readable (statistics.total_filename_count,
			 hbuf1, human_ceiling, 1, 1));
  /* XXX: We would ideally use ngettext() here, but I don't know 
   *      how to use it to handle more than one possibly-plural thing/
   */
  printf(_("File names have a cumulative length of %s bytes.\n"
	   "Of those file names,\n"
	   "\n\t%s contain whitespace, "
	   "\n\t%s contain newline characters, "
	   "\n\tand %s contain characters with the high bit set.\n"),
	 human_readable (statistics.total_filename_length,  hbuf1, human_ceiling, 1, 1),
	 human_readable (statistics.whitespace_count,       hbuf2, human_ceiling, 1, 1),
	 human_readable (statistics.newline_count,          hbuf3, human_ceiling, 1, 1),
	 human_readable (statistics.highbit_filename_count, hbuf4, human_ceiling, 1, 1));
  
  if (!argc)
    {
      if (results_were_filtered)
	{
	  printf(_("Some filenames may have been filtered out, "
		   "so we cannot compute the compression ratio.\n"));
	}
      else
	{
	  if (statistics.total_filename_length)
	    {
	      /* A negative compression ratio just means that the
	       * compressed database is larger than the list of
	       * filenames.  This can happen for example for
	       * old-format databases containing a small list of short
	       * filenames, because the bigram list is 256 bytes.
	       */
	      printf(_("Compression ratio %4.2f%% (higher is better)\n"),
		     100.0 * ((double)statistics.total_filename_length
			      - (double) database_file_size)
		     / (double) statistics.total_filename_length);
	    }
	  else
	    {
	      printf(_("Compression ratio is undefined\n"));
	    }
	}
    }
  printf("\n");
}

/*
 * Return nonzero if the data we read in indicates that we are 
 * looking at a LOCATE02 locate database. 
 */
static int 
looking_at_gnu_locatedb (const char *data, size_t len)
{
  if (len < sizeof (LOCATEDB_MAGIC))
    return 0;
  else if (0 == memcmp (data, LOCATEDB_MAGIC, sizeof (LOCATEDB_MAGIC)))
    return 1;			/* We saw the magic byte sequence */
  else
    return 0;
}

/*
 * Return nonzero if the data we read in indicates that we are 
 * looking at an slocate database. 
 */
static int 
looking_at_slocate_locatedb (const char *filename,
			     const char *data,
			     size_t len,
			     int *seclevel)
{
  assert(len <= 2);
  
  if (len < 2)
    {
      return 0;
    }
  else 
    {
      /* Check that the magic number is a one-byte string */
      if (0 == data[1])
	{
	  if (isdigit((unsigned char)data[0]))
	    {
	      /* looks promising. */
	      *seclevel = (data[0] - '0');
	      
	      if (*seclevel > 1)
		{
		  /* Hmm, well it's probably an slocate database 
		   * of some awsomely huge security level, like 2.
		   * We don't know how to handle those.
		   */
		  error(0, 0,
			_("locate database %s looks like an slocate "
			  "database but it seems to have security level %c, "
			  "which GNU findutils does not currently support"),
			quotearg_n_style(0, locale_quoting_style, filename),
			data[1]);
		  return 1;
		}
	      else
		{
		  return 1;
		}
	    }
	  else
	    {
	      /* Not a digit. */
	      return 0;
	    }
	}
      else
	{
	  /* Definitely not slocate. */
	  return 0;
	}
    }
}


static int 
i_am_little_endian(void)
{
  union 
  {
    unsigned char uch[4];
    unsigned int ui;
  } u;
  u.ui = 0u;
  u.uch[0] = 1;
  u.uch[1] = u.uch[2] = u.uch[3] = 0;
  return u.ui == 1;
}




/* Print or count the entries in DBFILE that match shell globbing patterns in 
   ARGV. Return the number of entries matched. */

static unsigned long
search_one_database (int argc,
		     char **argv,
		     const char *dbfile,
		     FILE *fp,
		     off_t filesize,
		     int ignore_case,
		     int enable_print,
		     int basename_only,
		     int use_limit,
		     struct locate_limits *plimit,
		     int stats,
		     int op_and,
		     int regex,
		     int regex_options)
{
  char *pathpart;		/* A pattern to consider. */
  int argn;			/* Index to current pattern in argv. */
  int nread;		     /* number of bytes read from an entry. */
  struct process_data procdata;	/* Storage for data shared with visitors. */
  int slocate_seclevel;
  int oldformat;
  struct visitor* pvis; /* temp for determining past_pat_inspector. */
  const char *format_name;
  enum ExistenceCheckType do_check_existence;


  /* We may turn on existence checking for a given database. 
   * We ensure that we can return to the previous behaviour 
   * by using two variables, do_check_existence (which we act on) 
   * and check_existence (whcih indicates the default before we 
   * adjust it on the bassis of what kind of database we;re using 
   */
  do_check_existence = check_existence;
   
  
  if (ignore_case)
    regex_options |= RE_ICASE;

  oldformat = 0;
  procdata.endian_state = GetwordEndianStateInitial;
  procdata.len = procdata.count = 0;
  procdata.slocatedb_format = 0;
  procdata.itemcount = 0;

  procdata.dbfile = dbfile;
  procdata.fp = fp;
    
  /* Set up the inspection regime */
  inspectors = NULL;
  lastinspector = NULL;
  past_pat_inspector = NULL;
  results_were_filtered = false;
#if 0  
  procdata.pathsize = 1026;	/* Increased as necessary by locate_read_str.  */
#else
  procdata.pathsize = 128;	/* Increased as necessary by locate_read_str.  */
#endif
  procdata.original_filename = xmalloc (procdata.pathsize);


  nread = fread (procdata.original_filename, 1, SLOCATE_DB_MAGIC_LEN,
		 procdata.fp);
  slocate_seclevel = 0;
  if (looking_at_slocate_locatedb(procdata.dbfile,
				  procdata.original_filename,
				  nread,
				  &slocate_seclevel))
    {
      error(0, 0,
	    _("%s is an slocate database.  "
	      "Support for these is new, expect problems for now."),
	    quotearg_n_style(0, locale_quoting_style, procdata.dbfile));
      
      /* slocate also uses frcode, but with a different header. 
       * We handle the header here and then work with the data 
       * in the normal way.
       */
      if (slocate_seclevel > 1)
	{
	  /* We don't know what those security levels mean, 
	   * so do nothing further 
	   */
	  error(0, 0,
		_("%s is an slocate database of unsupported security level %d; skipping it."),
		quotearg_n_style(0, locale_quoting_style, procdata.dbfile),
		slocate_seclevel);
	  return 0;
	}
      else if (slocate_seclevel > 0)
	{
	  /* Don't show the filenames to the user if they don't exist.
	   * Showing stats is safe since filenames are only counted
	   * after the existence check 
	   */
	  if (ACCEPT_NON_EXISTING == check_existence)
	    {
	      /* Do not allow the user to see a list of filenames that they 
	       * cannot stat().
	       */
	      error(0, 0, 
		    _("You specified the -E option, but that option "
		      "cannot be used with slocate-format databases "
		      "with a non-zero security level.  No results will be "
		      "generated for this database.\n"));
	      return 0;
	    }
	  if (ACCEPT_EXISTING != do_check_existence) 
	    {
	      if (enable_print || stats)
		{
		  error(0, 0,
			_("%s is an slocate database.  "
			  "Turning on the '-e' option."),
			quotearg_n_style(0, locale_quoting_style, procdata.dbfile));
		}
	      do_check_existence = ACCEPT_EXISTING;
	    }
	}
      add_visitor(visit_locate02_format, NULL);
      format_name = "slocate";
      procdata.slocatedb_format = 1;
    }
  else 
    {
      int nread2;
      
      procdata.slocatedb_format = 0;
      extend (&procdata, sizeof(LOCATEDB_MAGIC), 0u);
      nread2 = fread (procdata.original_filename+nread, 1, sizeof (LOCATEDB_MAGIC)-nread,
		      procdata.fp);
      if (looking_at_gnu_locatedb(procdata.original_filename, nread+nread2))
	{
	  add_visitor(visit_locate02_format, NULL);
	  format_name = "GNU LOCATE02";
	}
      else				/* Use the old format */
	{
	  int i;

	  nread += nread2;
	  extend (&procdata, 256u, 0u);
	  /* Read the list of the most common bigrams in the database.  */
	  if (nread < 256)
	    {
	      int more_read = fread (procdata.original_filename + nread, 1,
				     256 - nread, procdata.fp);
	      if ( (more_read + nread) != 256 )
		{
		  error(1, 0,
			_("Old-format locate database %s is "
			  "too short to be valid"),
			quotearg_n_style(0, locale_quoting_style, dbfile));
		  
		}
	    }
	  
	  for (i = 0; i < 128; i++)
	    {
	      procdata.bigram1[i] = procdata.original_filename[i << 1];
	      procdata.bigram2[i] = procdata.original_filename[(i << 1) + 1];
	    }
	  format_name = "old";
	  oldformat = 1;
	  add_visitor(visit_old_format, NULL);
	}
    }

  if (basename_only)
    add_visitor(visit_basename, NULL);
  
  /* Add an inspector for each pattern we're looking for. */
  for ( argn = 0; argn < argc; argn++ )
    {
      results_were_filtered = true;
      pathpart = argv[argn];
      if (regex)
	{
	  struct regular_expression *p = xmalloc(sizeof(*p));
	  const char *error_message = NULL;
	  
	  memset (&p->regex, 0, sizeof (p->regex));
	  
	  re_set_syntax(regex_options);
	  p->regex.allocated = 100;
	  p->regex.buffer = xmalloc (p->regex.allocated);
	  p->regex.fastmap = NULL;
	  p->regex.syntax = regex_options;
	  p->regex.translate = NULL;

	  error_message = re_compile_pattern (pathpart, strlen (pathpart),
					      &p->regex);
	  if (error_message)
	    {
	      error (1, 0, "%s", error_message);
	    }
	  else 
	    {
	      add_visitor(visit_regex, p);
	    }
	}
      else if (contains_metacharacter(pathpart))
	{
	  if (ignore_case)
	    add_visitor(visit_globmatch_casefold, pathpart);
	  else
	    add_visitor(visit_globmatch_nofold, pathpart);
	}
      else
	{
	  /* No glob characters used.  Hence we match on 
	   * _any part_ of the filename, not just the 
	   * basename.  This seems odd to me, but it is the 
	   * traditional behaviour.
	   * James Youngman <jay@gnu.org> 
	   */
	  visitfunc matcher;
	  if (1 == MB_CUR_MAX)
	    {
	      /* As an optimisation, use a strstr() matcher if we are
	       * in a unibyte locale.  This can give a x2 speedup in
	       * the C locale.  Some light testing reveals that
	       * glibc's strstr() is somewhere around 40% faster than
	       * gnulib's, so we just use strstr().
	       */
	      matcher = ignore_case ?
		visit_substring_match_casefold_narrow  :
		visit_substring_match_nocasefold_narrow;
	    }
	  else
	    {
	      matcher = ignore_case ?
		visit_substring_match_casefold_wide  :
		visit_substring_match_nocasefold_wide;
	    }
	  add_visitor(matcher, pathpart);
	}
    }

  pvis = lastinspector;

  /* We add visit_existing_*() as late as possible to reduce the
   * number of stat() calls.
   */
  switch (do_check_existence)
    {
      case ACCEPT_EXISTING:
	results_were_filtered = true;
	if (follow_symlinks)	/* -L, default */
	  add_visitor(visit_existing_follow, NULL);
	else			/* -P */
	  add_visitor(visit_existing_nofollow, NULL);
	break;
	  
      case ACCEPT_NON_EXISTING:
	results_were_filtered = true;
	if (follow_symlinks)	/* -L, default */
	  add_visitor(visit_non_existing_follow, NULL);
	else			/* -P */
	  add_visitor(visit_non_existing_nofollow, NULL);
	break;

      case ACCEPT_EITHER:	/* Default, neither -E nor -e */
	/* do nothing; no extra processing. */
	break;
    }

  /* Security issue: The stats visitor must be added immediately
   * before the print visitor, because otherwise the -S option would
   * leak information about files that the caller cannot see.
   */
  if (stats)
    add_visitor(visit_stats, &statistics);

  if (enable_print)
    {
      if (print_quoted_filename)
	add_visitor(visit_justprint_quoted,   NULL);
      else
	add_visitor(visit_justprint_unquoted, NULL);
    }


  if (use_limit)
    add_visitor(visit_limit, plimit);
  else
    add_visitor(visit_count, plimit);


  if (argc > 1)
    {
      past_pat_inspector = pvis->next;
      if (op_and)
        mainprocessor = process_and;
      else
        mainprocessor = process_or;
    }
  else
    mainprocessor = process_simple;

  if (stats)
    {
      printf(_("Database %s is in the %s format.\n"),
	     procdata.dbfile,
	     format_name);
    }


  procdata.c = getc (procdata.fp);
  /* If we are searching for filename patterns, the inspector list 
   * will contain an entry for each pattern for which we are searching.
   */
  while ( (procdata.c != EOF) &&
          (VISIT_ABORT != (mainprocessor)(&procdata)) )
    {
      /* Do nothing; all the work is done in the visitor functions. */
    }
  
  if (stats)
    {
      if (oldformat)
	{
	  int host_little_endian = i_am_little_endian();
	  const char *little = _("The database has little-endian "
				 "machine-word encoding.\n");
	  const char *big    = _("The database has big-endian "
				 "machine-word encoding.\n");
	  
	  if (GetwordEndianStateNative == procdata.endian_state)
	    {
	      printf("%s", (host_little_endian ? little : big));
	    }
	  else if (GetwordEndianStateSwab == procdata.endian_state)
	    {
	      printf("%s", (host_little_endian ? big : little));
	    }
	  else
	    {
	      printf(_("The database machine-word encoding order "
		       "is not obvious.\n"));
	    }
	}
      if (filesize)
	print_stats(argc, filesize);
    }
  
  if (ferror (procdata.fp))
    {
      error (0, errno, "%s",
	     quotearg_n_style(0, locale_quoting_style, procdata.dbfile));
      return 0;
    }
  return plimit->items_accepted;
}




extern char *version_string;

/* The name this program was run with. */
char *program_name;

static void
usage (FILE *stream)
{
  fprintf (stream, _("\
Usage: %s [-d path | --database=path] [-e | -E | --[non-]existing]\n\
      [-i | --ignore-case] [-w | --wholename] [-b | --basename] \n\
      [--limit=N | -l N] [-S | --statistics] [-0 | --null] [-c | --count]\n\
      [-P | -H | --nofollow] [-L | --follow] [-m | --mmap ] [ -s | --stdio ]\n\
      [-A | --all] [-p | --print] [-r | --regex ] [--regextype=TYPE]\n\
      [--max-database-age D] [--version] [--help]\n\
      pattern...\n"),
	   program_name);
  fputs (_("\nReport bugs to <bug-findutils@gnu.org>.\n"), stream);
}
enum
  {
    REGEXTYPE_OPTION = CHAR_MAX + 1,
    MAX_DB_AGE
  };


static struct option const longopts[] =
{
  {"database", required_argument, NULL, 'd'},
  {"existing", no_argument, NULL, 'e'},
  {"non-existing", no_argument, NULL, 'E'},
  {"ignore-case", no_argument, NULL, 'i'},
  {"all", no_argument, NULL, 'A'},
  {"help", no_argument, NULL, 'h'},
  {"version", no_argument, NULL, 'v'},
  {"null", no_argument, NULL, '0'},
  {"count", no_argument, NULL, 'c'},
  {"wholename", no_argument, NULL, 'w'},
  {"wholepath", no_argument, NULL, 'w'}, /* Synonym. */
  {"basename", no_argument, NULL, 'b'},
  {"print", no_argument, NULL, 'p'},
  {"stdio", no_argument, NULL, 's'},
  {"mmap",  no_argument, NULL, 'm'},
  {"limit",  required_argument, NULL, 'l'},
  {"regex",  no_argument, NULL, 'r'},
  {"regextype",  required_argument, NULL, REGEXTYPE_OPTION},
  {"statistics",  no_argument, NULL, 'S'},
  {"follow",      no_argument, NULL, 'L'},
  {"nofollow",    no_argument, NULL, 'P'},
  {"max-database-age",    required_argument, NULL, MAX_DB_AGE},
  {NULL, no_argument, NULL, 0}
};


static int 
drop_privs(void)
{
  const char * what = "failed";
  const uid_t orig_euid = geteuid();
  const uid_t uid       = getuid();
  const gid_t gid       = getgid();

#if HAVE_SETGROUPS  
  /* Use of setgroups() is restricted to root only. */
  if (0 == orig_euid)
    {
      /* We're either root or running setuid-root. */
      gid_t groups[1];
      groups[0] = gid;
      if (0 != setgroups(1u, groups)) 
	{
	  what = _("failed to drop group privileges");
	  goto fail;
	}
    }
#endif

  /* Drop any setuid privileges */
  if (uid != orig_euid)
    {
      if (0 == uid)
	{
	  /* We're really root anyway, but are setuid to something else. Leave it. */
	}
      else
	{
	  errno = 0;
	  if (0 != setuid(getuid()))
	    {
	      what = _("failed to drop setuid privileges");
	      goto fail;
	    }
	  
	  /* Defend against the case where the attacker runs us with the
	   * capability to call setuid() turned off, which on some systems
	   * will cause the above attempt to drop privileges fail (leaving us
	   * privileged).
	   */
	  else
	    {
	      /* Check that we can no longer switch bask to root */
	      if (0 == setuid(0))
		{
		  what = _("Failed to fully drop privileges");
		  /* The errno value here is not interesting (since
		   * the system call we are complaining about
		   * succeeded when we wanted it to fail).  Arrange
		   * for the call to error() not to print the errno
		   * value by setting errno=0.
		   */
		  errno = 0;
		  goto fail;
		}
	    }
	}
    }
  
  /* Drop any setgid privileges */
  errno = 0;
  if (0 != setgid(gid))
    {
      what = _("failed to drop setgid privileges");
      goto fail;
    }

  /* success. */
  return 0;
  
 fail:
  error(1, errno, "%s",
	quotearg_n_style(0, locale_quoting_style, what));
  abort();
  kill(0, SIGKILL);
  _exit(1);
  /*NOTREACHED*/
  /* ... we hope. */
  for (;;)
    {
      /* deliberate infinite loop */
    }
}

static int
opendb(const char *name)
{
  int fd = open(name, O_RDONLY
#if defined O_LARGEFILE
		|O_LARGEFILE
#endif
		);
  if (fd >= 0) 
    {
      /* Make sure it won't survive an exec */
      if (0 != fcntl(fd, F_SETFD, FD_CLOEXEC)) 
	{
	  close(fd);
	  fd = -1;
	}
    }
  return fd;
}

int
dolocate (int argc, char **argv, int secure_db_fd)
{
  char *dbpath;
  unsigned long int found = 0uL;
  int ignore_case = 0;
  int print = 0;
  int just_count = 0;
  int basename_only = 0;
  int use_limit = 0;
  int regex = 0;
  int regex_options = RE_SYNTAX_EMACS;
  int stats = 0;
  int op_and = 0;
  const char *e;
  FILE *fp;
  int they_chose_db = 0;
  bool did_stdin = false;	/* Set to prevent rereading stdin. */
  
  program_name = argv[0];

#ifdef HAVE_SETLOCALE
  setlocale (LC_ALL, "");
#endif
  bindtextdomain (PACKAGE, LOCALEDIR);
  textdomain (PACKAGE);
  atexit (close_stdout);

  limits.limit = 0;
  limits.items_accepted = 0;

  quote_opts = clone_quoting_options (NULL);
  print_quoted_filename = true;
  
  /* We cannot simultaneously trust $LOCATE_PATH and use the
   * setuid-access-controlled database,, since that could cause a leak
   * of private data.
   */
  dbpath = getenv ("LOCATE_PATH");
  if (dbpath)
    {
      they_chose_db = 1;
    }

  check_existence = ACCEPT_EITHER;

  for (;;)
    {
      int opti = -1;
      int optc = getopt_long (argc, argv, "Abcd:eEil:prsm0SwHPL", longopts,
			      &opti);
      if (optc == -1)
	break;

      switch (optc)
	{
	case '0':
	  separator = 0;
	  print_quoted_filename = false; /* print filename 'raw'. */
	  break;

	case 'A':
	  op_and = 1;
	  break;

	case 'b':
	  basename_only = 1;
	  break;

	case 'c':
	  just_count = 1;
	  break;

	case 'd':
	  dbpath = optarg;
	  they_chose_db = 1;
	  break;

	case 'e':
	  check_existence = ACCEPT_EXISTING;
	  break;

	case 'E':
	  check_existence = ACCEPT_NON_EXISTING;
	  break;

	case 'i':
	  ignore_case = 1;
	  break;

	case 'h':
	  usage (stdout);
	  return 0;

	case MAX_DB_AGE:
	  /* XXX: nothing in the test suite for this option. */
	  set_max_db_age (optarg);
	  break;

	case 'p':
	  print = 1;
	  break;

	case 'v':
	  display_findutils_version ("locate");
	  return 0;

	case 'w':
	  basename_only = 0;
	  break;

	case 'r':
	  regex = 1;
	  break;

	case REGEXTYPE_OPTION:
	  regex_options = get_regex_type (optarg);
	  break;

	case 'S':
	  stats = 1;
	  break;

	case 'L':
	  follow_symlinks = 1;
	  break;

	  /* In find, -P and -H differ in the way they handle paths
	   * given on the command line.  This is not relevant for
	   * locate, but the -H option is supported because it is
	   * probably more intuitive to do so.
	   */
	case 'P':
	case 'H':
	  follow_symlinks = 0;
	  break;

	case 'l':
	  {
	    char *end = optarg;
	    strtol_error err = xstrtoumax (optarg, &end, 10, &limits.limit,
					   NULL);
	    if (LONGINT_OK != err)
	      xstrtol_fatal (err, opti, optc, longopts, optarg);
	    use_limit = 1;
	  }
	  break;

	case 's':			/* use stdio */
	case 'm':			/* use mmap  */
	  /* These options are implemented simply for
	   * compatibility with FreeBSD
	   */
	  break;

	default:
	  usage (stderr);
	  return 1;
	}
    }


  /* If the user gave the -d option or set LOCATE_PATH,
   * relinquish access to the secure database.
   */
  if (they_chose_db)
    {
      if (secure_db_fd >= 0)
	{
	  close(secure_db_fd);
	  secure_db_fd = -1;
	}
    }

  if (!just_count && !stats)
    print = 1;

  if (stats)
    {
      if (optind == argc)
	use_limit = 0;
    }
  else
    {
      if (!just_count && optind == argc)
	{
	  usage (stderr);
	  return 1;
	}
    }
  

  if (1 == isatty(STDOUT_FILENO))
    stdout_is_a_tty = true;
  else
    stdout_is_a_tty = false;

  if (they_chose_db)
    next_element (dbpath, 0);	/* Initialize.  */

  /* Bail out early if limit already reached. */
  while (!use_limit || limits.limit > limits.items_accepted)
    {
      struct stat st;
      int fd;
      off_t filesize;
      
      statistics.compressed_bytes = 
      statistics.total_filename_count = 
      statistics.total_filename_length = 
      statistics.whitespace_count = 
      statistics.newline_count = 
      statistics.highbit_filename_count = 0u;

      if (they_chose_db)
	{
	  /* Take the next element from the list of databases */
	  e = next_element ((char *) NULL, 0);
	  if (NULL == e)
	    break;
	  
	  if (0 == strcmp (e, "-"))
	    {
	      if (did_stdin)
		{
		  error (0, 0,
			 _("warning: the locate database can only be read from stdin once."));
		  return 0;
		}
	      else
		{
		  e = "<stdin>";
		  fd = 0;
		  did_stdin = true;
		}
	    }
	  else
	    {
	      if (0 == strlen(e) || 0 == strcmp(e, "."))
		{
		  e = LOCATE_DB;
		}
	  
	      /* open the database */
	      fd = opendb(e);
	      if (fd < 0)
		{
		  error (0, errno, "%s",
			 quotearg_n_style(0, locale_quoting_style, e));
		  return 0;
		}
	    }
	}
      else
	{
	  if (-1 == secure_db_fd)
	    {
	      /* Already searched the database, it's time to exit the loop */
	      break;
	    }
	  else
	    {
	      e = selected_secure_db;
	      fd = secure_db_fd;
	      secure_db_fd = -1;
	    }
	}
      
      /* Check the database to see if it is old. */
      if (fstat(fd, &st))
	{
	  error (0, errno, "%s",
		 quotearg_n_style(0, locale_quoting_style, e));
	  /* continue anyway */
	  filesize = (off_t)0;
	}
      else
	{
	  time_t now;
	  
	  filesize = st.st_size;
	  
	  if ((time_t)-1 == time(&now))
	    {
	      /* If we can't tell the time, we don't know how old the
	       * database is.  But since the message is just advisory,
	       * we continue anyway.
	       */
	      error (0, errno, _("time system call failed"));
	    }
	  else
	    {
	      double age          = difftime(now, st.st_mtime);
	      double warn_seconds = SECONDS_PER_UNIT * warn_number_units;
	      if (age > warn_seconds)
		{
		  /* For example:
		     warning: database `fred' is more than 8 days old (actual age is 10 days)*/
		  error (0, 0,
			 _("warning: database %s is more than %d %s old (actual age is %.1f %s)"),
			 quotearg_n_style(0,  locale_quoting_style, e), 
			 warn_number_units,              _(warn_name_units),
			 (age/(double)SECONDS_PER_UNIT), _(warn_name_units));
		}
	    }
	}

      fp = fdopen(fd, "r");
      if (NULL == fp)
	{
	  error (0, errno, "%s",
		 quotearg_n_style(0, locale_quoting_style, e));
	  return 0;
	}

      /* Search this database for all patterns simultaneously */
      found = search_one_database (argc - optind, &argv[optind],
				   e, fp, filesize,
				   ignore_case, print, basename_only,
				   use_limit, &limits, stats,
				   op_and, regex, regex_options);

      /* Close the databsase (even if it is stdin) */
      if (fclose (fp) == EOF)
	{
	  error (0, errno, "%s",
		 quotearg_n_style(0, locale_quoting_style, e));
	  return 0;
	}
    }
  
  if (just_count)
    {
      printf("%ld\n", found);
    }
  
  if (found || (use_limit && (limits.limit==0)) || stats )
    return 0;
  else
    return 1;
}

#define ARRAYSIZE(a) (sizeof(a)/sizeof(a[0]))
static int 
open_secure_db(void)
{
  int fd, i;
  
  const char * secure_db_list[] = 
    {
      LOCATE_DB,
      "/var/lib/slocate/slocate.db",
      NULL
    };
  for (i=0; secure_db_list[i]; ++i)
    {
      fd = opendb(secure_db_list[i]);
      if (fd >= 0)
	{
	  selected_secure_db = secure_db_list[i];
	  return fd;
	}
    }
  return -1;
}

int
main (int argc, char **argv)
{
  int dbfd = open_secure_db();
  drop_privs();
  
  return dolocate(argc, argv, dbfd);
}
