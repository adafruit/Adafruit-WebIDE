/* buildcmd.c -- build command lines from a list of arguments.
   Copyright (C) 1990, 91, 92, 93, 94, 2000, 2003, 2005, 2006,
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

/*
 XXX_SOC:
 
 One of the aspects of the SOC project is to adapt this module.  
 This module currently makes an initial guess at two things:
  
   buildcmd_control->arg_max   (The most characters we can fit in)
   buildcmd_control->max_arg_count (most args)

 The nature of the SOC task is to adjust these values when exec fails.
 Optionally (if we have the time) we can make the software adjust them 
 when exec succeeds.  If we do the latter, we need to ensure we don't
 get into some state where we are sitting just below the limit and 
 keep trying to extend, because that would lead to every other exec
 failing. 

 If our initial guess is successful, there is no pressing need really to 
 increase our guess.  Indeed, if we are beign called by xargs (as opposed
 to find) th user may have specified a limit with "-s" and we should not 
 exceed it.
*/


#include <config.h>

# ifndef PARAMS
#  if defined PROTOTYPES || (defined __STDC__ && __STDC__)
#   define PARAMS(Args) Args
#  else
#   define PARAMS(Args) ()
#  endif
# endif

#include <string.h>


#if DO_MULTIBYTE
# if HAVE_MBRLEN
#  include <wchar.h>
# else
   /* Simulate mbrlen with mblen as best we can.  */
#  define mbstate_t int
#  define mbrlen(s, n, ps) mblen (s, n)
# endif
#endif

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

#ifndef _POSIX_SOURCE
#include <sys/param.h>
#endif

#ifdef HAVE_LIMITS_H
#include <limits.h>
#endif

/* The presence of unistd.h is assumed by gnulib these days, so we 
 * might as well assume it too. 
 */
/* for sysconf() */
#include <unistd.h>

#include <assert.h>

/* COMPAT:  SYSV version defaults size (and has a max value of) to 470.
   We try to make it as large as possible.  See bc_get_arg_max() below. */
#if !defined(ARG_MAX) && defined(NCARGS)
#error "You have an unusual system.  Once you remove this error message from buildcmd.c, it should work, but please make sure that DejaGnu is installed on your system and that 'make check' passes before using the findutils programs"
#define ARG_MAX NCARGS
#endif



#include <xalloc.h>
#include <error.h>
#include <openat.h>

#include "buildcmd.h"


extern char **environ;


/* Replace all instances of `replace_pat' in ARG with `linebuf',
   and add the resulting string to the list of arguments for the command
   to execute.
   ARGLEN is the length of ARG, not including the null.
   LBLEN is the length of LINEBUF, not including the null.
   PFXLEN is the length of PREFIX.  Substitution is not performed on
   the prefix.   The prefix is used if the argument contains replace_pat.

   COMPAT: insertions on the SYSV version are limited to 255 chars per line,
   and a max of 5 occurrences of replace_pat in the initial-arguments.
   Those restrictions do not exist here.  */

void
bc_do_insert (const struct buildcmd_control *ctl,
              struct buildcmd_state *state,
              char *arg, size_t arglen,
              const char *prefix, size_t pfxlen,
              const char *linebuf, size_t lblen,
              int initial_args)
{
  /* Temporary copy of each arg with the replace pattern replaced by the
     real arg.  */
  static char *insertbuf;
  char *p;
  size_t bytes_left = ctl->arg_max - 1;    /* Bytes left on the command line.  */

  /* XXX: on systems lacking an upper limit for exec args, ctl->arg_max
   *      may have been set to LONG_MAX (see bc_get_arg_max()).  Hence
   *      this xmalloc call may be a bad idea, especially since we are
   *      adding 1 to it...
   */
  if (!insertbuf)
    insertbuf = xmalloc (ctl->arg_max + 1);
  p = insertbuf;

  do
    {
      size_t len;               /* Length in ARG before `replace_pat'.  */
      char *s = mbsstr (arg, ctl->replace_pat);
      if (s)
        {
          len = s - arg;
        }
      else
        {
          len = arglen;
        }
      
      if (bytes_left <= len)
        break;
      else
	bytes_left -= len;

      strncpy (p, arg, len);
      p += len;
      arg += len;
      arglen -= len;

      if (s)
        {
	  if (bytes_left <= (lblen + pfxlen))
	    break;
	  else
	    bytes_left -= (lblen + pfxlen);
	  
	  if (prefix)
	    {
	      strcpy (p, prefix);
	      p += pfxlen;
	    }
          strcpy (p, linebuf);
          p += lblen;

          arg += ctl->rplen;
          arglen -= ctl->rplen;
        }
    }
  while (*arg);
  if (*arg)
    error (1, 0, _("command too long"));
  *p++ = '\0';
  
  bc_push_arg (ctl, state,
	       insertbuf, p - insertbuf,
               NULL, 0,
               initial_args);
}

static
void do_exec(const struct buildcmd_control *ctl,
             struct buildcmd_state *state)
{
  /* XXX_SOC:
   
     Here we are calling the user's function.  Currently there is no
     way for it to report that the argument list was too long.  We
     should introduce an externally callable function that allows them
     to report this.

     If the callee does report that the exec failed, we need to retry
     the exec with a shorter argument list.  Once we have reduced the
     argument list to the point where the exec can succeed, we need to 
     preserve the list of arguments we couldn't exec this time. 

     This also means that the control argument here probably needs not 
     to be const (since the limits are in the control arg).

     The caller's only requirement on do_exec is that it should 
     free up enough room for at least one argument.   
   */
  (ctl->exec_callback)(ctl, state);
}


/* Return nonzero if there would not be enough room for an additional
 * argument.  We check the total number of arguments only, not the space
 * occupied by those arguments.
 *
 * If we return zero, there still may not be enough room for the next
 * argument, depending on its length.
 */
static int 
bc_argc_limit_reached(int initial_args, 
		      const struct buildcmd_control *ctl,
		      struct buildcmd_state *state)
{
  /* Check to see if we about to exceed a limit set by xargs' -n option */
  if (!initial_args && ctl->args_per_exec &&
      ( (state->cmd_argc - ctl->initial_argc) == ctl->args_per_exec))
    return 1;

  /* We deliberately use an equality test here rather than >= in order
   * to force a software failure if the code is modified in such a way
   * that it fails to call this function for every new argument.
   */
  return state->cmd_argc == ctl->max_arg_count;
}


/* Add ARG to the end of the list of arguments `cmd_argv' to pass
   to the command.
   LEN is the length of ARG, including the terminating null.
   If this brings the list up to its maximum size, execute the command.  
*/
/* XXX: sometimes this function is called (internally) 
 *      just to push a NULL onto the and of the arg list.
 *      We should probably do that with a separate function
 *      for greater clarity.
 */
void
bc_push_arg (const struct buildcmd_control *ctl,
             struct buildcmd_state *state,
             const char *arg, size_t len,
             const char *prefix, size_t pfxlen,
             int initial_args)
{
  if (!initial_args)
    {
      state->todo = 1;
    }
  
  if (arg)
    {
      /* XXX_SOC: if do_exec() is only guaranteeed to free up one
       * argument, this if statement may need to become a while loop.
       * If it becomes a while loop, it needs not to be an infinite
       * loop...
       */
      if (state->cmd_argv_chars + len > ctl->arg_max)
        {
          if (initial_args || state->cmd_argc == ctl->initial_argc)
            error (1, 0, _("can not fit single argument within argument list size limit"));
          /* xargs option -i (replace_pat) implies -x (exit_if_size_exceeded) */
          if (ctl->replace_pat
              || (ctl->exit_if_size_exceeded &&
                  (ctl->lines_per_exec || ctl->args_per_exec)))
            error (1, 0, _("argument list too long"));
          do_exec (ctl, state);
        }
      /* XXX_SOC: this if may also need to become a while loop.  In
	 fact perhaps it is best to factor this out into a separate
	 function which ceeps calling the exec handler until there is
	 space for our next argument.  Each exec will free one argc
	 "slot" so the main thing to worry about repeated exec calls
	 for would be total argument length.
       */
      if (bc_argc_limit_reached(initial_args, ctl, state))
	do_exec (ctl, state);
    }

  if (state->cmd_argc >= state->cmd_argv_alloc)
    {
      /* XXX: we could use extendbuf() here. */
      if (!state->cmd_argv)
        {
          state->cmd_argv_alloc = 64;
          state->cmd_argv = xmalloc (sizeof (char *) * state->cmd_argv_alloc);
        }
      else
        {
          state->cmd_argv_alloc *= 2;
          state->cmd_argv = xrealloc (state->cmd_argv,
				      sizeof (char *) * state->cmd_argv_alloc);
        }
    }

  if (!arg)
    state->cmd_argv[state->cmd_argc++] = NULL;
  else
    {
      state->cmd_argv[state->cmd_argc++] = state->argbuf + state->cmd_argv_chars;
      if (prefix)
        {
          strcpy (state->argbuf + state->cmd_argv_chars, prefix);
          state->cmd_argv_chars += pfxlen;
        }
      
      strcpy (state->argbuf + state->cmd_argv_chars, arg);
      state->cmd_argv_chars += len;
      
      /* If we have now collected enough arguments,
       * do the exec immediately.  This must be 
       * conditional on arg!=NULL, since do_exec() 
       * actually calls bc_push_arg(ctl, state, NULL, 0, false).
       */
      if (bc_argc_limit_reached(initial_args, ctl, state))
	do_exec (ctl, state);
    }

  /* If this is an initial argument, set the high-water mark. */
  if (initial_args)
    {
      state->cmd_initial_argv_chars = state->cmd_argv_chars;
    }
}

#if 0
/* We used to set posix_arg_size_min to the LINE_MAX limit, but 
 * currently we use _POSIX_ARG_MAX (which is the minimum value).
 */
static size_t
get_line_max(void)
{
  long val;
#ifdef _SC_LINE_MAX  
  val = sysconf(_SC_LINE_MAX);
#else
  val = -1;
#endif
  
  if (val > 0)
    return val;

  /* either _SC_LINE_MAX was not available or 
   * there is no particular limit.
   */
#ifdef LINE_MAX
  val = LINE_MAX;
#endif

  if (val > 0)
    return val;

  return 2048L;			/* a reasonable guess. */
}
#endif

size_t
bc_get_arg_max(void)
{
  long val;

  /* We may resort to using LONG_MAX, so check it fits. */
  /* XXX: better to do a compile-time check */
  assert ( (~(size_t)0) >= LONG_MAX);

#ifdef _SC_ARG_MAX  
  val = sysconf(_SC_ARG_MAX);
#else
  val = -1;
#endif

  if (val > 0)
    return val;

  /* either _SC_ARG_MAX was not available or 
   * there is no particular limit.
   */
#ifdef ARG_MAX
  val = ARG_MAX;
#endif

  if (val > 0)
    return val;
  
  /* The value returned by this function bounds the
   * value applied as the ceiling for the -s option.
   * Hence it the system won't tell us what its limit
   * is, we allow the user to specify more or less 
   * whatever value they like.
   */
  return LONG_MAX;
}


static int cb_exec_noop(const struct buildcmd_control *ctl,
                         struct buildcmd_state *state)
{
  /* does nothing. */
  (void) ctl;
  (void) state;

  return 0;
}


/* Return how much of ARG_MAX is used by the environment.  */
size_t
bc_size_of_environment (void)
{
  size_t len = 0u;
  char **envp = environ;

  while (*envp)
    len += strlen (*envp++) + 1;

  return len;
}


enum BC_INIT_STATUS
bc_init_controlinfo(struct buildcmd_control *ctl,
		    size_t headroom)
{
  size_t size_of_environment = bc_size_of_environment();

  /* POSIX requires that _POSIX_ARG_MAX is 4096.  That is the lowest
   * possible value for ARG_MAX on a POSIX compliant system.  See
   * http://www.opengroup.org/onlinepubs/009695399/basedefs/limits.h.html
   */
  ctl->posix_arg_size_min = _POSIX_ARG_MAX;
  ctl->posix_arg_size_max = bc_get_arg_max();
  
  ctl->exit_if_size_exceeded = 0;

  /* Take the size of the environment into account.  */
  if (size_of_environment > ctl->posix_arg_size_max)
    {
      return BC_INIT_ENV_TOO_BIG;
    }
  else if ((headroom + size_of_environment) >= ctl->posix_arg_size_max)
    {
      /* POSIX.2 requires xargs to subtract 2048, but ARG_MAX is
       * guaranteed to be at least 4096.  Although xargs could use an
       * assertion here, we use a runtime check which returns an error
       * code, because our caller may not be xargs.
       */
      return BC_INIT_CANNOT_ACCOMODATE_HEADROOM;
    }
  else
    {
      ctl->posix_arg_size_max -= size_of_environment;
      ctl->posix_arg_size_max -= headroom;
    }
  
  /* need to subtract 2 on the following line - for Linux/PPC */
  ctl->max_arg_count = (ctl->posix_arg_size_max / sizeof(char*)) - 2u;
  assert (ctl->max_arg_count > 0);
  ctl->rplen = 0u;
  ctl->replace_pat = NULL;
  ctl->initial_argc = 0;
  ctl->exec_callback = cb_exec_noop;
  ctl->lines_per_exec = 0;
  ctl->args_per_exec = 0;

  /* Set the initial value of arg_max to the largest value we can
   * tolerate.
   */
  ctl->arg_max = ctl->posix_arg_size_max;

  return BC_INIT_OK;
}

void
bc_use_sensible_arg_max(struct buildcmd_control *ctl)
{
#ifdef DEFAULT_ARG_SIZE
  enum { arg_size = DEFAULT_ARG_SIZE };
#else
  enum { arg_size = (128u * 1024u) };
#endif

  /* Check against the upper and lower limits. */  
  if (arg_size > ctl->posix_arg_size_max)
    ctl->arg_max = ctl->posix_arg_size_max;
  else if (arg_size < ctl->posix_arg_size_min)
    ctl->arg_max = ctl->posix_arg_size_min;
  else 
    ctl->arg_max = arg_size;
}




void
bc_init_state(const struct buildcmd_control *ctl,
              struct buildcmd_state *state,
              void *context)
{
  state->cmd_argc = 0;
  state->cmd_argv_chars = 0;
  state->cmd_argv = NULL;
  state->cmd_argv_alloc = 0;
  
  /* XXX: the following memory allocation is inadvisable on systems
   * with no ARG_MAX, because ctl->arg_max may actually be close to 
   * LONG_MAX.   Adding one to it is safe though because earlier we
   * subtracted 2048.
   */
  assert (ctl->arg_max <= (LONG_MAX - 2048L));
  state->argbuf = xmalloc (ctl->arg_max + 1u);
  
  state->cmd_argv_chars = state->cmd_initial_argv_chars = 0;
  state->todo = 0;
  state->dir_fd = -1;
  state->usercontext = context;
}

void 
bc_clear_args(const struct buildcmd_control *ctl,
              struct buildcmd_state *state)
{
  state->cmd_argc = ctl->initial_argc;
  state->cmd_argv_chars = state->cmd_initial_argv_chars;
  state->todo = 0;
  state->dir_fd = -1;
}
