/* defs.h -- data types and declarations.
   Copyright (C) 1990, 91, 92, 93, 94, 2000, 2004, 2005,
                 2006, 2007, 2008 Free Software Foundation, Inc.

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


#ifndef INC_DEFS_H
#define INC_DEFS_H 1

#if !defined(ALREADY_INCLUDED_CONFIG_H)
/*
 * Savannah bug #20128: if we include some system header and it
 * includes some othersecond system header, the second system header
 * may in fact turn out to be a file provided by gnulib.  For that
 * situation, we need to have already included <config.h> so that the
 * Gnulib files have access to the information probed by their
 * configure script fragments.  So <config.h> should be the first
 * thing included.
 */
#error "<config.h> should be #included before defs.h, and indeed before any other header"
Please stop compiling the program now
#endif


#include <sys/types.h>

/* XXX: some of these includes probably don't belong in a common header file */
#include <sys/stat.h>
#include <stdio.h>		/* for FILE* */
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <limits.h>		/* for CHAR_BIT */
#include <stdbool.h>		/* for bool/boolean */
#include <stdint.h>		/* for uintmax_t */
#include <sys/stat.h> /* S_ISUID etc. */



#ifndef CHAR_BIT
# define CHAR_BIT 8
#endif

#if HAVE_INTTYPES_H
# include <inttypes.h>
#endif
typedef bool boolean;

#include "regex.h"
#include "timespec.h"
#include "buildcmd.h"
#include "quotearg.h"

/* These days we will assume ANSI/ISO C protootypes work on our compiler. */
#define PARAMS(Args) Args

#ifndef ATTRIBUTE_NORETURN
# if HAVE_ATTRIBUTE_NORETURN
#  define ATTRIBUTE_NORETURN __attribute__ ((__noreturn__))
# else
#  define ATTRIBUTE_NORETURN /* nothing */
# endif
#endif

int optionl_stat PARAMS((const char *name, struct stat *p));
int optionp_stat PARAMS((const char *name, struct stat *p));
int optionh_stat PARAMS((const char *name, struct stat *p));
int debug_stat   PARAMS((const char *file, struct stat *bufp));

void set_stat_placeholders PARAMS((struct stat *p));
int get_statinfo PARAMS((const char *pathname, const char *name, struct stat *p));


#define MODE_WXUSR	(S_IWUSR | S_IXUSR)
#define MODE_R		(S_IRUSR | S_IRGRP | S_IROTH)
#define MODE_RW		(S_IWUSR | S_IWGRP | S_IWOTH | MODE_R)
#define MODE_RWX	(S_IXUSR | S_IXGRP | S_IXOTH | MODE_RW)
#define MODE_ALL	(S_ISUID | S_ISGID | S_ISVTX | MODE_RWX)


struct predicate;
struct options;

/* Pointer to a predicate function. */
typedef boolean (*PRED_FUNC)(const char *pathname, struct stat *stat_buf, struct predicate *pred_ptr);

/* The number of seconds in a day. */
#define		DAYSECS	    86400

/* Argument structures for predicates. */

enum comparison_type
{
  COMP_GT,
  COMP_LT,
  COMP_EQ
};

enum permissions_type
{
  PERM_AT_LEAST,
  PERM_ANY,
  PERM_EXACT
};

enum predicate_type
{
  NO_TYPE,
  PRIMARY_TYPE,
  UNI_OP,
  BI_OP,
  OPEN_PAREN,
  CLOSE_PAREN
};

enum predicate_precedence
{
  NO_PREC,
  COMMA_PREC,
  OR_PREC,
  AND_PREC,
  NEGATE_PREC,
  MAX_PREC
};

struct long_val
{
  enum comparison_type kind;
  boolean negative;		/* Defined only when representing time_t.  */
  uintmax_t l_val;
};

struct perm_val
{
  enum permissions_type kind;
  mode_t val[2];
};

/* dir_id is used to support loop detection in find.c
 */
struct dir_id
{
  ino_t ino;
  dev_t dev;
};

/* samefile_file_id is used to support the -samefile test.
 */
struct samefile_file_id
{
  ino_t ino;
  dev_t dev;
  int   fd;
};

struct size_val
{
  enum comparison_type kind;
  int blocksize;
  uintmax_t size;
};


enum xval 
  {
    XVAL_ATIME, XVAL_BIRTHTIME, XVAL_CTIME, XVAL_MTIME, XVAL_TIME
  };

struct time_val
{
  enum xval            xval; 
  enum comparison_type kind;
  struct timespec      ts;
};

    
struct exec_val
{
  boolean multiple;		/* -exec {} \+ denotes multiple argument. */
  struct buildcmd_control ctl;
  struct buildcmd_state   state;
  char **replace_vec;		/* Command arguments (for ";" style) */
  int num_args;
  boolean use_current_dir;      /* If nonzero, don't chdir to start dir */
  boolean close_stdin;		/* If true, close stdin in the child. */
  int dir_fd;			/* The directory to do the exec in. */
};

/* The format string for a -printf or -fprintf is chopped into one or
   more `struct segment', linked together into a list.
   Each stretch of plain text is a segment, and
   each \c and `%' conversion is a segment. */

/* Special values for the `kind' field of `struct segment'. */
enum SegmentKind 
  {
    KIND_PLAIN=0,		/* Segment containing just plain text. */
    KIND_STOP=1,		/* \c -- stop printing and flush output. */
    KIND_FORMAT,		/* Regular format */
  };

struct segment
{
  enum SegmentKind segkind;     /* KIND_FORMAT, KIND_PLAIN, KIND_STOP */
  char format_char[2];		/* Format chars if kind is KIND_FORMAT */
  char *text;			/* Plain text or `%' format string. */
  int text_len;			/* Length of `text'. */
  struct segment *next;		/* Next segment for this predicate. */
};

struct format_val
{
  struct segment *segment;	/* Linked list of segments. */
  FILE *stream;			/* Output stream to print on. */
  const char *filename;		/* We need the filename for error messages. */
  boolean dest_is_tty;		/* True if the destination is a terminal. */
  struct quoting_options *quote_opts;
};

/* Profiling information for a predicate */
struct predicate_performance_info
{
  unsigned long visits;
  unsigned long successes;
};

/* evaluation cost of a predicate */
enum EvaluationCost
{
  NeedsNothing,
  NeedsType,
  NeedsStatInfo,
  NeedsLinkName,
  NeedsAccessInfo,
  NeedsSyncDiskHit,
  NeedsEventualExec,
  NeedsImmediateExec,
  NeedsUserInteraction,
  NeedsUnknown,
  NumEvaluationCosts
};
    
struct predicate
{
  /* Pointer to the function that implements this predicate.  */
  PRED_FUNC pred_func;

  /* Only used for debugging, but defined unconditionally so individual
     modules can be compiled with -DDEBUG.  */
  char *p_name;

  /* The type of this node.  There are two kinds.  The first is real
     predicates ("primaries") such as -perm, -print, or -exec.  The
     other kind is operators for combining predicates. */
  enum predicate_type p_type;

  /* The precedence of this node.  Only has meaning for operators. */
  enum predicate_precedence p_prec;

  /* True if this predicate node produces side effects.
     If side_effects are produced
     then optimization will not be performed */
  boolean side_effects;

  /* True if this predicate node requires default print be turned off. */
  boolean no_default_print;

  /* True if this predicate node requires a stat system call to execute. */
  boolean need_stat;

  /* True if this predicate node requires knowledge of the file type. */
  boolean need_type;

  enum EvaluationCost p_cost;

  /* est_success_rate is a number between 0.0 and 1.0 */
  float est_success_rate;
  
  /* True if this predicate should display control characters literally */
  boolean literal_control_chars;

  /* True if this predicate didn't originate from the user. */
  boolean artificial;

  /* The raw text of the argument of this predicate. */
  char *arg_text;
  
  /* Information needed by the predicate processor.
     Next to each member are listed the predicates that use it. */
  union
  {
    const char *str;		/* fstype [i]lname [i]name [i]path */
    struct re_pattern_buffer *regex; /* regex */
    struct exec_val exec_vec;	/* exec ok */
    struct long_val numinfo;	/* gid inum links  uid */
    struct size_val size;	/* size */
    uid_t uid;			/* user */
    gid_t gid;			/* group */
    struct time_val reftime;	/* newer newerXY anewer cnewer mtime atime ctime mmin amin cmin */
    struct perm_val perm;	/* perm */
    struct samefile_file_id samefileid; /* samefile */
    mode_t type;		/* type */
    struct format_val printf_vec; /* printf fprintf fprint ls fls print0 fprint0 print */
  } args;

  /* The next predicate in the user input sequence,
     which represents the order in which the user supplied the
     predicates on the command line. */
  struct predicate *pred_next;

  /* The right and left branches from this node in the expression
     tree, which represents the order in which the nodes should be
     processed. */
  struct predicate *pred_left;
  struct predicate *pred_right;

  struct predicate_performance_info perf;
  
  const struct parser_table* parser_entry;
};

/* find.c, ftsfind.c */
boolean is_fts_enabled(int *ftsoptions);
int get_start_dirfd(void);
int get_current_dirfd(void);

/* find library function declarations.  */

/* find global function declarations.  */

/* find.c */
/* SymlinkOption represents the choice of 
 * -P, -L or -P (default) on the command line.
 */
enum SymlinkOption 
  {
    SYMLINK_NEVER_DEREF,	/* Option -P */
    SYMLINK_ALWAYS_DEREF,	/* Option -L */
    SYMLINK_DEREF_ARGSONLY	/* Option -H */
  };
extern enum SymlinkOption symlink_handling; /* defined in find.c. */

void set_follow_state PARAMS((enum SymlinkOption opt));
void cleanup(void);

/* fstype.c */
char *filesystem_type PARAMS((const struct stat *statp, const char *path));
char * get_mounted_filesystems (void);
dev_t * get_mounted_devices PARAMS((size_t *));



enum arg_type
  {
    ARG_OPTION,			/* regular options like -maxdepth */
    ARG_NOOP,			/* does nothing, returns true, internal use only */
    ARG_POSITIONAL_OPTION,	/* options whose position is important (-follow) */
    ARG_TEST,			/* a like -name */
    ARG_SPECIAL_PARSE,		/* complex to parse, don't eat the test name before calling parse_xx(). */
    ARG_PUNCTUATION,		/* like -o or ( */
    ARG_ACTION			/* like -print */
  };


struct parser_table;
/* Pointer to a parser function. */
typedef boolean (*PARSE_FUNC)(const struct parser_table *p,
			      char *argv[], int *arg_ptr);
struct parser_table
{
  enum arg_type type;
  char *parser_name;
  PARSE_FUNC parser_func;
  PRED_FUNC    pred_func;
};

/* parser.c */
const struct parser_table* find_parser PARAMS((char *search_name));
boolean parse_print PARAMS((const struct parser_table*, char *argv[], int *arg_ptr));
void pred_sanity_check PARAMS((const struct predicate *predicates));
void check_option_combinations (const struct predicate *p);
void parse_begin_user_args PARAMS((char **args, int argno, const struct predicate *last, const struct predicate *predicates));
void parse_end_user_args PARAMS((char **args, int argno, const struct predicate *last, const struct predicate *predicates));
boolean parse_openparen              PARAMS((const struct parser_table* entry, char *argv[], int *arg_ptr));
boolean parse_closeparen             PARAMS((const struct parser_table* entry, char *argv[], int *arg_ptr));

/* pred.c */

typedef boolean PREDICATEFUNCTION(const char *pathname, struct stat *stat_buf, struct predicate *pred_ptr);
PREDICATEFUNCTION pred_amin;
PREDICATEFUNCTION pred_and;
PREDICATEFUNCTION pred_anewer;
PREDICATEFUNCTION pred_atime;
PREDICATEFUNCTION pred_closeparen;
PREDICATEFUNCTION pred_cmin;
PREDICATEFUNCTION pred_cnewer;
PREDICATEFUNCTION pred_comma;
PREDICATEFUNCTION pred_ctime;
PREDICATEFUNCTION pred_delete;
PREDICATEFUNCTION pred_empty;
PREDICATEFUNCTION pred_exec;
PREDICATEFUNCTION pred_execdir;
PREDICATEFUNCTION pred_executable;
PREDICATEFUNCTION pred_false;
PREDICATEFUNCTION pred_fls;
PREDICATEFUNCTION pred_fprint;
PREDICATEFUNCTION pred_fprint0;
PREDICATEFUNCTION pred_fprintf;
PREDICATEFUNCTION pred_fstype;
PREDICATEFUNCTION pred_gid;
PREDICATEFUNCTION pred_group;
PREDICATEFUNCTION pred_ilname;
PREDICATEFUNCTION pred_iname;
PREDICATEFUNCTION pred_inum;
PREDICATEFUNCTION pred_ipath;
PREDICATEFUNCTION pred_links;
PREDICATEFUNCTION pred_lname;
PREDICATEFUNCTION pred_ls;
PREDICATEFUNCTION pred_mmin;
PREDICATEFUNCTION pred_mtime;
PREDICATEFUNCTION pred_name;
PREDICATEFUNCTION pred_negate;
PREDICATEFUNCTION pred_newer;
PREDICATEFUNCTION pred_newerXY;
PREDICATEFUNCTION pred_nogroup;
PREDICATEFUNCTION pred_nouser;
PREDICATEFUNCTION pred_ok;
PREDICATEFUNCTION pred_okdir;
PREDICATEFUNCTION pred_openparen;
PREDICATEFUNCTION pred_or;
PREDICATEFUNCTION pred_path;
PREDICATEFUNCTION pred_perm;
PREDICATEFUNCTION pred_print;
PREDICATEFUNCTION pred_print0;
PREDICATEFUNCTION pred_prune;
PREDICATEFUNCTION pred_quit;
PREDICATEFUNCTION pred_readable;
PREDICATEFUNCTION pred_regex;
PREDICATEFUNCTION pred_samefile;
PREDICATEFUNCTION pred_size;
PREDICATEFUNCTION pred_true;
PREDICATEFUNCTION pred_type;
PREDICATEFUNCTION pred_uid;
PREDICATEFUNCTION pred_used;
PREDICATEFUNCTION pred_user;
PREDICATEFUNCTION pred_writable;
PREDICATEFUNCTION pred_xtype;



int launch PARAMS((const struct buildcmd_control *ctl,
		   struct buildcmd_state *buildstate));


char *find_pred_name PARAMS((PRED_FUNC pred_func));



void print_predicate PARAMS((FILE *fp, const struct predicate *p));
void print_tree PARAMS((FILE*, struct predicate *node, int indent));
void print_list PARAMS((FILE*, struct predicate *node));
void print_optlist PARAMS((FILE *fp, const struct predicate *node));
void show_success_rates(const struct predicate *node);


/* tree.c */
struct predicate * build_expression_tree PARAMS((int argc, char *argv[], int end_of_leading_options));
struct predicate * get_eval_tree PARAMS((void));
struct predicate *get_new_pred PARAMS((const struct parser_table *entry));
struct predicate *get_new_pred_chk_op PARAMS((const struct parser_table *entry));
float  calculate_derived_rates PARAMS((struct predicate *p));

/* util.c */
struct predicate *insert_primary PARAMS((const struct parser_table *entry));
struct predicate *insert_primary_withpred PARAMS((const struct parser_table *entry, PRED_FUNC fptr));
void usage PARAMS((FILE *fp, int status, char *msg));
extern boolean check_nofollow(void);
void complete_pending_execs(struct predicate *p);
void complete_pending_execdirs(int dir_fd); /* Passing dir_fd is an unpleasant CodeSmell. */
const char *safely_quote_err_filename (int n, char const *arg);
void fatal_file_error(const char *name) ATTRIBUTE_NORETURN;
void nonfatal_file_error(const char *name);

int process_leading_options PARAMS((int argc, char *argv[]));
void set_option_defaults PARAMS((struct options *p));

#if 0
#define apply_predicate(pathname, stat_buf_ptr, node)	\
  (*(node)->pred_func)((pathname), (stat_buf_ptr), (node))
#else
boolean apply_predicate(const char *pathname, struct stat *stat_buf, struct predicate *p);
#endif

#define pred_is(node, fn) ( ((node)->pred_func) == (fn) )


/* find.c. */
int get_info PARAMS((const char *pathname, struct stat *p, struct predicate *pred_ptr));
int following_links PARAMS((void));
int digest_mode PARAMS((mode_t mode, const char *pathname, const char *name, struct stat *pstat, boolean leaf));
boolean default_prints PARAMS((struct predicate *pred));
boolean looks_like_expression PARAMS((const char *arg, boolean leading));


enum DebugOption
  {
    DebugNone             = 0,
    DebugExpressionTree   = 1,
    DebugStat             = 2,
    DebugSearch           = 4,
    DebugTreeOpt          = 8,
    DebugHelp             = 16,
    DebugExec             = 32,
    DebugSuccessRates     = 64
  };

struct options
{
  /* If true, process directory before contents.  True unless -depth given. */
  boolean do_dir_first;
  /* If true, -depth was EXPLICITLY set (as opposed to having been turned 
   * on by -delete, for example).
   */
   boolean explicit_depth;
  
  /* If >=0, don't descend more than this many levels of subdirectories. */
  int maxdepth;
  
  /* If >=0, don't process files above this level. */
  int mindepth;
  
  /* If true, do not assume that files in directories with nlink == 2
     are non-directories. */
  boolean no_leaf_check;
  
  /* If true, don't cross filesystem boundaries. */
  boolean stay_on_filesystem;
  
  /* If true, we ignore the problem where we find that a directory entry 
   * no longer exists by the time we get around to processing it.
   */
  boolean ignore_readdir_race;

  /* If true, pass control characters through.  If false, escape them
   * or turn them into harmless things.
   */
  boolean literal_control_chars;
  
  /* If true, we issue warning messages
   */
  boolean warnings;
  
  /* If true, avoid POSIX-incompatible behaviours 
   * (this functionality is currently incomplete 
   * and at the moment affects mainly warning messages).
   */
  boolean posixly_correct;
  
  struct timespec      start_time;		/* Time at start of execution.  */
  
  /* Either one day before now (the default), or the start of today (if -daystart is given). */
  struct timespec      cur_day_start;
  
  /* If true, cur_day_start has been adjusted to the start of the day. */
  boolean full_days;
  
  int output_block_size;	/* Output block size.  */

  /* bitmask for debug options */
  unsigned long debug_options;
  
  enum SymlinkOption symlink_handling;
  
  
  /* Pointer to the function used to stat files. */
  int (*xstat) (const char *name, struct stat *statbuf);


  /* Indicate if we can implement safely_chdir() using the O_NOFOLLOW 
   * flag to open(2). 
   */
  boolean open_nofollow_available;

  /* The variety of regular expression that we support.
   * The default is POSIX Basic Regular Expressions, but this 
   * can be changed with the positional option, -regextype.
   */
  int regex_options;

  /* Optimisation level.  One is the default. 
   */
  unsigned short optimisation_level;


  /* How should we quote filenames in error messages and so forth?
   */
  enum quoting_style err_quoting_style;
};
extern struct options options;


struct state
{
  /* Current depth; 0 means current path is a command line arg. */
  int curdepth;
  
  /* If true, we have called stat on the current path. */
  boolean have_stat;
  
  /* If true, we know the type of the current path. */
  boolean have_type;
  mode_t type;			/* this is the actual type */
  
  /* The file being operated on, relative to the current directory.
     Used for stat, readlink, remove, and opendir.  */
  char *rel_pathname;
  /* The directory fd to which rel_pathname is relative.  Thsi is relevant
   * when we're navigating the hierarchy with fts() and using FTS_CWDFD.
   */
  int cwd_dir_fd;

  /* Length of starting path. */
  int starting_path_length;

  /* If true, don't descend past current directory.
     Can be set by -prune, -maxdepth, and -xdev/-mount. */
  boolean stop_at_current_level;
  
  /* Status value to return to system. */
  int exit_status;

  /* True if there are any execdirs.  This saves us a pair of fchdir()
   * calls for every directory we leave if it is false.  This is just
   * an optimisation.  Set to true if you want to be conservative.
   */
  boolean execdirs_outstanding;
};

/* finddata.c */
extern struct state state;
extern char const *starting_dir;
extern int starting_desc;
extern char *program_name;


#endif
