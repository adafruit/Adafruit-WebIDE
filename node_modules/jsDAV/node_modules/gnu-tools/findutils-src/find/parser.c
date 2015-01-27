/* parser.c -- convert the command line args into an expression tree.
   Copyright (C) 1990, 1991, 1992, 1993, 1994, 2000, 2001, 2003,
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

#include <config.h>

#include "defs.h"
#include <ctype.h>
#include <math.h>
#include <assert.h>
#include <pwd.h>
#include <errno.h>
#include <grp.h>
#include <fnmatch.h>
#include "modechange.h"
#include "modetype.h"
#include "xstrtol.h"
#include "xalloc.h"
#include "quote.h"
#include "quotearg.h"
#include "buildcmd.h"
#include "nextelem.h"
#include "stdio-safer.h"
#include "regextype.h"
#include "stat-time.h"
#include "xstrtod.h"
#include "fts_.h"
#include "getdate.h"
#include "error.h"
#include "findutils-version.h"

#include <fcntl.h>


/* The presence of unistd.h is assumed by gnulib these days, so we
 * might as well assume it too.
 */
/* We need <unistd.h> for isatty(). */
#include <unistd.h>
#include <sys/stat.h>

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

#if !defined (isascii) || defined (STDC_HEADERS)
#ifdef isascii
#undef isascii
#endif
#define isascii(c) 1
#endif

#define ISDIGIT(c) (isascii ((unsigned char)c) && isdigit ((unsigned char)c))

#ifndef HAVE_ENDGRENT
#define endgrent()
#endif
#ifndef HAVE_ENDPWENT
#define endpwent()
#endif

static boolean parse_accesscheck   PARAMS((const struct parser_table* entry, char **argv, int *arg_ptr));
static boolean parse_amin          PARAMS((const struct parser_table*, char *argv[], int *arg_ptr));
static boolean parse_and           PARAMS((const struct parser_table*, char *argv[], int *arg_ptr));
static boolean parse_anewer        PARAMS((const struct parser_table*, char *argv[], int *arg_ptr));
static boolean parse_cmin          PARAMS((const struct parser_table*, char *argv[], int *arg_ptr));
static boolean parse_cnewer        PARAMS((const struct parser_table*, char *argv[], int *arg_ptr));
static boolean parse_comma         PARAMS((const struct parser_table*, char *argv[], int *arg_ptr));
static boolean parse_daystart      PARAMS((const struct parser_table*, char *argv[], int *arg_ptr));
static boolean parse_delete        PARAMS((const struct parser_table*, char *argv[], int *arg_ptr));
static boolean parse_d             PARAMS((const struct parser_table*, char *argv[], int *arg_ptr));
static boolean parse_depth         PARAMS((const struct parser_table*, char *argv[], int *arg_ptr));
static boolean parse_empty         PARAMS((const struct parser_table*, char *argv[], int *arg_ptr));
static boolean parse_exec          PARAMS((const struct parser_table*, char *argv[], int *arg_ptr));
static boolean parse_execdir       PARAMS((const struct parser_table*, char *argv[], int *arg_ptr));
static boolean parse_false         PARAMS((const struct parser_table*, char *argv[], int *arg_ptr));
static boolean parse_fls           PARAMS((const struct parser_table*, char *argv[], int *arg_ptr));
static boolean parse_fprintf       PARAMS((const struct parser_table*, char *argv[], int *arg_ptr));
static boolean parse_follow        PARAMS((const struct parser_table*, char *argv[], int *arg_ptr));
static boolean parse_fprint        PARAMS((const struct parser_table*, char *argv[], int *arg_ptr));
static boolean parse_fprint0       PARAMS((const struct parser_table*, char *argv[], int *arg_ptr));
static boolean parse_fstype        PARAMS((const struct parser_table*, char *argv[], int *arg_ptr));
static boolean parse_gid           PARAMS((const struct parser_table*, char *argv[], int *arg_ptr));
static boolean parse_group         PARAMS((const struct parser_table*, char *argv[], int *arg_ptr));
static boolean parse_help          PARAMS((const struct parser_table*, char *argv[], int *arg_ptr));
static boolean parse_ilname        PARAMS((const struct parser_table*, char *argv[], int *arg_ptr));
static boolean parse_iname         PARAMS((const struct parser_table*, char *argv[], int *arg_ptr));
static boolean parse_inum          PARAMS((const struct parser_table*, char *argv[], int *arg_ptr));
static boolean parse_ipath         PARAMS((const struct parser_table*, char *argv[], int *arg_ptr));
static boolean parse_iregex        PARAMS((const struct parser_table*, char *argv[], int *arg_ptr));
static boolean parse_iwholename    PARAMS((const struct parser_table*, char *argv[], int *arg_ptr));
static boolean parse_links         PARAMS((const struct parser_table*, char *argv[], int *arg_ptr));
static boolean parse_lname         PARAMS((const struct parser_table*, char *argv[], int *arg_ptr));
static boolean parse_ls            PARAMS((const struct parser_table*, char *argv[], int *arg_ptr));
static boolean parse_maxdepth      PARAMS((const struct parser_table*, char *argv[], int *arg_ptr));
static boolean parse_mindepth      PARAMS((const struct parser_table*, char *argv[], int *arg_ptr));
static boolean parse_mmin          PARAMS((const struct parser_table*, char *argv[], int *arg_ptr));
static boolean parse_name          PARAMS((const struct parser_table*, char *argv[], int *arg_ptr));
static boolean parse_negate        PARAMS((const struct parser_table*, char *argv[], int *arg_ptr));
static boolean parse_newer         PARAMS((const struct parser_table*, char *argv[], int *arg_ptr));
static boolean parse_newerXY       PARAMS((const struct parser_table*, char *argv[], int *arg_ptr));
static boolean parse_noleaf        PARAMS((const struct parser_table*, char *argv[], int *arg_ptr));
static boolean parse_nogroup       PARAMS((const struct parser_table*, char *argv[], int *arg_ptr));
static boolean parse_nouser        PARAMS((const struct parser_table*, char *argv[], int *arg_ptr));
static boolean parse_nowarn        PARAMS((const struct parser_table*, char *argv[], int *arg_ptr));
static boolean parse_ok            PARAMS((const struct parser_table*, char *argv[], int *arg_ptr));
static boolean parse_okdir         PARAMS((const struct parser_table*, char *argv[], int *arg_ptr));
static boolean parse_or            PARAMS((const struct parser_table*, char *argv[], int *arg_ptr));
static boolean parse_path          PARAMS((const struct parser_table*, char *argv[], int *arg_ptr));
static boolean parse_perm          PARAMS((const struct parser_table*, char *argv[], int *arg_ptr));
static boolean parse_print0        PARAMS((const struct parser_table*, char *argv[], int *arg_ptr));
static boolean parse_printf        PARAMS((const struct parser_table*, char *argv[], int *arg_ptr));
static boolean parse_prune         PARAMS((const struct parser_table*, char *argv[], int *arg_ptr));
static boolean parse_regex         PARAMS((const struct parser_table*, char *argv[], int *arg_ptr));
static boolean parse_regextype     PARAMS((const struct parser_table*, char *argv[], int *arg_ptr));
static boolean parse_samefile      PARAMS((const struct parser_table*, char *argv[], int *arg_ptr));
#if 0
static boolean parse_show_control_chars PARAMS((const struct parser_table*, char *argv[], int *arg_ptr));
#endif
static boolean parse_size          PARAMS((const struct parser_table*, char *argv[], int *arg_ptr));
static boolean parse_time          PARAMS((const struct parser_table*, char *argv[], int *arg_ptr));
static boolean parse_true          PARAMS((const struct parser_table*, char *argv[], int *arg_ptr));
static boolean parse_type          PARAMS((const struct parser_table*, char *argv[], int *arg_ptr));
static boolean parse_uid           PARAMS((const struct parser_table*, char *argv[], int *arg_ptr));
static boolean parse_used          PARAMS((const struct parser_table*, char *argv[], int *arg_ptr));
static boolean parse_user          PARAMS((const struct parser_table*, char *argv[], int *arg_ptr));
static boolean parse_version       PARAMS((const struct parser_table*, char *argv[], int *arg_ptr));
static boolean parse_wholename     PARAMS((const struct parser_table*, char *argv[], int *arg_ptr));
static boolean parse_xdev          PARAMS((const struct parser_table*, char *argv[], int *arg_ptr));
static boolean parse_ignore_race   PARAMS((const struct parser_table*, char *argv[], int *arg_ptr));
static boolean parse_noignore_race PARAMS((const struct parser_table*, char *argv[], int *arg_ptr));
static boolean parse_warn          PARAMS((const struct parser_table*, char *argv[], int *arg_ptr));
static boolean parse_xtype         PARAMS((const struct parser_table*, char *argv[], int *arg_ptr));
static boolean parse_quit          PARAMS((const struct parser_table*, char *argv[], int *arg_ptr));

boolean parse_print             PARAMS((const struct parser_table*, char *argv[], int *arg_ptr));


static boolean insert_type PARAMS((char **argv, int *arg_ptr,
				   const struct parser_table *entry,
				   PRED_FUNC which_pred));
static boolean insert_regex PARAMS((char *argv[], int *arg_ptr,
				    const struct parser_table *entry,
				    int regex_options));
static boolean insert_fprintf (struct format_val *vec,
			       const struct parser_table *entry,
			       PRED_FUNC func,
			       const char *format);

static struct segment **make_segment PARAMS((struct segment **segment,
					     char *format, int len,
					     int kind, char format_char,
					     char aux_format_char,
					     struct predicate *pred));
static boolean insert_exec_ok PARAMS((const char *action,
				      const struct parser_table *entry,
				      int dir_fd,
				      char *argv[],
				      int *arg_ptr));
static boolean get_comp_type PARAMS((const char **str,
				     enum comparison_type *comp_type));
static boolean get_relative_timestamp PARAMS((const char *str,
					      struct time_val *tval,
					      struct timespec origin,
					      double sec_per_unit,
					      const char *overflowmessage));
static boolean get_num PARAMS((const char *str,
			       uintmax_t *num,
			       enum comparison_type *comp_type));
static struct predicate* insert_num PARAMS((char *argv[], int *arg_ptr,
					    const struct parser_table *entry));
static void open_output_file (const char *path, struct format_val *p);
static void open_stdout (struct format_val *p);
static boolean stream_is_tty(FILE *fp);
static boolean parse_noop PARAMS((const struct parser_table* entry,
				  char **argv, int *arg_ptr));

#define PASTE(x,y) x##y
#define STRINGIFY(s) #s

#define PARSE_OPTION(what,suffix) \
  { (ARG_OPTION), (what), PASTE(parse_,suffix), NULL }

#define PARSE_POSOPT(what,suffix) \
  { (ARG_POSITIONAL_OPTION), (what), PASTE(parse_,suffix), NULL }

#define PARSE_TEST(what,suffix) \
  { (ARG_TEST), (what), PASTE(parse_,suffix), PASTE(pred_,suffix) }

#define PARSE_TEST_NP(what,suffix) \
  { (ARG_TEST), (what), PASTE(parse_,suffix), NULL }

#define PARSE_ACTION(what,suffix) \
  { (ARG_ACTION), (what), PASTE(parse_,suffix), PASTE(pred_,suffix) }

#define PARSE_ACTION_NP(what,suffix) \
  { (ARG_ACTION), (what), PASTE(parse_,suffix), NULL }

#define PARSE_PUNCTUATION(what,suffix) \
  { (ARG_PUNCTUATION), (what), PASTE(parse_,suffix), PASTE(pred_,suffix) }


/* Predicates we cannot handle in the usual way.  If you add an entry
 * to this table, double-check the switch statement in
 * pred_sanity_check() to make sure that the new case is being
 * correctly handled.
 */
static struct parser_table const parse_entry_newerXY =
  {
    ARG_SPECIAL_PARSE, "newerXY",            parse_newerXY, pred_newerXY /* BSD  */
  };

/* GNU find predicates that are not mentioned in POSIX.2 are marked `GNU'.
   If they are in some Unix versions of find, they are marked `Unix'. */

static struct parser_table const parse_table[] =
{
  PARSE_PUNCTUATION("!",                     negate), /* POSIX */
  PARSE_PUNCTUATION("not",                   negate),	     /* GNU */
  PARSE_PUNCTUATION("(",                     openparen), /* POSIX */
  PARSE_PUNCTUATION(")",                     closeparen), /* POSIX */
  PARSE_PUNCTUATION(",",                     comma),	     /* GNU */
  PARSE_PUNCTUATION("a",                     and), /* POSIX */
  PARSE_TEST       ("amin",                  amin),	     /* GNU */
  PARSE_PUNCTUATION("and",                   and),		/* GNU */
  PARSE_TEST       ("anewer",                anewer),	     /* GNU */
  {ARG_TEST,       "atime",                  parse_time, pred_atime}, /* POSIX */
  PARSE_TEST       ("cmin",                  cmin),	     /* GNU */
  PARSE_TEST       ("cnewer",                cnewer),	     /* GNU */
  {ARG_TEST,       "ctime",                  parse_time, pred_ctime}, /* POSIX */
  PARSE_POSOPT     ("daystart",              daystart),	     /* GNU */
  PARSE_ACTION     ("delete",                delete), /* GNU, Mac OS, FreeBSD */
  PARSE_OPTION     ("d",                     d), /* Mac OS X, FreeBSD, NetBSD, OpenBSD, but deprecated  in favour of -depth */
  PARSE_OPTION     ("depth",                 depth), /* POSIX */
  PARSE_TEST       ("empty",                 empty),	     /* GNU */
  {ARG_ACTION,      "exec",    parse_exec, pred_exec}, /* POSIX */
  {ARG_TEST,        "executable",            parse_accesscheck, pred_executable}, /* GNU, 4.3.0+ */
  PARSE_ACTION     ("execdir",               execdir), /* *BSD, GNU */
  PARSE_ACTION     ("fls",                   fls),	     /* GNU */
  PARSE_POSOPT     ("follow",                follow),  /* GNU, Unix */
  PARSE_ACTION     ("fprint",                fprint),	     /* GNU */
  PARSE_ACTION     ("fprint0",               fprint0),	     /* GNU */
  {ARG_ACTION,      "fprintf", parse_fprintf, pred_fprintf}, /* GNU */
  PARSE_TEST       ("fstype",                fstype),  /* GNU, Unix */
  PARSE_TEST       ("gid",                   gid),	     /* GNU */
  PARSE_TEST       ("group",                 group), /* POSIX */
  PARSE_OPTION     ("ignore_readdir_race",   ignore_race),   /* GNU */
  PARSE_TEST       ("ilname",                ilname),	     /* GNU */
  PARSE_TEST       ("iname",                 iname),	     /* GNU */
  PARSE_TEST       ("inum",                  inum),    /* GNU, Unix */
  PARSE_TEST       ("ipath",                 ipath), /* GNU, deprecated in favour of iwholename */
  PARSE_TEST_NP    ("iregex",                iregex),	     /* GNU */
  PARSE_TEST_NP    ("iwholename",            iwholename),    /* GNU */
  PARSE_TEST       ("links",                 links), /* POSIX */
  PARSE_TEST       ("lname",                 lname),	     /* GNU */
  PARSE_ACTION     ("ls",                    ls),      /* GNU, Unix */
  PARSE_OPTION     ("maxdepth",              maxdepth),	     /* GNU */
  PARSE_OPTION     ("mindepth",              mindepth),	     /* GNU */
  PARSE_TEST       ("mmin",                  mmin),	     /* GNU */
  PARSE_OPTION     ("mount",                 xdev),	    /* Unix */
  {ARG_TEST,       "mtime",                  parse_time, pred_mtime}, /* POSIX */
  PARSE_TEST       ("name",                  name),
#ifdef UNIMPLEMENTED_UNIX
  PARSE(ARG_UNIMPLEMENTED, "ncpio",          ncpio),	    /* Unix */
#endif
  PARSE_TEST       ("newer",                 newer), /* POSIX */
  {ARG_TEST,       "atime",                  parse_time, pred_atime}, /* POSIX */
  PARSE_OPTION     ("noleaf",                noleaf),	     /* GNU */
  PARSE_TEST       ("nogroup",               nogroup), /* POSIX */
  PARSE_TEST       ("nouser",                nouser), /* POSIX */
  PARSE_OPTION     ("noignore_readdir_race", noignore_race), /* GNU */
  PARSE_POSOPT     ("nowarn",                nowarn),	     /* GNU */
  PARSE_PUNCTUATION("o",                     or), /* POSIX */
  PARSE_PUNCTUATION("or",                    or),	     /* GNU */
  PARSE_ACTION     ("ok",                    ok), /* POSIX */
  PARSE_ACTION     ("okdir",                 okdir), /* GNU (-execdir is BSD) */
  PARSE_TEST       ("path",                  path), /* GNU, HP-UX, RMS prefers wholename, but anyway soon POSIX */
  PARSE_TEST       ("perm",                  perm), /* POSIX */
  PARSE_ACTION     ("print",                 print), /* POSIX */
  PARSE_ACTION     ("print0",                print0),	     /* GNU */
  {ARG_ACTION,      "printf",   parse_printf, NULL},	     /* GNU */
  PARSE_ACTION     ("prune",                 prune), /* POSIX */
  PARSE_ACTION     ("quit",                  quit),	     /* GNU */
  {ARG_TEST,       "readable",            parse_accesscheck, pred_readable}, /* GNU, 4.3.0+ */
  PARSE_TEST       ("regex",                 regex),	     /* GNU */
  PARSE_POSOPT     ("regextype",             regextype),     /* GNU */
  PARSE_TEST       ("samefile",              samefile),	     /* GNU */
#if 0
  PARSE_OPTION     ("show-control-chars",    show_control_chars), /* GNU, 4.3.0+ */
#endif
  PARSE_TEST       ("size",                  size), /* POSIX */
  PARSE_TEST       ("type",                  type), /* POSIX */
  PARSE_TEST       ("uid",                   uid),	     /* GNU */
  PARSE_TEST       ("used",                  used),	     /* GNU */
  PARSE_TEST       ("user",                  user), /* POSIX */
  PARSE_OPTION     ("warn",                  warn),	     /* GNU */
  PARSE_TEST_NP    ("wholename",             wholename), /* GNU, replaced -path, but anyway -path will soon be in POSIX */
  {ARG_TEST,       "writable",               parse_accesscheck, pred_writable}, /* GNU, 4.3.0+ */
  PARSE_OPTION     ("xdev",                  xdev), /* POSIX */
  PARSE_TEST       ("xtype",                 xtype),	     /* GNU */
#ifdef UNIMPLEMENTED_UNIX
  /* It's pretty ugly for find to know about archive formats.
     Plus what it could do with cpio archives is very limited.
     Better to leave it out.  */
  PARSE(ARG_UNIMPLEMENTED,      "cpio",                  cpio),	/* Unix */
#endif
  /* gnulib's stdbool.h might have made true and false into macros,
   * so we can't leave named 'true' and 'false' tokens, so we have
   * to expeant the relevant entries longhand.
   */
  {ARG_TEST, "false",                 parse_false,   pred_false}, /* GNU */
  {ARG_TEST, "true",                  parse_true,    pred_true }, /* GNU */
  {ARG_NOOP, "noop",                  NULL,          pred_true }, /* GNU, internal use only */

  /* Various other cases that don't fit neatly into our macro scheme. */
  {ARG_TEST, "help",                  parse_help,    NULL},       /* GNU */
  {ARG_TEST, "-help",                 parse_help,    NULL},       /* GNU */
  {ARG_TEST, "version",               parse_version, NULL},	  /* GNU */
  {ARG_TEST, "-version",              parse_version, NULL},	  /* GNU */
  {0, 0, 0, 0}
};


static const char *first_nonoption_arg = NULL;
static const struct parser_table *noop = NULL;


void
check_option_combinations(const struct predicate *p)
{
  enum { seen_delete=1u, seen_prune=2u };
  unsigned int predicates = 0u;

  while (p)
    {
      if (p->pred_func == pred_delete)
	predicates |= seen_delete;
      else if (p->pred_func == pred_prune)
	predicates |= seen_prune;
      p = p->pred_next;
    }

  if ((predicates & seen_prune) && (predicates & seen_delete))
    {
      /* The user specified both -delete and -prune.  One might test
       * this by first doing
       *    find dirs   .... -prune ..... -print
       * to fnd out what's going to get deleted, and then switch to
       *    find dirs   .... -prune ..... -delete
       * once we are happy.  Unfortunately, the -delete action also
       * implicitly turns on -depth, which will affect the behaviour
       * of -prune (in fact, it makes it a no-op).  In this case we
       * would like to prevent unfortunate accidents, so we require
       * the user to have explicitly used -depth.
       *
       * We only get away with this because the -delete predicate is not
       * in POSIX.   If it was, we couldn't issue a fatal error here.
       */
      if (!options.explicit_depth)
	{
	  /* This fixes Savannah bug #20865. */
	  error (1, 0, _("The -delete action atomatically turns on -depth, "
			 "but -prune does nothing when -depth is in effect.  "
			 "If you want to carry on anyway, just explicitly use "
			 "the -depth option."));
	}
    }
}


static const struct parser_table*
get_noop(void)
{
  int i;
  if (NULL == noop)
    {
      for (i = 0; parse_table[i].parser_name != 0; i++)
	{
	  if (ARG_NOOP ==parse_table[i].type)
	    {
	      noop = &(parse_table[i]);
	      break;
	    }
	}
    }
  return noop;
}

static int
get_stat_Ytime(const struct stat *p,
	       char what,
	       struct timespec *ret)
{
  switch (what)
    {
    case 'a':
      *ret = get_stat_atime(p);
      return 1;
    case 'B':
      *ret = get_stat_birthtime(p);
      return (ret->tv_nsec >= 0);
    case 'c':
      *ret = get_stat_ctime(p);
      return 1;
    case 'm':
      *ret = get_stat_mtime(p);
      return 1;
    default:
      assert (0);
      abort();
    }
}

void
set_follow_state(enum SymlinkOption opt)
{
  if (options.debug_options & DebugStat)
    {
      /* For DebugStat, the choice is made at runtime within debug_stat()
       * by checking the contents of the symlink_handling variable.
       */
      options.xstat = debug_stat;
    }
  else
    {
      switch (opt)
	{
	case SYMLINK_ALWAYS_DEREF:  /* -L */
	  options.xstat = optionl_stat;
	  options.no_leaf_check = true;
	  break;

	case SYMLINK_NEVER_DEREF:	/* -P (default) */
	  options.xstat = optionp_stat;
	  /* Can't turn no_leaf_check off because the user might have specified
	   * -noleaf anyway
	   */
	  break;

	case SYMLINK_DEREF_ARGSONLY: /* -H */
	  options.xstat = optionh_stat;
	  options.no_leaf_check = true;
	}
    }
  options.symlink_handling = opt;
}


void
parse_begin_user_args (char **args, int argno,
		       const struct predicate *last,
		       const struct predicate *predicates)
{
  (void) args;
  (void) argno;
  (void) last;
  (void) predicates;
  first_nonoption_arg = NULL;
}

void
parse_end_user_args (char **args, int argno,
		     const struct predicate *last,
		     const struct predicate *predicates)
{
  /* does nothing */
  (void) args;
  (void) argno;
  (void) last;
  (void) predicates;
}


/* Check that it is legal to fid the given primary in its
 * position and return it.
 */
const struct parser_table*
found_parser(const char *original_arg, const struct parser_table *entry)
{
  /* If this is an option, but we have already had a
   * non-option argument, the user may be under the
   * impression that the behaviour of the option
   * argument is conditional on some preceding
   * tests.  This might typically be the case with,
   * for example, -maxdepth.
   *
   * The options -daystart and -follow are exempt
   * from this treatment, since their positioning
   * in the command line does have an effect on
   * subsequent tests but not previous ones.  That
   * might be intentional on the part of the user.
   */
  if (entry->type != ARG_POSITIONAL_OPTION)
    {
      /* Something other than -follow/-daystart.
       * If this is an option, check if it followed
       * a non-option and if so, issue a warning.
       */
      if (entry->type == ARG_OPTION)
	{
	  if ((first_nonoption_arg != NULL)
	      && options.warnings )
	    {
	      /* option which follows a non-option */
	      error (0, 0,
		     _("warning: you have specified the %s "
		       "option after a non-option argument %s, "
		       "but options are not positional (%s affects "
		       "tests specified before it as well as those "
		       "specified after it).  Please specify options "
		       "before other arguments.\n"),
		     original_arg,
		     first_nonoption_arg,
		     original_arg);
	    }
	}
      else
	{
	  /* Not an option or a positional option,
	   * so remember we've seen it in order to
	   * use it in a possible future warning message.
	   */
	  if (first_nonoption_arg == NULL)
	    {
	      first_nonoption_arg = original_arg;
	    }
	}
    }

  return entry;
}


/* Return a pointer to the parser function to invoke for predicate
   SEARCH_NAME.
   Return NULL if SEARCH_NAME is not a valid predicate name. */

const struct parser_table*
find_parser (char *search_name)
{
  int i;
  const char *original_arg = search_name;

  /* Ugh.  Special case -newerXY. */
  if (0 == strncmp("-newer", search_name, 6)
      && (8 == strlen(search_name)))
    {
      return found_parser(original_arg, &parse_entry_newerXY);
    }

  if (*search_name == '-')
    search_name++;

  for (i = 0; parse_table[i].parser_name != 0; i++)
    {
      if (strcmp (parse_table[i].parser_name, search_name) == 0)
	{
	  return found_parser(original_arg, &parse_table[i]);
	}
    }
  return NULL;
}

static float
estimate_file_age_success_rate(float num_days)
{
  if (num_days < 0.1)
    {
      /* Assume 1% of files have timestamps in the future */
      return 0.01f;
    }
  else if (num_days < 1)
    {
      /* Assume 30% of files have timestamps today */
      return 0.3f;
    }
  else if (num_days > 100)
    {
      /* Assume 30% of files are very old */
      return 0.3f;
    }
  else
    {
      /* Assume 39% of files are between 1 and 100 days old. */
      return 0.39f;
    }
}

static float
estimate_timestamp_success_rate(time_t when)
{
  /* This calculation ignores the nanoseconds field of the
   * origin, but I don't think that makes much difference
   * to our estimate.
   */
  int num_days = (options.cur_day_start.tv_sec - when) / 86400;
  return estimate_file_age_success_rate(num_days);
}

/* Collect an argument from the argument list, or
 * return false.
 */
static boolean
collect_arg(char **argv, int *arg_ptr, const char **collected_arg)
{
  if ((argv == NULL) || (argv[*arg_ptr] == NULL))
    {
      *collected_arg = NULL;
      return false;
    }
  else
    {
      *collected_arg = argv[*arg_ptr];
      (*arg_ptr)++;
      return true;
    }
}

static boolean
collect_arg_stat_info(char **argv, int *arg_ptr, struct stat *p)
{
  const char *filename;
  if (collect_arg(argv, arg_ptr, &filename))
    {
      if (0 == (options.xstat)(filename, p))
	{
	  return true;
	}
      else
	{
	  fatal_file_error(filename);
	}
    }
  else
    {
      return false;
    }
}

/* The parsers are responsible to continue scanning ARGV for
   their arguments.  Each parser knows what is and isn't
   allowed for itself.

   ARGV is the argument array.
   *ARG_PTR is the index to start at in ARGV,
   updated to point beyond the last element consumed.

   The predicate structure is updated with the new information. */


static boolean
parse_and (const struct parser_table* entry, char **argv, int *arg_ptr)
{
  struct predicate *our_pred;

  (void) argv;
  (void) arg_ptr;

  our_pred = get_new_pred (entry);
  our_pred->pred_func = pred_and;
  our_pred->p_type = BI_OP;
  our_pred->p_prec = AND_PREC;
  our_pred->need_stat = our_pred->need_type = false;
  return true;
}

static boolean
parse_anewer (const struct parser_table* entry, char **argv, int *arg_ptr)
{
  struct stat stat_newer;

  set_stat_placeholders(&stat_newer);
  if (collect_arg_stat_info(argv, arg_ptr, &stat_newer))
    {
      struct predicate *our_pred = insert_primary (entry);
      our_pred->args.reftime.xval = XVAL_ATIME;
      our_pred->args.reftime.ts = get_stat_mtime(&stat_newer);
      our_pred->args.reftime.kind = COMP_GT;
      our_pred->est_success_rate = estimate_timestamp_success_rate(stat_newer.st_mtime);
      return true;
    }
  return false;
}

boolean
parse_closeparen (const struct parser_table* entry, char **argv, int *arg_ptr)
{
  struct predicate *our_pred;

  (void) argv;
  (void) arg_ptr;

  our_pred = get_new_pred (entry);
  our_pred->pred_func = pred_closeparen;
  our_pred->p_type = CLOSE_PAREN;
  our_pred->p_prec = NO_PREC;
  our_pred->need_stat = our_pred->need_type = false;
  return true;
}

static boolean
parse_cnewer (const struct parser_table* entry, char **argv, int *arg_ptr)
{
  struct stat stat_newer;

  set_stat_placeholders(&stat_newer);
  if (collect_arg_stat_info(argv, arg_ptr, &stat_newer))
    {
      struct predicate *our_pred = insert_primary (entry);
      our_pred->args.reftime.xval = XVAL_CTIME; /* like -newercm */
      our_pred->args.reftime.ts = get_stat_mtime(&stat_newer);
      our_pred->args.reftime.kind = COMP_GT;
      our_pred->est_success_rate = estimate_timestamp_success_rate(stat_newer.st_mtime);
      return true;
    }
  return false;
}

static boolean
parse_comma (const struct parser_table* entry, char **argv, int *arg_ptr)
{
  struct predicate *our_pred;

  (void) argv;
  (void) arg_ptr;

  our_pred = get_new_pred (entry);
  our_pred->pred_func = pred_comma;
  our_pred->p_type = BI_OP;
  our_pred->p_prec = COMMA_PREC;
  our_pred->need_stat = our_pred->need_type = false;
  our_pred->est_success_rate = 1.0f;
  return true;
}

static boolean
parse_daystart (const struct parser_table* entry, char **argv, int *arg_ptr)
{
  struct tm *local;

  (void) entry;
  (void) argv;
  (void) arg_ptr;

  if (options.full_days == false)
    {
      options.cur_day_start.tv_sec += DAYSECS;
      options.cur_day_start.tv_nsec = 0;
      local = localtime (&options.cur_day_start.tv_sec);
      options.cur_day_start.tv_sec -= (local
				       ? (local->tm_sec + local->tm_min * 60
					  + local->tm_hour * 3600)
				       : options.cur_day_start.tv_sec % DAYSECS);
      options.full_days = true;
    }
  return true;
}

static boolean
parse_delete (const struct parser_table* entry, char *argv[], int *arg_ptr)
{
  struct predicate *our_pred;
  (void) argv;
  (void) arg_ptr;

  our_pred = insert_primary (entry);
  our_pred->side_effects = our_pred->no_default_print = true;
  /* -delete implies -depth */
  options.do_dir_first = false;

  /* We do not need stat information because we check for the case
   * (errno==EISDIR) in pred_delete.
   */
  our_pred->need_stat = our_pred->need_type = false;

  our_pred->est_success_rate = 1.0f;
  return true;
}

static boolean
parse_depth (const struct parser_table* entry, char **argv, int *arg_ptr)
{
  (void) entry;
  (void) argv;

  options.do_dir_first = false;
  options.explicit_depth = true;
  return parse_noop(entry, argv, arg_ptr);
}

static boolean
parse_d (const struct parser_table* entry, char **argv, int *arg_ptr)
{
  if (options.warnings)
    {
      error (0, 0,
	     _("warning: the -d option is deprecated; please use "
	       "-depth instead, because the latter is a "
	       "POSIX-compliant feature."));
    }
  return parse_depth(entry, argv, arg_ptr);
}

static boolean
parse_empty (const struct parser_table* entry, char **argv, int *arg_ptr)
{
  struct predicate *our_pred;
  (void) argv;
  (void) arg_ptr;

  our_pred = insert_primary (entry);
  our_pred->est_success_rate = 0.01f; /* assume 1% of files are empty. */
  return true;
}

static boolean
parse_exec (const struct parser_table* entry, char **argv, int *arg_ptr)
{
  return insert_exec_ok ("-exec", entry, get_start_dirfd(), argv, arg_ptr);
}

static boolean
parse_execdir (const struct parser_table* entry, char **argv, int *arg_ptr)
{
  return insert_exec_ok ("-execdir", entry, -1, argv, arg_ptr);
}

static boolean
parse_false (const struct parser_table* entry, char **argv, int *arg_ptr)
{
  struct predicate *our_pred;

  (void) argv;
  (void) arg_ptr;

  our_pred = insert_primary (entry);
  our_pred->need_stat = our_pred->need_type = false;
  our_pred->side_effects = our_pred->no_default_print = false;
  our_pred->est_success_rate = 0.0f;
  return true;
}

static boolean
insert_fls (const struct parser_table* entry, const char *filename)
{
  struct predicate *our_pred = insert_primary (entry);
  if (filename)
    open_output_file (filename, &our_pred->args.printf_vec);
  else
    open_stdout (&our_pred->args.printf_vec);
  our_pred->side_effects = our_pred->no_default_print = true;
  our_pred->est_success_rate = 1.0f;
  return true;
}


static boolean
parse_fls (const struct parser_table* entry, char **argv, int *arg_ptr)
{
  const char *filename;
  return collect_arg(argv, arg_ptr, &filename)
    &&  insert_fls(entry, filename);
}

static boolean
parse_follow (const struct parser_table* entry, char **argv, int *arg_ptr)
{
  set_follow_state(SYMLINK_ALWAYS_DEREF);
  return parse_noop(entry, argv, arg_ptr);
}

static boolean
parse_fprint (const struct parser_table* entry, char **argv, int *arg_ptr)
{
  struct predicate *our_pred;
  const char *filename;
  if (collect_arg(argv, arg_ptr, &filename))
    {
      our_pred = insert_primary (entry);
      open_output_file (filename, &our_pred->args.printf_vec);
      our_pred->side_effects = our_pred->no_default_print = true;
      our_pred->need_stat = our_pred->need_type = false;
      our_pred->est_success_rate = 1.0f;
      return true;
    }
  else
    {
      return false;
    }
}

static boolean
insert_fprint(const struct parser_table* entry, const char *filename)
{
  struct predicate *our_pred = insert_primary (entry);
  if (filename)
    open_output_file (filename, &our_pred->args.printf_vec);
  else
    open_stdout (&our_pred->args.printf_vec);
  our_pred->side_effects = our_pred->no_default_print = true;
  our_pred->need_stat = our_pred->need_type = false;
  our_pred->est_success_rate = 1.0f;
  return true;
}


static boolean
parse_fprint0 (const struct parser_table* entry, char **argv, int *arg_ptr)
{
  const char *filename;
  if (collect_arg(argv, arg_ptr, &filename))
    return insert_fprint(entry, filename);
  else
    return false;
}

static float estimate_fstype_success_rate(const char *fsname)
{
  struct stat dir_stat;
  const char *dir = "/";
  if (0 == stat(dir, &dir_stat))
    {
      const char *fstype = filesystem_type(&dir_stat, dir);
      /* Assume most files are on the same file system type as the root fs. */
      if (0 == strcmp(fsname, fstype))
	  return 0.7f;
      else
	return 0.3f;
    }
  return 1.0f;
}


static boolean
parse_fstype (const struct parser_table* entry, char **argv, int *arg_ptr)
{
  const char *typename;
  if (collect_arg(argv, arg_ptr, &typename))
    {
      struct predicate *our_pred = insert_primary (entry);
      our_pred->args.str = typename;

      /* This is an expensive operation, so although there are
       * circumstances where it is selective, we ignore this fact
       * because we probably don't want to promote this test to the
       * front anyway.
       */
      our_pred->est_success_rate = estimate_fstype_success_rate(typename);
      return true;
    }
  else
    {
      return false;
    }
}

static boolean
parse_gid (const struct parser_table* entry, char **argv, int *arg_ptr)
{
  struct predicate *p = insert_num (argv, arg_ptr, entry);
  if (p)
    {
      p->est_success_rate = (p->args.numinfo.l_val < 100) ? 0.99 : 0.2;
      return true;
    }
  else
    {
      return false;
    }
}


static int
safe_atoi (const char *s)
{
  long lval;
  char *end;

  errno = 0;
  lval = strtol(s, &end, 10);
  if ( (LONG_MAX == lval) || (LONG_MIN == lval) )
    {
      /* max/min possible value, or an error. */
      if (errno == ERANGE)
	{
	  /* too big, or too small. */
	  error(1, errno, "%s", s);
	}
      else
	{
	  /* not a valid number */
	  error(1, errno, "%s", s);
	}
      /* Otherwise, we do a range chack against INT_MAX and INT_MIN
       * below.
       */
    }

  if (lval > INT_MAX || lval < INT_MIN)
    {
      /* The number was in range for long, but not int. */
      errno = ERANGE;
      error(1, errno, "%s", s);
    }
  else if (*end)
    {
      error(1, errno, "Unexpected suffix %s on %s",
	    quotearg_n_style(0, options.err_quoting_style, end),
	    quotearg_n_style(1, options.err_quoting_style, s));
    }
  else if (end == s)
    {
      error(1, errno, "Expected an integer: %s",
	    quotearg_n_style(0, options.err_quoting_style, s));
    }
  return (int)lval;
}


static boolean
parse_group (const struct parser_table* entry, char **argv, int *arg_ptr)
{
  const char *groupname;

  if (collect_arg(argv, arg_ptr, &groupname))
    {
      gid_t gid;
      struct predicate *our_pred;
      struct group *cur_gr = getgrnam(groupname);
      endgrent();
      if (cur_gr)
	{
	  gid = cur_gr->gr_gid;
	}
      else
	{
	  const int gid_len = strspn (groupname, "0123456789");
	  if (gid_len)
	    {
	      if (groupname[gid_len] == 0)
		{
		  gid = safe_atoi (groupname);
		}
	      else
		{
		  /* XXX: no test in test suite for this */
		  error(1, 0, _("%s is not the name of an existing group and"
				" it does not look like a numeric group ID "
				"because it has the unexpected suffix %s"),
			quotearg_n_style(0, options.err_quoting_style, groupname),
			quotearg_n_style(1, options.err_quoting_style, groupname+gid_len));
		  return false;
		}
	    }
	  else
	    {
	      if (*groupname)
		{
		  /* XXX: no test in test suite for this */
		  error(1, 0, _("%s is not the name of an existing group"),
			quotearg_n_style(0, options.err_quoting_style, groupname));
		}
	      else
		{
		  error(1, 0, _("argument to -group is empty, but should be a group name"));
		}
	      return false;
	    }
	}
      our_pred = insert_primary (entry);
      our_pred->args.gid = gid;
      our_pred->est_success_rate = (our_pred->args.numinfo.l_val < 100) ? 0.99 : 0.2;
      return true;
    }
  return false;
}

static boolean
parse_help (const struct parser_table* entry, char **argv, int *arg_ptr)
{
  (void) entry;
  (void) argv;
  (void) arg_ptr;

  usage(stdout, 0, NULL);
  puts (_("\n\
default path is the current directory; default expression is -print\n\
expression may consist of: operators, options, tests, and actions:\n"));
  puts (_("\
operators (decreasing precedence; -and is implicit where no others are given):\n\
      ( EXPR )   ! EXPR   -not EXPR   EXPR1 -a EXPR2   EXPR1 -and EXPR2\n\
      EXPR1 -o EXPR2   EXPR1 -or EXPR2   EXPR1 , EXPR2\n"));
  puts (_("\
positional options (always true): -daystart -follow -regextype\n\n\
normal options (always true, specified before other expressions):\n\
      -depth --help -maxdepth LEVELS -mindepth LEVELS -mount -noleaf\n\
      --version -xdev -ignore_readdir_race -noignore_readdir_race\n"));
  puts (_("\
tests (N can be +N or -N or N): -amin N -anewer FILE -atime N -cmin N\n\
      -cnewer FILE -ctime N -empty -false -fstype TYPE -gid N -group NAME\n\
      -ilname PATTERN -iname PATTERN -inum N -iwholename PATTERN -iregex PATTERN\n\
      -links N -lname PATTERN -mmin N -mtime N -name PATTERN -newer FILE"));
  puts (_("\
      -nouser -nogroup -path PATTERN -perm [+-]MODE -regex PATTERN\n\
      -readable -writable -executable\n\
      -wholename PATTERN -size N[bcwkMG] -true -type [bcdpflsD] -uid N\n\
      -used N -user NAME -xtype [bcdpfls]\n"));
  puts (_("\
actions: -delete -print0 -printf FORMAT -fprintf FILE FORMAT -print \n\
      -fprint0 FILE -fprint FILE -ls -fls FILE -prune -quit\n\
      -exec COMMAND ; -exec COMMAND {} + -ok COMMAND ;\n\
      -execdir COMMAND ; -execdir COMMAND {} + -okdir COMMAND ;\n\
"));
  puts (_("Report (and track progress on fixing) bugs via the findutils bug-reporting\n\
page at http://savannah.gnu.org/ or, if you have no web access, by sending\n\
email to <bug-findutils@gnu.org>."));
  exit (0);
}

static float
estimate_pattern_match_rate(const char *pattern, int is_regex)
{
  if (strpbrk(pattern, "*?[") || (is_regex && strpbrk(pattern, ".")))
    {
      /* A wildcard; assume the pattern matches most files. */
      return 0.8f;
    }
  else
    {
      return 0.1f;
    }
}

static boolean
parse_ilname (const struct parser_table* entry, char **argv, int *arg_ptr)
{
  const char *name;
  if (collect_arg(argv, arg_ptr, &name))
    {
      struct predicate *our_pred = insert_primary (entry);
      our_pred->args.str = name;
      /* Use the generic glob pattern estimator to figure out how many
       * links will match, but bear in mind that most files won't be links.
       */
      our_pred->est_success_rate = 0.1 * estimate_pattern_match_rate(name, 0);
      return true;
    }
  else
    {
      return false;
    }
}


/* sanity check the fnmatch() function to make sure that case folding
 * is supported (as opposed to just having the flag ignored).
 */
static boolean
fnmatch_sanitycheck(void)
{
  static boolean checked = false;
  if (!checked)
    {
      if (0 != fnmatch("foo", "foo", 0)
	  || 0 == fnmatch("Foo", "foo", 0)
	  || 0 != fnmatch("Foo", "foo", FNM_CASEFOLD))
	{
	  error (1, 0, _("sanity check of the fnmatch() library function failed."));
	  return false;
	}
      checked = true;
    }
  return checked;
}


static boolean
check_name_arg(const char *pred, const char *arg)
{
  if (options.warnings && strchr(arg, '/'))
    {
      error(0, 0,_("warning: Unix filenames usually don't contain slashes "
		   "(though pathnames do).  That means that '%s %s' will "
		   "probably evaluate to false all the time on this system.  "
		   "You might find the '-wholename' test more useful, or "
		   "perhaps '-samefile'.  Alternatively, if you are using "
		   "GNU grep, you could "
		   "use 'find ... -print0 | grep -FzZ %s'."),
	    pred,
	    safely_quote_err_filename(0, arg),
	    safely_quote_err_filename(1, arg));
    }
  return true;			/* allow it anyway */
}



static boolean
parse_iname (const struct parser_table* entry, char **argv, int *arg_ptr)
{
  const char *name;
  fnmatch_sanitycheck();
  if (collect_arg(argv, arg_ptr, &name))
    {
      if (check_name_arg("-iname", name))
	{
	  struct predicate *our_pred = insert_primary (entry);
	  our_pred->need_stat = our_pred->need_type = false;
	  our_pred->args.str = name;
	  our_pred->est_success_rate = estimate_pattern_match_rate(name, 0);
	  return true;
	}
    }
  return false;
}

static boolean
parse_inum (const struct parser_table* entry, char **argv, int *arg_ptr)
{
  struct predicate *p =  insert_num (argv, arg_ptr, entry);
  if (p)
    {
      /* inode number is exact match only, so very low proportions of
       * files match
       */
      p->est_success_rate = 1e-6;
      return true;
    }
  else
    {
      return false;
    }
}

/* -ipath is deprecated (at RMS's request) in favour of
 * -iwholename.   See the node "GNU Manuals" in standards.texi
 * for the rationale for this (basically, GNU prefers the use
 * of the phrase "file name" to "path name"
 */
static boolean
parse_ipath (const struct parser_table* entry, char **argv, int *arg_ptr)
{
  const char *name;

  fnmatch_sanitycheck ();
  if (collect_arg (argv, arg_ptr, &name))
    {
      struct predicate *our_pred = insert_primary_withpred (entry, pred_ipath);
      our_pred->need_stat = our_pred->need_type = false;
      our_pred->args.str = name;
      our_pred->est_success_rate = estimate_pattern_match_rate (name, 0);
      return true;
    }
  return false;
}

static boolean
parse_iwholename (const struct parser_table* entry, char **argv, int *arg_ptr)
{
  return parse_ipath (entry, argv, arg_ptr);
}

static boolean
parse_iregex (const struct parser_table* entry, char **argv, int *arg_ptr)
{
  return insert_regex (argv, arg_ptr, entry, RE_ICASE|options.regex_options);
}

static boolean
parse_links (const struct parser_table* entry, char **argv, int *arg_ptr)
{
  struct predicate *p = insert_num (argv, arg_ptr, entry);
  if (p)
    {
      if (p->args.numinfo.l_val == 1)
	p->est_success_rate = 0.99;
      else if (p->args.numinfo.l_val == 2)
	p->est_success_rate = 0.01;
      else
	p->est_success_rate = 1e-3;
      return true;
    }
  else
    {
      return false;
    }
}

static boolean
parse_lname (const struct parser_table* entry, char **argv, int *arg_ptr)
{
  const char *name;
  fnmatch_sanitycheck();
  if (collect_arg(argv, arg_ptr, &name))
    {
      struct predicate *our_pred = insert_primary (entry);
      our_pred->args.str = name;
      our_pred->est_success_rate = 0.1 * estimate_pattern_match_rate(name, 0);
      return true;
    }
  return false;
}

static boolean
parse_ls (const struct parser_table* entry, char **argv, int *arg_ptr)
{
  (void) &argv;
  (void) &arg_ptr;
  return insert_fls(entry, NULL);
}

static boolean
insert_depthspec(const struct parser_table* entry, char **argv, int *arg_ptr,
		 int *limitptr)
{
  const char *depthstr;
  int depth_len;
  const char *predicate = argv[(*arg_ptr)-1];
  if (collect_arg(argv, arg_ptr, &depthstr))
    {
      depth_len = strspn (depthstr, "0123456789");
      if ((depth_len > 0) && (depthstr[depth_len] == 0))
	{
	  (*limitptr) = safe_atoi (depthstr);
	  if (*limitptr >= 0)
	    {
	      return parse_noop(entry, argv, arg_ptr);
	    }
	}
      error(1, 0, _("Expected a positive decimal integer argument to %s, but got %s"),
	    predicate,
	    quotearg_n_style(0, options.err_quoting_style, depthstr));
      return false;
    }
  /* missing argument */
  return false;
}


static boolean
parse_maxdepth (const struct parser_table* entry, char **argv, int *arg_ptr)
{
  return insert_depthspec(entry, argv, arg_ptr, &options.maxdepth);
}

static boolean
parse_mindepth (const struct parser_table* entry, char **argv, int *arg_ptr)
{
  return insert_depthspec(entry, argv, arg_ptr, &options.mindepth);
}


static boolean
do_parse_xmin (const struct parser_table* entry,
	       char **argv,
	       int *arg_ptr,
	       enum xval xv)
{
  const char *minutes;

  if (collect_arg(argv, arg_ptr, &minutes))
    {
      struct time_val tval;
      struct timespec origin = options.cur_day_start;
      tval.xval = xv;
      origin.tv_sec += DAYSECS;
      if (get_relative_timestamp(minutes, &tval, origin, 60,
				 "arithmetic overflow while converting %s "
				 "minutes to a number of seconds"))
	{
	  struct predicate *our_pred = insert_primary (entry);
	  our_pred->args.reftime = tval;
	  our_pred->est_success_rate = estimate_timestamp_success_rate(tval.ts.tv_sec);
	  return true;
	}
    }
  return false;
}
static boolean
parse_amin (const struct parser_table* entry, char **argv, int *arg_ptr)
{
  return do_parse_xmin(entry, argv, arg_ptr, XVAL_ATIME);
}

static boolean
parse_cmin (const struct parser_table* entry, char **argv, int *arg_ptr)
{
  return do_parse_xmin(entry, argv, arg_ptr, XVAL_CTIME);
}


static boolean
parse_mmin (const struct parser_table* entry, char **argv, int *arg_ptr)
{
  return do_parse_xmin(entry, argv, arg_ptr, XVAL_MTIME);
}

static boolean
parse_name (const struct parser_table* entry, char **argv, int *arg_ptr)
{
  const char *name;
  if (collect_arg(argv, arg_ptr, &name))
    {
      fnmatch_sanitycheck();
      if (check_name_arg("-name", name))
	{
	  struct predicate *our_pred = insert_primary (entry);
	  our_pred->need_stat = our_pred->need_type = false;
	  our_pred->args.str = name;
	  our_pred->est_success_rate = estimate_pattern_match_rate(name, 0);
	  return true;
	}
    }
  return false;
}

static boolean
parse_negate (const struct parser_table* entry, char **argv, int *arg_ptr)
{
  struct predicate *our_pred;

  (void) &argv;
  (void) &arg_ptr;

  our_pred = get_new_pred_chk_op (entry);
  our_pred->pred_func = pred_negate;
  our_pred->p_type = UNI_OP;
  our_pred->p_prec = NEGATE_PREC;
  our_pred->need_stat = our_pred->need_type = false;
  return true;
}

static boolean
parse_newer (const struct parser_table* entry, char **argv, int *arg_ptr)
{
  struct predicate *our_pred;
  struct stat stat_newer;

  set_stat_placeholders(&stat_newer);
  if (collect_arg_stat_info(argv, arg_ptr, &stat_newer))
    {
      our_pred = insert_primary (entry);
      our_pred->args.reftime.ts = get_stat_mtime(&stat_newer);
      our_pred->args.reftime.xval = XVAL_MTIME;
      our_pred->args.reftime.kind = COMP_GT;
      our_pred->est_success_rate = estimate_timestamp_success_rate(stat_newer.st_mtime);
      return true;
    }
  return false;
}


static boolean
parse_newerXY (const struct parser_table* entry, char **argv, int *arg_ptr)
{
  (void) argv;
  (void) arg_ptr;

  if ((argv == NULL) || (argv[*arg_ptr] == NULL))
    {
      return false;
    }
  else if (8u != strlen(argv[*arg_ptr]))
    {
      return false;
    }
  else
    {
      char x, y;
      const char validchars[] = "aBcmt";

      assert (0 == strncmp("-newer", argv[*arg_ptr], 6));
      x = argv[*arg_ptr][6];
      y = argv[*arg_ptr][7];


#if !defined(HAVE_STRUCT_STAT_ST_BIRTHTIME) && !defined(HAVE_STRUCT_STAT_ST_BIRTHTIMENSEC) && !defined(HAVE_STRUCT_STAT_ST_BIRTHTIMESPEC_TV_NSEC)
      if ('B' == x || 'B' == y)
	{
	  error(0, 0,
		_("This system does not provide a way to find the birth time of a file."));
	  return false;
	}
#endif

      /* -newertY (for any Y) is invalid. */
      if (x == 't'
	  || 0 == strchr(validchars, x)
	  || 0 == strchr( validchars, y))
	{
	  return false;
	}
      else
	{
	  struct predicate *our_pred;

	  /* Because this item is ARG_SPECIAL_PARSE, we have to advance arg_ptr
	   * past the test name (for most other tests, this is already done)
	   */
	  if (argv[1+*arg_ptr] == NULL)
	    {
	      error(1, 0, _("The %s test needs an argument"),
		    quotearg_n_style(0, options.err_quoting_style, argv[*arg_ptr]));
	    }
	  else
	    {
	      (*arg_ptr)++;
	    }

	  our_pred = insert_primary (entry);


	  switch (x)
	    {
	    case 'a':
	      our_pred->args.reftime.xval = XVAL_ATIME;
	      break;
	    case 'B':
	      our_pred->args.reftime.xval = XVAL_BIRTHTIME;
	      break;
	    case 'c':
	      our_pred->args.reftime.xval = XVAL_CTIME;
	      break;
	    case 'm':
	      our_pred->args.reftime.xval = XVAL_MTIME;
	      break;
	    default:
	      assert (strchr(validchars, x));
	      assert (0);
	    }

	  if ('t' == y)
	    {
	      if (!get_date(&our_pred->args.reftime.ts,
			    argv[*arg_ptr],
			    &options.start_time))
		{
		  error(1, 0,
			_("I cannot figure out how to interpret %s as a date or time"),
			quotearg_n_style(0, options.err_quoting_style, argv[*arg_ptr]));
		}
	    }
	  else
	    {
	      struct stat stat_newer;

	      /* Stat the named file. */
	      set_stat_placeholders(&stat_newer);
	      if ((*options.xstat) (argv[*arg_ptr], &stat_newer))
		fatal_file_error(argv[*arg_ptr]);

	      if (!get_stat_Ytime(&stat_newer, y, &our_pred->args.reftime.ts))
		{
		  /* We cannot extract a timestamp from the struct stat. */
		  error(1, 0, _("Cannot obtain birth time of file %s"),
			safely_quote_err_filename(0, argv[*arg_ptr]));
		}
	    }
	  our_pred->args.reftime.kind = COMP_GT;
	  our_pred->est_success_rate = estimate_timestamp_success_rate(our_pred->args.reftime.ts.tv_sec);
	  (*arg_ptr)++;

	  assert (our_pred->pred_func != NULL);
	  assert (our_pred->pred_func == pred_newerXY);
	  assert (our_pred->need_stat);
	  return true;
	}
    }
}


static boolean
parse_noleaf (const struct parser_table* entry, char **argv, int *arg_ptr)
{
  options.no_leaf_check = true;
  return parse_noop(entry, argv, arg_ptr);
}

#ifdef CACHE_IDS
/* Arbitrary amount by which to increase size
   of `uid_unused' and `gid_unused'. */
#define ALLOC_STEP 2048

/* Boolean: if uid_unused[n] is nonzero, then UID n has no passwd entry. */
char *uid_unused = NULL;

/* Number of elements in `uid_unused'. */
unsigned uid_allocated;

/* Similar for GIDs and group entries. */
char *gid_unused = NULL;
unsigned gid_allocated;
#endif

static boolean
parse_nogroup (const struct parser_table* entry, char **argv, int *arg_ptr)
{
  struct predicate *our_pred;

  (void) &argv;
  (void) &arg_ptr;

  our_pred = insert_primary (entry);
  our_pred->est_success_rate = 1e-4;
#ifdef CACHE_IDS
  if (gid_unused == NULL)
    {
      struct group *gr;

      gid_allocated = ALLOC_STEP;
      gid_unused = xmalloc (gid_allocated);
      memset (gid_unused, 1, gid_allocated);
      setgrent ();
      while ((gr = getgrent ()) != NULL)
	{
	  if ((unsigned) gr->gr_gid >= gid_allocated)
	    {
	      unsigned new_allocated = (unsigned) gr->gr_gid + ALLOC_STEP;
	      gid_unused = xrealloc (gid_unused, new_allocated);
	      memset (gid_unused + gid_allocated, 1,
		      new_allocated - gid_allocated);
	      gid_allocated = new_allocated;
	    }
	  gid_unused[(unsigned) gr->gr_gid] = 0;
	}
      endgrent ();
    }
#endif
  return true;
}

static boolean
parse_nouser (const struct parser_table* entry, char **argv, int *arg_ptr)
{
  struct predicate *our_pred;
  (void) argv;
  (void) arg_ptr;


  our_pred = insert_primary (entry);
  our_pred->est_success_rate = 1e-3;
#ifdef CACHE_IDS
  if (uid_unused == NULL)
    {
      struct passwd *pw;

      uid_allocated = ALLOC_STEP;
      uid_unused = xmalloc (uid_allocated);
      memset (uid_unused, 1, uid_allocated);
      setpwent ();
      while ((pw = getpwent ()) != NULL)
	{
	  if ((unsigned) pw->pw_uid >= uid_allocated)
	    {
	      unsigned new_allocated = (unsigned) pw->pw_uid + ALLOC_STEP;
	      uid_unused = xrealloc (uid_unused, new_allocated);
	      memset (uid_unused + uid_allocated, 1,
		      new_allocated - uid_allocated);
	      uid_allocated = new_allocated;
	    }
	  uid_unused[(unsigned) pw->pw_uid] = 0;
	}
      endpwent ();
    }
#endif
  return true;
}

static boolean
parse_nowarn (const struct parser_table* entry, char **argv, int *arg_ptr)
{
  options.warnings = false;
  return parse_noop(entry, argv, arg_ptr);
}

static boolean
parse_ok (const struct parser_table* entry, char **argv, int *arg_ptr)
{
  return insert_exec_ok ("-ok", entry, get_start_dirfd(), argv, arg_ptr);
}

static boolean
parse_okdir (const struct parser_table* entry, char **argv, int *arg_ptr)
{
  return insert_exec_ok ("-okdir", entry, -1, argv, arg_ptr);
}

boolean
parse_openparen (const struct parser_table* entry, char **argv, int *arg_ptr)
{
  struct predicate *our_pred;

  (void) argv;
  (void) arg_ptr;

  our_pred = get_new_pred_chk_op (entry);
  our_pred->pred_func = pred_openparen;
  our_pred->p_type = OPEN_PAREN;
  our_pred->p_prec = NO_PREC;
  our_pred->need_stat = our_pred->need_type = false;
  return true;
}

static boolean
parse_or (const struct parser_table* entry, char **argv, int *arg_ptr)
{
  struct predicate *our_pred;

  (void) argv;
  (void) arg_ptr;

  our_pred = get_new_pred (entry);
  our_pred->pred_func = pred_or;
  our_pred->p_type = BI_OP;
  our_pred->p_prec = OR_PREC;
  our_pred->need_stat = our_pred->need_type = false;
  return true;
}

/* For some time, -path was deprecated (at RMS's request) in favour of
 * -iwholename.  See the node "GNU Manuals" in standards.texi for the
 * rationale for this (basically, GNU prefers the use of the phrase
 * "file name" to "path name".
 *
 * We do not issue a warning that this usage is deprecated
 * since
 * (a) HPUX find supports this predicate also and
 * (b) it will soon be in POSIX anyway.
 */
static boolean
parse_path (const struct parser_table* entry, char **argv, int *arg_ptr)
{
  const char *name;
  if (collect_arg(argv, arg_ptr, &name))
    {
      struct predicate *our_pred = insert_primary_withpred (entry, pred_path);
      our_pred->need_stat = our_pred->need_type = false;
      our_pred->args.str = name;
      our_pred->est_success_rate = estimate_pattern_match_rate (name, 0);
      return true;
    }
  return false;
}

static boolean
parse_wholename (const struct parser_table* entry, char **argv, int *arg_ptr)
{
  return parse_path (entry, argv, arg_ptr);
}

static void
non_posix_mode(const char *mode)
{
  if (options.posixly_correct)
    {
      error (1, 0, _("Mode %s is not valid when POSIXLY_CORRECT is on."),
	     quotearg_n_style(0, options.err_quoting_style, mode));
    }
}


static boolean
parse_perm (const struct parser_table* entry, char **argv, int *arg_ptr)
{
  mode_t perm_val[2];
  float rate;
  int mode_start = 0;
  boolean havekind = false;
  enum permissions_type kind = PERM_EXACT;
  struct mode_change *change = NULL;
  struct predicate *our_pred;
  const char *perm_expr;

  if (!collect_arg(argv, arg_ptr, &perm_expr))
    return false;

  switch (perm_expr[0])
    {
    case '-':
      mode_start = 1;
      kind = PERM_AT_LEAST;
      havekind = true;
      rate = 0.2;
      break;

     case '+':
       change = mode_compile (perm_expr);
       if (NULL == change)
	 {
	   /* Most likely the caller is an old script that is still
	    * using the obsolete GNU syntax '-perm +MODE'.  This old
	    * syntax was withdrawn in favor of '-perm /MODE' because
	    * it is incompatible with POSIX in some cases, but we
	    * still support uses of it that are not incompatible with
	    * POSIX.
	    *
	    * Example: POSIXLY_CORRECT=y find -perm +a+x
	    */
	   non_posix_mode(perm_expr);

	   /* support the previous behaviour. */
	   mode_start = 1;
	   kind = PERM_ANY;
	   rate = 0.3;
	 }
       else
	 {
	   /* This is a POSIX-compatible usage */
	   mode_start = 0;
	   kind = PERM_EXACT;
	   rate = 0.1;
	 }
       havekind = true;
       break;

    case '/':			/* GNU extension */
      non_posix_mode(perm_expr);
      mode_start = 1;
      kind = PERM_ANY;
      havekind = true;
      rate = 0.3;
      break;

    default:
      /* For example, '-perm 0644', which is valid and matches
       * only files whose mode is exactly 0644.
       */
      mode_start = 0;
      kind = PERM_EXACT;
      havekind = true;
      rate = 0.01;
      break;
    }

  if (NULL == change)
    {
      change = mode_compile (perm_expr + mode_start);
      if (NULL == change)
	error (1, 0, _("invalid mode %s"),
	       quotearg_n_style(0, options.err_quoting_style, perm_expr));
    }
  perm_val[0] = mode_adjust (0, false, 0, change, NULL);
  perm_val[1] = mode_adjust (0, true, 0, change, NULL);
  free (change);

  if (('/' == perm_expr[0]) && (0 == perm_val[0]) && (0 == perm_val[1]))
    {
      /* The meaning of -perm /000 will change in the future.  It
       * currently matches no files, but like -perm -000 it should
       * match all files.
       *
       * Starting in 2005, we used to issue a warning message
       * informing the user that the behaviour would change in the
       * future.  We have now changed the behaviour and issue a
       * warning message that the behaviour recently changed.
       */
      error (0, 0,
	     _("warning: you have specified a mode pattern %s (which is "
	       "equivalent to /000). The meaning of -perm /000 has now been "
	       "changed to be consistent with -perm -000; that is, while it "
	       "used to match no files, it now matches all files."),
	     perm_expr);

      kind = PERM_AT_LEAST;
      havekind = true;

      /* The "magic" number below is just the fraction of files on my
       * own system that "-type l -xtype l" fails for (i.e. unbroken symlinks).
       * Actual totals are 1472 and 1073833.
       */
      rate = 0.9986; /* probably matches anything but a broken symlink */
    }

  our_pred = insert_primary (entry);
  our_pred->est_success_rate = rate;
  if (havekind)
    {
      our_pred->args.perm.kind = kind;
    }
  else
    {

      switch (perm_expr[0])
	{
	case '-':
	  our_pred->args.perm.kind = PERM_AT_LEAST;
	  break;
	case '+':
	  our_pred->args.perm.kind = PERM_ANY;
	  break;
	default:
	  our_pred->args.perm.kind = PERM_EXACT;
	  break;
	}
    }
  memcpy (our_pred->args.perm.val, perm_val, sizeof perm_val);
  return true;
}

boolean
parse_print (const struct parser_table* entry, char **argv, int *arg_ptr)
{
  struct predicate *our_pred;

  (void) argv;
  (void) arg_ptr;

  our_pred = insert_primary (entry);
  /* -print has the side effect of printing.  This prevents us
     from doing undesired multiple printing when the user has
     already specified -print. */
  our_pred->side_effects = our_pred->no_default_print = true;
  our_pred->need_stat = our_pred->need_type = false;
  open_stdout(&our_pred->args.printf_vec);
  return true;
}

static boolean
parse_print0 (const struct parser_table* entry, char **argv, int *arg_ptr)
{
  return insert_fprint(entry, NULL);
}

static boolean
parse_printf (const struct parser_table* entry, char **argv, int *arg_ptr)
{
  const char *format;
  if (collect_arg(argv, arg_ptr, &format))
    {
      struct format_val fmt;
      open_stdout(&fmt);
      return insert_fprintf (&fmt, entry, pred_fprintf, format);
    }
  return false;
}

static boolean
parse_fprintf (const struct parser_table* entry, char **argv, int *arg_ptr)
{
  const char *format, *filename;
  if (collect_arg(argv, arg_ptr, &filename))
    {
      if (collect_arg(argv, arg_ptr, &format))
	{
	  struct format_val fmt;
	  open_output_file (filename, &fmt);
	  return insert_fprintf (&fmt, entry, pred_fprintf, format);
	}
    }
  return false;
}

static boolean
parse_prune (const struct parser_table* entry, char **argv, int *arg_ptr)
{
  struct predicate *our_pred;

  (void) argv;
  (void) arg_ptr;

  our_pred = insert_primary (entry);
  if (options.do_dir_first == false)
    our_pred->need_stat = our_pred->need_type = false;
  /* -prune has a side effect that it does not descend into
     the current directory. */
  our_pred->side_effects = true;
  our_pred->no_default_print = false;
  return true;
}

static boolean
parse_quit  (const struct parser_table* entry, char **argv, int *arg_ptr)
{
  struct predicate *our_pred = insert_primary (entry);
  (void) argv;
  (void) arg_ptr;
  our_pred->need_stat = our_pred->need_type = false;
  our_pred->side_effects = true; /* Exiting is a side effect... */
  our_pred->no_default_print = false; /* Don't inhibit the default print, though. */
  our_pred->est_success_rate = 1.0f;
  return true;
}


static boolean
parse_regextype (const struct parser_table* entry, char **argv, int *arg_ptr)
{
  const char *type_name;
  if (collect_arg(argv, arg_ptr, &type_name))
    {
      /* collect the regex type name */
      options.regex_options = get_regex_type(type_name);
      return parse_noop(entry, argv, arg_ptr);
    }
  return false;
}


static boolean
parse_regex (const struct parser_table* entry, char **argv, int *arg_ptr)
{
  return insert_regex (argv, arg_ptr, entry, options.regex_options);
}

static boolean
insert_regex (char **argv,
	      int *arg_ptr,
	      const struct parser_table *entry,
	      int regex_options)
{
  const char *rx;
  if (collect_arg(argv, arg_ptr, &rx))
    {
      struct re_pattern_buffer *re;
      const char *error_message;
      struct predicate *our_pred = insert_primary_withpred (entry, pred_regex);
      our_pred->need_stat = our_pred->need_type = false;
      re = xmalloc (sizeof (struct re_pattern_buffer));
      our_pred->args.regex = re;
      re->allocated = 100;
      re->buffer = xmalloc (re->allocated);
      re->fastmap = NULL;

      re_set_syntax(regex_options);
      re->syntax = regex_options;
      re->translate = NULL;

      error_message = re_compile_pattern (rx, strlen(rx), re);
      if (error_message)
	error (1, 0, "%s", error_message);
      our_pred->est_success_rate = estimate_pattern_match_rate(rx, 1);
      return true;
    }
  return false;
}

static boolean
parse_size (const struct parser_table* entry, char **argv, int *arg_ptr)
{
  struct predicate *our_pred;
  uintmax_t num;
  char suffix;
  enum comparison_type c_type;

  int blksize = 512;
  int len;

  /* XXX: cannot (yet) convert to ue collect_arg() as this
   * function modifies the args in-place.
   */
  if ((argv == NULL) || (argv[*arg_ptr] == NULL))
    return false;

  len = strlen (argv[*arg_ptr]);
  if (len == 0)
    error (1, 0, _("invalid null argument to -size"));

  suffix = argv[*arg_ptr][len - 1];
  switch (suffix)
    {
    case 'b':
      blksize = 512;
      argv[*arg_ptr][len - 1] = '\0';
      break;

    case 'c':
      blksize = 1;
      argv[*arg_ptr][len - 1] = '\0';
      break;

    case 'k':
      blksize = 1024;
      argv[*arg_ptr][len - 1] = '\0';
      break;

    case 'M':			/* Megabytes */
      blksize = 1024*1024;
      argv[*arg_ptr][len - 1] = '\0';
      break;

    case 'G':			/* Gigabytes */
      blksize = 1024*1024*1024;
      argv[*arg_ptr][len - 1] = '\0';
      break;

    case 'w':
      blksize = 2;
      argv[*arg_ptr][len - 1] = '\0';
      break;

    case '0':
    case '1':
    case '2':
    case '3':
    case '4':
    case '5':
    case '6':
    case '7':
    case '8':
    case '9':
      break;

    default:
      error (1, 0, _("invalid -size type `%c'"), argv[*arg_ptr][len - 1]);
    }
  /* TODO: accept fractional megabytes etc. ? */
  if (!get_num (argv[*arg_ptr], &num, &c_type))
    {
      error(1, 0,
	    _("Invalid argument `%s%c' to -size"),
	    argv[*arg_ptr], (int)suffix);
      return false;
    }
  our_pred = insert_primary (entry);
  our_pred->args.size.kind = c_type;
  our_pred->args.size.blocksize = blksize;
  our_pred->args.size.size = num;
  our_pred->need_stat = true;
  our_pred->need_type = false;

  if (COMP_GT == c_type)
    our_pred->est_success_rate = (num*blksize > 20480) ? 0.1 : 0.9;
  else if (COMP_LT == c_type)
    our_pred->est_success_rate = (num*blksize > 20480) ? 0.9 : 0.1;
  else
    our_pred->est_success_rate = 0.01;

  (*arg_ptr)++;
  return true;
}


static boolean
parse_samefile (const struct parser_table* entry, char **argv, int *arg_ptr)
{
  /* General idea: stat the file, remember device and inode numbers.
   * If a candidate file matches those, it's the same file.
   */
  struct predicate *our_pred;
  struct stat st, fst;
  int fd, openflags;

  set_stat_placeholders(&st);
  if (!collect_arg_stat_info(argv, arg_ptr, &st))
    return false;

  set_stat_placeholders(&fst);
  /* POSIX systems are free to re-use the inode number of a deleted
   * file.  To ensure that we are not fooled by inode reuse, we hold
   * the file open if we can.  This would prevent the system reusing
   * the file.
   */
  fd = -3;			/* means, uninitialised */
  openflags = O_RDONLY;

  if (options.symlink_handling == SYMLINK_NEVER_DEREF)
    {
      if (options.open_nofollow_available)
	{
	  assert (O_NOFOLLOW != 0);
	  openflags |= O_NOFOLLOW;
	  fd = -1;		/* safe to open it. */
	}
      else
	{
	  if (S_ISLNK(st.st_mode))
	    {
	      /* no way to ensure that a symlink will not be followed
	       * by open(2), so fall back on using lstat().  Accept
	       * the risk that the named file will be deleted and
	       * replaced with another having the same inode.
	       *
	       * Avoid opening the file.
	       */
	      fd = -2;		/* Do not open it */
	    }
	  else
	    {
	      fd = -1;
	      /* Race condition here: the file might become a symlink here. */
	    }
	}
    }
  else
    {
      /* We want to dereference the symlink anyway */
      fd = -1;			/* safe to open it without O_NOFOLLOW */
    }

  assert (fd != -3);		/* check we made a decision */
  if (fd == -1)
    {
      /* Race condition here.  The file might become a
       * symbolic link in between out call to stat and
       * the call to open.
       */
      fd = open(argv[*arg_ptr], openflags);

      if (fd >= 0)
	{
	  /* We stat the file again here to prevent a race condition
	   * between the first stat and the call to open(2).
	   */
	  if (0 != fstat(fd, &fst))
	    {
	      fatal_file_error(argv[*arg_ptr]);
	    }
	  else
	    {
	      /* Worry about the race condition.  If the file became a
	       * symlink after our first stat and before our call to
	       * open, fst may contain the stat information for the
	       * destination of the link, not the link itself.
	       */
	      if ((*options.xstat) (argv[*arg_ptr], &st))
		fatal_file_error(argv[*arg_ptr]);

	      if ((options.symlink_handling == SYMLINK_NEVER_DEREF)
		  && (!options.open_nofollow_available))
		{
		  if (S_ISLNK(st.st_mode))
		    {
		      /* We lost the race.  Leave the data in st.  The
		       * file descriptor points to the wrong thing.
		       */
		      close(fd);
		      fd = -1;
		    }
		  else
		    {
		      /* Several possibilities here:
		       * 1. There was no race
		       * 2. The file changed into a symlink after the stat and
		       *    before the open, and then back into a non-symlink
		       *    before the second stat.
		       *
		       * In case (1) there is no problem.  In case (2),
		       * the stat() and fstat() calls will have returned
		       * different data.  O_NOFOLLOW was not available,
		       * so the open() call may have followed a symlink
		       * even if the -P option is in effect.
		       */
		      if ((st.st_dev == fst.st_dev)
			  && (st.st_ino == fst.st_ino))
			{
			  /* No race.  No need to copy fst to st,
			   * since they should be identical (modulo
			   * differences in padding bytes).
			   */
			}
		      else
			{
			  /* We lost the race.  Leave the data in st.  The
			   * file descriptor points to the wrong thing.
			   */
			  close(fd);
			  fd = -1;
			}
		    }
		}
	      else
		{
		  st = fst;
		}
	    }
	}
    }

  our_pred = insert_primary (entry);
  our_pred->args.samefileid.ino = st.st_ino;
  our_pred->args.samefileid.dev = st.st_dev;
  our_pred->args.samefileid.fd  = fd;
  our_pred->need_type = false;
  our_pred->need_stat = true;
  our_pred->est_success_rate = 0.01f;
  return true;
}

#if 0
/* This function is commented out partly because support for it is
 * uneven.
 */
static boolean
parse_show_control_chars (const struct parser_table* entry,
			  char **argv,
			  int *arg_ptr)
{
  const char *arg;
  const char *errmsg = _("The -show-control-chars option takes "
			 "a single argument which "
			 "must be 'literal' or 'safe'");

  if ((argv == NULL) || (argv[*arg_ptr] == NULL))
    {
      error (1, errno, "%s", errmsg);
      return false;
    }
  else
    {
      arg = argv[*arg_ptr];

      if (0 == strcmp("literal", arg))
	{
	  options.literal_control_chars = true;
	}
      else if (0 == strcmp("safe", arg))
	{
	  options.literal_control_chars = false;
	}
      else
	{
	  error (1, errno, "%s", errmsg);
	  return false;
	}
      (*arg_ptr)++;		/* consume the argument. */
      return true;
    }
}
#endif


static boolean
parse_true (const struct parser_table* entry, char **argv, int *arg_ptr)
{
  struct predicate *our_pred;

  (void) argv;
  (void) arg_ptr;

  our_pred = insert_primary (entry);
  our_pred->need_stat = our_pred->need_type = false;
  our_pred->est_success_rate = 1.0f;
  return true;
}

static boolean
parse_noop (const struct parser_table* entry, char **argv, int *arg_ptr)
{
  (void) entry;
  return parse_true(get_noop(), argv, arg_ptr);
}

static boolean
parse_accesscheck (const struct parser_table* entry, char **argv, int *arg_ptr)
{
  struct predicate *our_pred;
  (void) argv;
  (void) arg_ptr;
  our_pred = insert_primary (entry);
  our_pred->need_stat = our_pred->need_type = false;
  our_pred->side_effects = our_pred->no_default_print = false;
  if (pred_is(our_pred, pred_executable))
    our_pred->est_success_rate = 0.2;
  else
    our_pred->est_success_rate = 0.9;
  return true;
}

static boolean
parse_type (const struct parser_table* entry, char **argv, int *arg_ptr)
{
  return insert_type (argv, arg_ptr, entry, pred_type);
}

static boolean
parse_uid (const struct parser_table* entry, char **argv, int *arg_ptr)
{
  struct predicate *p = insert_num (argv, arg_ptr, entry);
  if (p)
    {
      p->est_success_rate = (p->args.numinfo.l_val < 100) ? 0.99 : 0.2;
      return true;
    }
  else
    {
      return false;
    }
}

static boolean
parse_used (const struct parser_table* entry, char **argv, int *arg_ptr)
{
  struct predicate *our_pred;
  struct time_val tval;
  const char *offset_str;
  const char *errmsg = "arithmetic overflow while converting %s days to a number of seconds";

  if (collect_arg(argv, arg_ptr, &offset_str))
    {
      /* The timespec is actually a delta value, so we use an origin of 0. */
      struct timespec zero = {0,0};
      if (get_relative_timestamp(offset_str, &tval, zero, DAYSECS, errmsg))
	{
	  our_pred = insert_primary (entry);
	  our_pred->args.reftime = tval;
	  our_pred->est_success_rate = estimate_file_age_success_rate(tval.ts.tv_sec / DAYSECS);
	  return true;
	}
      else
	{
	  error(1, 0, _("Invalid argument %s to -used"), offset_str);
	  return false;
	}
    }
  else
    {
      return false;		/* missing argument */
    }
}

static boolean
parse_user (const struct parser_table* entry, char **argv, int *arg_ptr)
{
  const char *username;

  if (collect_arg(argv, arg_ptr, &username))
    {
      struct predicate *our_pred;
      uid_t uid;
      struct passwd *cur_pwd = getpwnam(username);
      endpwent();
      if (cur_pwd != NULL)
	{
	  uid = cur_pwd->pw_uid;
	}
      else
	{
	  const size_t uid_len = strspn (username, "0123456789");
	  if (uid_len && (username[uid_len]==0))
	    {
	      uid = safe_atoi (username);
	    }
	  else
	    {
	      /* This is a fatal error (if we just return false, the caller
	       * will say "invalid argument `username' to -user", which is
	       * not as helpful). */
	      if (username[0])
		{
		  error (1, 0, _("%s is not the name of a known user"),
			 quotearg_n_style (0, options.err_quoting_style,
					   username));
		}
	      else
		{
		  error (1, 0, _("The argument to -user should not be empty"));
		}
	      /*NOTREACHED*/
	      return false;
	    }
	}
      our_pred = insert_primary (entry);
      our_pred->args.uid = uid;
      our_pred->est_success_rate = (our_pred->args.uid < 100) ? 0.99 : 0.2;
      return true;
    }
  return false;
}

static boolean
parse_version (const struct parser_table* entry, char **argv, int *arg_ptr)
{
  int features = 0;
  int flags;

  (void) argv;
  (void) arg_ptr;
  (void) entry;

  display_findutils_version("find");
  printf (_("Features enabled: "));

#if CACHE_IDS
  printf("CACHE_IDS ");
  ++features;
#endif
#if DEBUG
  printf("DEBUG ");
  ++features;
#endif
#if DEBUG_STAT
  printf("DEBUG_STAT ");
  ++features;
#endif
#if defined USE_STRUCT_DIRENT_D_TYPE && defined HAVE_STRUCT_DIRENT_D_TYPE
  printf("D_TYPE ");
  ++features;
#endif
#if defined O_NOFOLLOW
  printf("O_NOFOLLOW(%s) ",
	 (options.open_nofollow_available ? "enabled" : "disabled"));
  ++features;
#endif
#if defined LEAF_OPTIMISATION
  printf("LEAF_OPTIMISATION ");
  ++features;
#endif

  flags = 0;
  if (is_fts_enabled(&flags))
    {
      int nflags = 0;
      printf("FTS(");
      ++features;

      if (flags & FTS_CWDFD)
	{
	  if (nflags)
	    {
	      printf(",");
	    }
	  printf("FTS_CWDFD");
	  ++nflags;
	}
      printf(") ");
    }

  printf("CBO(level=%d) ", (int)(options.optimisation_level));
  ++features;

  if (0 == features)
    {
      /* For the moment, leave this as English in case someone wants
	 to parse these strings. */
      printf("none");
    }
  printf("\n");

  exit (0);
}

static boolean
parse_xdev (const struct parser_table* entry, char **argv, int *arg_ptr)
{
  options.stay_on_filesystem = true;
  return parse_noop(entry, argv, arg_ptr);
}

static boolean
parse_ignore_race (const struct parser_table* entry, char **argv, int *arg_ptr)
{
  options.ignore_readdir_race = true;
  return parse_noop(entry, argv, arg_ptr);
}

static boolean
parse_noignore_race (const struct parser_table* entry, char **argv, int *arg_ptr)
{
  options.ignore_readdir_race = false;
  return parse_noop(entry, argv, arg_ptr);
}

static boolean
parse_warn (const struct parser_table* entry, char **argv, int *arg_ptr)
{
  options.warnings = true;
  return parse_noop(entry, argv, arg_ptr);
}

static boolean
parse_xtype (const struct parser_table* entry, char **argv, int *arg_ptr)
{
  return insert_type (argv, arg_ptr, entry, pred_xtype);
}

static boolean
insert_type (char **argv, int *arg_ptr,
	     const struct parser_table *entry,
	     PRED_FUNC which_pred)
{
  mode_t type_cell;
  struct predicate *our_pred;
  float rate = 0.5;
  const char *typeletter;

  if (collect_arg(argv, arg_ptr, &typeletter))
    {
      if (strlen(typeletter) != 1u)
	{
	  error(1, 0, _("Arguments to -type should contain only one letter"));
	  return false;
	}

      switch (typeletter[0])
	{
	case 'b':			/* block special */
	  type_cell = S_IFBLK;
	  rate = 0.01f;
	  break;
	case 'c':			/* character special */
	  type_cell = S_IFCHR;
	  rate = 0.01f;
	  break;
	case 'd':			/* directory */
	  type_cell = S_IFDIR;
	  rate = 0.4f;
	  break;
	case 'f':			/* regular file */
	  type_cell = S_IFREG;
	  rate = 0.95f;
	  break;
#ifdef S_IFLNK
	case 'l':			/* symbolic link */
	  type_cell = S_IFLNK;
	  rate = 0.1f;
	  break;
#endif
#ifdef S_IFIFO
	case 'p':			/* pipe */
	  type_cell = S_IFIFO;
	  rate = 0.01f;
	  break;
#endif
#ifdef S_IFSOCK
	case 's':			/* socket */
	  type_cell = S_IFSOCK;
	  rate = 0.01f;
	  break;
#endif
#ifdef S_IFDOOR
	case 'D':			/* Solaris door */
	  type_cell = S_IFDOOR;
	  rate = 0.01f;
	  break;
#endif
	default:			/* None of the above ... nuke 'em. */
	  error(1, 0, _("Unknown argument to -type: %c"), (*typeletter));
	  return false;
	}
      our_pred = insert_primary_withpred (entry, which_pred);
      our_pred->est_success_rate = rate;

      /* Figure out if we will need to stat the file, because if we don't
       * need to follow symlinks, we can avoid a stat call by using
       * struct dirent.d_type.
       */
      if (which_pred == pred_xtype)
	{
	  our_pred->need_stat = true;
	  our_pred->need_type = false;
	}
      else
	{
	  our_pred->need_stat = false; /* struct dirent is enough */
	  our_pred->need_type = true;
	}
      our_pred->args.type = type_cell;
      return true;
    }
  return false;
}


/* Return true if the file accessed via FP is a terminal.
 */
static boolean
stream_is_tty(FILE *fp)
{
  int fd = fileno(fp);
  if (-1 == fd)
    {
      return false; /* not a valid stream */
    }
  else
    {
      return isatty(fd) ? true : false;
    }

}




/* XXX: do we need to pass FUNC to this function? */
static boolean
insert_fprintf (struct format_val *vec,
		const struct parser_table *entry, PRED_FUNC func,
		const char *format_const)
{
  char *format = (char*)format_const; /* XXX: casting away constness */
  register char *scan;		/* Current address in scanning `format'. */
  register char *scan2;		/* Address inside of element being scanned. */
  struct segment **segmentp;	/* Address of current segment. */
  struct predicate *our_pred;

  our_pred = insert_primary_withpred (entry, func);
  our_pred->side_effects = our_pred->no_default_print = true;
  our_pred->args.printf_vec = *vec;
  our_pred->need_type = false;
  our_pred->need_stat = false;
  our_pred->p_cost    = NeedsNothing;

  segmentp = &our_pred->args.printf_vec.segment;
  *segmentp = NULL;

  for (scan = format; *scan; scan++)
    {
      if (*scan == '\\')
	{
	  scan2 = scan + 1;
	  if (*scan2 >= '0' && *scan2 <= '7')
	    {
	      register int n, i;

	      for (i = n = 0; i < 3 && (*scan2 >= '0' && *scan2 <= '7');
		   i++, scan2++)
		n = 8 * n + *scan2 - '0';
	      scan2--;
	      *scan = n;
	    }
	  else
	    {
	      switch (*scan2)
		{
		case 'a':
		  *scan = 7;
		  break;
		case 'b':
		  *scan = '\b';
		  break;
		case 'c':
		  make_segment (segmentp, format, scan - format,
				KIND_STOP, 0, 0,
				our_pred);
		  if (our_pred->need_stat && (our_pred->p_cost < NeedsStatInfo))
		    our_pred->p_cost = NeedsStatInfo;
		  return true;
		case 'f':
		  *scan = '\f';
		  break;
		case 'n':
		  *scan = '\n';
		  break;
		case 'r':
		  *scan = '\r';
		  break;
		case 't':
		  *scan = '\t';
		  break;
		case 'v':
		  *scan = '\v';
		  break;
		case '\\':
		  /* *scan = '\\'; * it already is */
		  break;
		default:
		  error (0, 0,
			 _("warning: unrecognized escape `\\%c'"), *scan2);
		  scan++;
		  continue;
		}
	    }
	  segmentp = make_segment (segmentp, format, scan - format + 1,
				   KIND_PLAIN, 0, 0,
				   our_pred);
	  format = scan2 + 1;	/* Move past the escape. */
	  scan = scan2;		/* Incremented immediately by `for'. */
	}
      else if (*scan == '%')
	{
	  if (scan[1] == 0)
	    {
	      /* Trailing %.  We don't like those. */
	      error (1, 0, _("error: %s at end of format string"), scan);
	    }
	  else if (scan[1] == '%')
	    {
	      segmentp = make_segment (segmentp, format, scan - format + 1,
				       KIND_PLAIN, 0, 0,
				       our_pred);
	      scan++;
	      format = scan + 1;
	      continue;
	    }
	  /* Scan past flags, width and precision, to verify kind. */
	  for (scan2 = scan; *++scan2 && strchr ("-+ #", *scan2);)
	    /* Do nothing. */ ;
	  while (ISDIGIT (*scan2))
	    scan2++;
	  if (*scan2 == '.')
	    for (scan2++; ISDIGIT (*scan2); scan2++)
	      /* Do nothing. */ ;
	  if (strchr ("abcdDfFgGhHiklmMnpPsStuUyY", *scan2))
	    {
	      segmentp = make_segment (segmentp, format, scan2 - format,
				       KIND_FORMAT, *scan2, 0,
				       our_pred);
	      scan = scan2;
	      format = scan + 1;
	    }
	  else if (strchr ("ABCT", *scan2) && scan2[1])
	    {
	      segmentp = make_segment (segmentp, format, scan2 - format,
				       KIND_FORMAT, scan2[0], scan2[1],
				       our_pred);
	      scan = scan2 + 1;
	      format = scan + 1;
	      continue;
	    }
	  else
	    {
	      /* An unrecognized % escape.  Print the char after the %. */
	      error (0, 0, _("warning: unrecognized format directive `%%%c'"),
		     *scan2);
	      segmentp = make_segment (segmentp, format, scan - format,
				       KIND_PLAIN, 0, 0,
				       our_pred);
	      format = scan + 1;
	      continue;
	    }
	}
    }

  if (scan > format)
    make_segment (segmentp, format, scan - format, KIND_PLAIN, 0, 0,
		  our_pred);
  return true;
}

/* Create a new fprintf segment in *SEGMENT, with type KIND,
   from the text in FORMAT, which has length LEN.
   Return the address of the `next' pointer of the new segment. */

static struct segment **
make_segment (struct segment **segment,
	      char *format,
	      int len,
	      int kind,
	      char format_char,
	      char aux_format_char,
	      struct predicate *pred)
{
  enum EvaluationCost mycost = NeedsNothing;
  char *fmt;

  *segment = xmalloc (sizeof (struct segment));

  (*segment)->segkind = kind;
  (*segment)->format_char[0] = format_char;
  (*segment)->format_char[1] = aux_format_char;
  (*segment)->next = NULL;
  (*segment)->text_len = len;

  fmt = (*segment)->text = xmalloc (len + sizeof "d");
  strncpy (fmt, format, len);
  fmt += len;

  switch (kind)
    {
    case KIND_PLAIN:		/* Plain text string, no % conversion. */
    case KIND_STOP:		/* Terminate argument, no newline. */
      assert (0 == format_char);
      assert (0 == aux_format_char);
      *fmt = '\0';
      if (mycost > pred->p_cost)
	pred->p_cost = NeedsNothing;
      return &(*segment)->next;
      break;
    }

  assert (kind == KIND_FORMAT);
  switch (format_char)
    {
    case 'l':			/* object of symlink */
      pred->need_stat = true;
      mycost = NeedsLinkName;
      *fmt++ = 's';
      break;

    case 'y':			/* file type */
      pred->need_type = true;
      mycost = NeedsType;
      *fmt++ = 's';
      break;

    case 'a':			/* atime in `ctime' format */
    case 'A':			/* atime in user-specified strftime format */
    case 'B':			/* birth time in user-specified strftime format */
    case 'c':			/* ctime in `ctime' format */
    case 'C':			/* ctime in user-specified strftime format */
    case 'F':			/* file system type */
    case 'g':			/* group name */
    case 'i':			/* inode number */
    case 'M':			/* mode in `ls -l' format (eg., "drwxr-xr-x") */
    case 's':			/* size in bytes */
    case 't':			/* mtime in `ctime' format */
    case 'T':			/* mtime in user-specified strftime format */
    case 'u':			/* user name */
      pred->need_stat = true;
      mycost = NeedsStatInfo;
      *fmt++ = 's';
      break;

    case 'S':			/* sparseness */
      pred->need_stat = true;
      mycost = NeedsStatInfo;
      *fmt++ = 'g';
      break;

    case 'Y':			/* symlink pointed file type */
      pred->need_stat = true;
      mycost = NeedsType;	/* true for amortised effect */
      *fmt++ = 's';
      break;

    case 'f':			/* basename of path */
    case 'h':			/* leading directories part of path */
    case 'p':			/* pathname */
    case 'P':			/* pathname with ARGV element stripped */
      *fmt++ = 's';
      break;

    case 'H':			/* ARGV element file was found under */
      *fmt++ = 's';
      break;

      /* Numeric items that one might expect to honour
       * #, 0, + flags but which do not.
       */
    case 'G':			/* GID number */
    case 'U':			/* UID number */
    case 'b':			/* size in 512-byte blocks (NOT birthtime in ctime fmt)*/
    case 'D':                   /* Filesystem device on which the file exits */
    case 'k':			/* size in 1K blocks */
    case 'n':			/* number of links */
      pred->need_stat = true;
      mycost = NeedsStatInfo;
      *fmt++ = 's';
      break;

      /* Numeric items that DO honour #, 0, + flags.
       */
    case 'd':			/* depth in search tree (0 = ARGV element) */
      *fmt++ = 'd';
      break;

    case 'm':			/* mode as octal number (perms only) */
      *fmt++ = 'o';
      pred->need_stat = true;
      mycost = NeedsStatInfo;
      break;

    case '{':
    case '[':
    case '(':
      error (1, 0,
	     _("error: the format directive `%%%c' is reserved for future use"),
	     (int)kind);
      /*NOTREACHED*/
      break;
    }
  *fmt = '\0';

  if (mycost > pred->p_cost)
    pred->p_cost = mycost;
  return &(*segment)->next;
}

static void
check_path_safety(const char *action, char **argv)
{
  char *s;
  const char *path = getenv("PATH");
  if (NULL == path)
    {
      /* $PATH is not set.  Assume the OS default is safe.
       * That may not be true on Windows, but I'm not aware
       * of a way to get Windows to avoid searching the
       * current directory anyway.
       */
      return;
    }

  (void)argv;

  s = next_element(path, 1);
  while ((s = next_element ((char *) NULL, 1)) != NULL)
    {
      if (0 == strcmp(s, "."))
	{
	  error(1, 0, _("The current directory is included in the PATH "
			"environment variable, which is insecure in "
			"combination with the %s action of find.  "
			"Please remove the current directory from your "
			"$PATH (that is, remove \".\" or leading or trailing "
			"colons)"),
		action);
	}
      else if ('/' != s[0])
	{
	  /* Relative paths are also dangerous in $PATH. */
	  error(1, 0, _("The relative path %s is included in the PATH "
			"environment variable, which is insecure in "
			"combination with the %s action of find.  "
			"Please remove that entry from $PATH"),
		safely_quote_err_filename(0, s),
		action);
	}
    }
}


/* handles both exec and ok predicate */
static boolean
new_insert_exec_ok (const char *action,
		    const struct parser_table *entry,
		    int dir_fd,
		    char **argv,
		    int *arg_ptr)
{
  int start, end;		/* Indexes in ARGV of start & end of cmd. */
  int i;			/* Index into cmd args */
  int saw_braces;		/* True if previous arg was '{}'. */
  boolean allow_plus;		/* True if + is a valid terminator */
  int brace_count;		/* Number of instances of {}. */
  PRED_FUNC func = entry->pred_func;
  enum BC_INIT_STATUS bcstatus;

  struct predicate *our_pred;
  struct exec_val *execp;	/* Pointer for efficiency. */

  if ((argv == NULL) || (argv[*arg_ptr] == NULL))
    return false;

  our_pred = insert_primary_withpred (entry, func);
  our_pred->side_effects = our_pred->no_default_print = true;
  our_pred->need_type = our_pred->need_stat = false;

  execp = &our_pred->args.exec_vec;

  if ((func != pred_okdir) && (func != pred_ok))
    {
      allow_plus = true;
      execp->close_stdin = false;
    }
  else
    {
      allow_plus = false;
      /* If find reads stdin (i.e. for -ok and similar), close stdin
       * in the child to prevent some script from consiming the output
       * intended for find.
       */
      execp->close_stdin = true;
    }


  if ((func == pred_execdir) || (func == pred_okdir))
    {
      options.ignore_readdir_race = false;
      check_path_safety(action, argv);
      execp->use_current_dir = true;
    }
  else
    {
      execp->use_current_dir = false;
    }

  our_pred->args.exec_vec.multiple = 0;

  /* Count the number of args with path replacements, up until the ';'.
   * Also figure out if the command is terminated by ";" or by "+".
   */
  start = *arg_ptr;
  for (end = start, saw_braces=0, brace_count=0;
       (argv[end] != NULL)
       && ((argv[end][0] != ';') || (argv[end][1] != '\0'));
       end++)
    {
      /* For -exec and -execdir, "{} +" can terminate the command. */
      if ( allow_plus
	   && argv[end][0] == '+' && argv[end][1] == 0
	   && saw_braces)
	{
	  our_pred->args.exec_vec.multiple = 1;
	  break;
	}

      saw_braces = 0;
      if (mbsstr (argv[end], "{}"))
	{
	  saw_braces = 1;
	  ++brace_count;

	  if (0 == end && (func == pred_execdir || func == pred_okdir))
	    {
	      /* The POSIX standard says that {} replacement should
	       * occur even in the utility name.  This is insecure
	       * since it means we will be executing a command whose
	       * name is chosen according to whatever find finds in
	       * the file system.  That can be influenced by an
	       * attacker.  Hence for -execdir and -okdir this is not
	       * allowed.  We can specify this as those options are
	       * not defined by POSIX.
	       */
	      error(1, 0, _("You may not use {} within the utility name for "
			    "-execdir and -okdir, because this is a potential "
			    "security problem."));
	    }
	}
    }

  /* Fail if no command given or no semicolon found. */
  if ((end == start) || (argv[end] == NULL))
    {
      *arg_ptr = end;
      free(our_pred);
      return false;
    }

  if (our_pred->args.exec_vec.multiple && brace_count > 1)
    {

      const char *suffix;
      if (func == pred_execdir)
	suffix = "dir";
      else
	suffix = "";

      error(1, 0,
	    _("Only one instance of {} is supported with -exec%s ... +"),
	    suffix);
    }

  /* We use a switch statement here so that the compiler warns us when
   * we forget to handle a newly invented enum value.
   *
   * Like xargs, we allow 2KiB of headroom for the launched utility to
   * export its own environment variables before calling something
   * else.
   */
  bcstatus = bc_init_controlinfo(&execp->ctl, 2048u);
  switch (bcstatus)
    {
    case BC_INIT_ENV_TOO_BIG:
    case BC_INIT_CANNOT_ACCOMODATE_HEADROOM:
      error(1, 0,
	    _("The environment is too large for exec()."));
      break;
    case BC_INIT_OK:
      /* Good news.  Carry on. */
      break;
    }
  bc_use_sensible_arg_max(&execp->ctl);


  execp->ctl.exec_callback = launch;

  if (our_pred->args.exec_vec.multiple)
    {
      /* "+" terminator, so we can just append our arguments after the
       * command and initial arguments.
       */
      execp->replace_vec = NULL;
      execp->ctl.replace_pat = NULL;
      execp->ctl.rplen = 0;
      execp->ctl.lines_per_exec = 0; /* no limit */
      execp->ctl.args_per_exec = 0; /* no limit */

      /* remember how many arguments there are */
      execp->ctl.initial_argc = (end-start) - 1;

      /* execp->state = xmalloc(sizeof struct buildcmd_state); */
      bc_init_state(&execp->ctl, &execp->state, execp);

      /* Gather the initial arguments.  Skip the {}. */
      for (i=start; i<end-1; ++i)
	{
	  bc_push_arg(&execp->ctl, &execp->state,
		      argv[i], strlen(argv[i])+1,
		      NULL, 0,
		      1);
	}
    }
  else
    {
      /* Semicolon terminator - more than one {} is supported, so we
       * have to do brace-replacement.
       */
      execp->num_args = end - start;

      execp->ctl.replace_pat = "{}";
      execp->ctl.rplen = strlen(execp->ctl.replace_pat);
      execp->ctl.lines_per_exec = 0; /* no limit */
      execp->ctl.args_per_exec = 0; /* no limit */
      execp->replace_vec = xmalloc(sizeof(char*)*execp->num_args);


      /* execp->state = xmalloc(sizeof(*(execp->state))); */
      bc_init_state(&execp->ctl, &execp->state, execp);

      /* Remember the (pre-replacement) arguments for later. */
      for (i=0; i<execp->num_args; ++i)
	{
	  execp->replace_vec[i] = argv[i+start];
	}
    }

  if (argv[end] == NULL)
    *arg_ptr = end;
  else
    *arg_ptr = end + 1;

  return true;
}



static boolean
insert_exec_ok (const char *action,
		const struct parser_table *entry,
		int dir_fd,
		char **argv,
		int *arg_ptr)
{
  return new_insert_exec_ok(action, entry, dir_fd, argv, arg_ptr);
}



/* Get a timestamp and comparison type.

   STR is the ASCII representation.
   Set *NUM_DAYS to the number of days/minutes/whatever, taken as being
   relative to ORIGIN (usually the current moment or midnight).
   Thus the sense of the comparison type appears to be reversed.
   Set *COMP_TYPE to the kind of comparison that is requested.
   Issue OVERFLOWMESSAGE if overflow occurs.
   Return true if all okay, false if input error.

   Used by -atime, -ctime and -mtime (parsers) to
   get the appropriate information for a time predicate processor. */

static boolean
get_relative_timestamp (const char *str,
			struct time_val *result,
			struct timespec origin,
			double sec_per_unit,
			const char *overflowmessage)
{
  double offset, seconds, nanosec;
  static const long nanosec_per_sec = 1000000000;

  if (get_comp_type(&str, &result->kind))
    {
      /* Invert the sense of the comparison */
      switch (result->kind)
	{
	case COMP_LT: result->kind = COMP_GT; break;
	case COMP_GT: result->kind = COMP_LT; break;
	default: break;
	}

      /* Convert the ASCII number into floating-point. */
      if (xstrtod(str, NULL, &offset, strtod))
	{
	  /* Separate the floating point number the user specified
	   * (which is a number of days, or minutes, etc) into an
	   * integral number of seconds (SECONDS) and a fraction (NANOSEC).
	   */
	  nanosec = modf(offset * sec_per_unit, &seconds);
	  nanosec *= 1.0e9;	/* convert from fractional seconds to ns. */
	  assert (nanosec < nanosec_per_sec);

	  /* Perform the subtraction, and then check for overflow.
	   * On systems where signed aritmetic overflow does not
	   * wrap, this check may be unreliable.   The C standard
	   * does not require this approach to work, but I am aware
	   * of no platforms where it fails.
	   */
	  result->ts.tv_sec  = origin.tv_sec - seconds;
	  if ((origin.tv_sec < result->ts.tv_sec) != (seconds < 0))
	    {
	      /* an overflow has occurred. */
	      error (1, 0, overflowmessage, str);
	    }

	  result->ts.tv_nsec = origin.tv_nsec - nanosec;
	  if (origin.tv_nsec < nanosec)
	    {
	      /* Perform a carry operation */
	      result->ts.tv_nsec += nanosec_per_sec;
	      result->ts.tv_sec  -= 1;
	    }
	  return true;
	}
      else
	{
	  /* Conversion from ASCII to double failed. */
	  return false;
	}
    }
  else
    {
      return false;
    }
}

/* Insert a time predicate based on the information in ENTRY.
   ARGV is a pointer to the argument array.
   ARG_PTR is a pointer to an index into the array, incremented if
   all went well.

   Return true if input is valid, false if not.

   A new predicate node is assigned, along with an argument node
   obtained with malloc.

   Used by -atime, -ctime, and -mtime parsers. */

static boolean
parse_time (const struct parser_table* entry, char *argv[], int *arg_ptr)
{
  struct predicate *our_pred;
  struct time_val tval;
  enum comparison_type comp;
  const char *timearg, *orig_timearg;
  const char *errmsg = "arithmetic overflow while converting %s "
    "days to a number of seconds";
  struct timespec origin;

  if (!collect_arg(argv, arg_ptr, &timearg))
    return false;
  orig_timearg = timearg;

  /* Decide the origin by previewing the comparison type. */
  origin = options.cur_day_start;

  if (get_comp_type(&timearg, &comp))
    {
      /* Remember, we invert the sense of the comparison, so this tests
       * against COMP_LT instead of COMP_GT...
       */
      if (COMP_LT == comp)
	{
	  uintmax_t expected = origin.tv_sec + (DAYSECS-1);
	  origin.tv_sec += (DAYSECS-1);
	  if (origin.tv_sec != expected)
	    {
	      error(1, 0,
		    _("arithmetic overflow when trying to calculate the end of today"));
	    }
	}
    }
  /* We discard the value of comp here, as get_relative_timestamp
   * will set tval.kind.  For that to work, we have to restore
   * timearg so that it points to the +/- prefix, if any.  get_comp_type()
   * will have advanced timearg, so we restore it.
   */
  timearg = orig_timearg;

  if (!get_relative_timestamp(timearg, &tval, origin, DAYSECS, errmsg))
    return false;

  our_pred = insert_primary (entry);
  our_pred->args.reftime = tval;
  our_pred->est_success_rate = estimate_timestamp_success_rate(tval.ts.tv_sec);

  if (options.debug_options & DebugExpressionTree)
    {
      time_t t;

      fprintf (stderr, "inserting %s\n", our_pred->p_name);
      fprintf (stderr, "    type: %s    %s  ",
	       (tval.kind == COMP_GT) ? "gt" :
	       ((tval.kind == COMP_LT) ? "lt" : ((tval.kind == COMP_EQ) ? "eq" : "?")),
	       (tval.kind == COMP_GT) ? " >" :
	       ((tval.kind == COMP_LT) ? " <" : ((tval.kind == COMP_EQ) ? ">=" : " ?")));
      t = our_pred->args.reftime.ts.tv_sec;
      fprintf (stderr, "%ju %s",
	       (uintmax_t) our_pred->args.reftime.ts.tv_sec,
	       ctime (&t));
      if (tval.kind == COMP_EQ)
	{
	  t = our_pred->args.reftime.ts.tv_sec + DAYSECS;
	  fprintf (stderr, "                 <  %ju %s",
		   (uintmax_t) t, ctime (&t));
	}
    }

  return true;
}

/* Get the comparison type prefix (if any) from a number argument.
   The prefix is at *STR.
   Set *COMP_TYPE to the kind of comparison that is requested.
   Advance *STR beyond any initial comparison prefix.

   Return true if all okay, false if input error.  */
static boolean
get_comp_type(const char **str, enum comparison_type *comp_type)
{
  switch (**str)
    {
    case '+':
      *comp_type = COMP_GT;
      (*str)++;
      break;
    case '-':
      *comp_type = COMP_LT;
      (*str)++;
      break;
    default:
      *comp_type = COMP_EQ;
      break;
    }
  return true;
}





/* Get a number with comparison information.
   The sense of the comparison information is 'normal'; that is,
   '+' looks for a count > than the number and '-' less than.

   STR is the ASCII representation of the number.
   Set *NUM to the number.
   Set *COMP_TYPE to the kind of comparison that is requested.

   Return true if all okay, false if input error.  */

static boolean
get_num (const char *str,
	 uintmax_t *num,
	 enum comparison_type *comp_type)
{
  char *pend;

  if (str == NULL)
    return false;

  /* Figure out the comparison type if the caller accepts one. */
  if (comp_type)
    {
      if (!get_comp_type(&str, comp_type))
	return false;
    }

  return xstrtoumax (str, &pend, 10, num, "") == LONGINT_OK;
}

/* Insert a number predicate.
   ARGV is a pointer to the argument array.
   *ARG_PTR is an index into ARGV, incremented if all went well.
   *PRED is the predicate processor to insert.

   Return true if input is valid, false if error.

   A new predicate node is assigned, along with an argument node
   obtained with malloc.

   Used by -inum and -links parsers. */

static struct predicate *
insert_num (char **argv, int *arg_ptr, const struct parser_table *entry)
{
  const char *numstr;

  if (collect_arg(argv, arg_ptr, &numstr))
  {
    uintmax_t num;
    enum comparison_type c_type;

    if (get_num (numstr, &num, &c_type))
      {
	struct predicate *our_pred = insert_primary (entry);
	our_pred->args.numinfo.kind = c_type;
	our_pred->args.numinfo.l_val = num;

	if (options.debug_options & DebugExpressionTree)
	  {
	    fprintf (stderr, "inserting %s\n", our_pred->p_name);
	    fprintf (stderr, "    type: %s    %s  ",
		     (c_type == COMP_GT) ? "gt" :
		     ((c_type == COMP_LT) ? "lt" : ((c_type == COMP_EQ) ? "eq" : "?")),
		     (c_type == COMP_GT) ? " >" :
		     ((c_type == COMP_LT) ? " <" : ((c_type == COMP_EQ) ? " =" : " ?")));
	    fprintf (stderr, "%ju\n", our_pred->args.numinfo.l_val);
	  }
	return our_pred;
      }
  }
  return NULL;
}

static void
open_output_file (const char *path, struct format_val *p)
{
  p->segment = NULL;
  p->quote_opts = clone_quoting_options (NULL);

  if (!strcmp (path, "/dev/stderr"))
    {
      p->stream = stderr;
      p->filename = _("standard error");
    }
  else if (!strcmp (path, "/dev/stdout"))
    {
      p->stream = stdout;
      p->filename = _("standard output");
    }
  else
    {
      p->stream = fopen_safer (path, "w");
      p->filename = path;

      if (p->stream == NULL)
	{
	  fatal_file_error(path);
	}
    }

  p->dest_is_tty = stream_is_tty(p->stream);
}

static void
open_stdout (struct format_val *p)
{
  open_output_file("/dev/stdout", p);
}
