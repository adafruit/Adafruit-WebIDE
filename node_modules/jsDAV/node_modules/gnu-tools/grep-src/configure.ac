dnl
dnl autoconf input file for ``GNU grep''
dnl
dnl Copyright (C) 1997-2006, 2009-2012 Free Software Foundation, Inc.
dnl
dnl This file is part of GNU grep.
dnl
dnl This program is free software; you can redistribute it and/or modify
dnl it under the terms of the GNU General Public License as published by
dnl the Free Software Foundation; either version 3, or (at your option)
dnl any later version.
dnl
dnl This program is distributed in the hope that it will be useful,
dnl but WITHOUT ANY WARRANTY; without even the implied warranty of
dnl MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
dnl GNU General Public License for more details.
dnl
dnl You should have received a copy of the GNU General Public License
dnl along with this program.  If not, see <http://www.gnu.org/licenses/>.

AC_INIT([GNU grep],
        m4_esyscmd([build-aux/git-version-gen .tarball-version]),
        [bug-grep@gnu.org])

# Set the GREP and EGREP variables to a dummy replacement for the 'grep'
# command, so that AC_PROG_GREP and AC_PROG_EGREP don't fail when no good
# 'grep' program is found. This makes it possible to build GNU grep on a
# Solaris machine that has only /usr/bin/grep and no /usr/xpg4/bin/grep.
# This function supports only restricted arguments:
#   - No file names as arguments, process only standard input.
#   - Only literal strings without backslashes, no regular expressions.
#   - The only options are -e and -E (and -Ee).
# This function also does not support long lines beyond what the shell
# supports), and backslash-processes the input.
fn_grep () {
  test "$1" = -E && shift
  case $@%:@:$1 in
    0:*) AC_MSG_ERROR([fn_grep: expected pattern]) ;;
    1:-*) AC_MSG_ERROR([fn_grep: invalid command line]) ;;
    1:*) pattern=$1 ;;
    2:--|2:-e|2:-Ee) pattern=$2 ;;
    *) AC_MSG_ERROR([fn_grep: invalid command line]) ;;
  esac

  case $pattern in
    [*[].^$\\*[]*]) dnl The outer brackets are for M4.
      AC_MSG_ERROR([fn_grep: regular expressions not supported])  ;;
  esac

  rc=1
  while read line; do
    case $line in
      *$pattern*)
        rc=0
        AS_ECHO([$line]) ;;
    esac
  done
  return $rc
}

test -n "$GREP" || GREP=fn_grep
test -n "$EGREP" || EGREP=fn_grep
ac_cv_path_EGREP=$EGREP

AC_CONFIG_AUX_DIR(build-aux)
AC_CONFIG_SRCDIR(src/grep.c)
AC_DEFINE([GREP], 1, [We are building grep])
AC_PREREQ(2.59)

dnl Automake stuff.
AM_INIT_AUTOMAKE([1.11 no-dist-gzip dist-xz color-tests parallel-tests])
AM_SILENT_RULES([yes]) # make --enable-silent-rules the default.

AC_CONFIG_HEADERS([config.h:config.hin])

dnl Checks for programs.
AC_CANONICAL_HOST
AC_PROG_AWK
AC_PROG_INSTALL
AC_PROG_CC
gl_EARLY
AC_PROG_RANLIB

dnl Checks for typedefs, structures, and compiler characteristics.
AC_TYPE_SIZE_T
AC_C_CONST
gl_INIT

AC_ARG_ENABLE([gcc-warnings],
  [AS_HELP_STRING([--enable-gcc-warnings],
                  [turn on lots of GCC warnings (for developers)])],
  [case $enableval in
     yes|no) ;;
     *)      AC_MSG_ERROR([bad value $enableval for gcc-warnings option]) ;;
   esac
   gl_gcc_warnings=$enableval],
  [gl_gcc_warnings=no]
)

if test "$gl_gcc_warnings" = yes; then
  gl_WARN_ADD([-Werror], [WERROR_CFLAGS])
  AC_SUBST([WERROR_CFLAGS])

  nw=
  # This, $nw, is the list of warnings we disable.
  nw="$nw -Wdeclaration-after-statement" # too useful to forbid
  nw="$nw -Waggregate-return"       # anachronistic
  nw="$nw -Wlong-long"              # C90 is anachronistic (lib/gethrxtime.h)
  nw="$nw -Wc++-compat"             # We don't care about C++ compilers
  nw="$nw -Wundef"                  # Warns on '#if GNULIB_FOO' etc in gnulib
  nw="$nw -Wtraditional"            # Warns on #elif which we use often
  nw="$nw -Wcast-qual"              # Too many warnings for now
  nw="$nw -Wconversion"             # Too many warnings for now
  nw="$nw -Wsystem-headers"         # Don't let system headers trigger warnings
  nw="$nw -Wsign-conversion"        # Too many warnings for now
  nw="$nw -Wtraditional-conversion" # Too many warnings for now
  nw="$nw -Wunreachable-code"       # Too many warnings for now
  nw="$nw -Wpadded"                 # Our structs are not padded
  nw="$nw -Wredundant-decls"        # openat.h declares e.g., mkdirat
  nw="$nw -Wlogical-op"             # any use of fwrite provokes this
  nw="$nw -Wformat-nonliteral"      # who.c and pinky.c strftime uses
  nw="$nw -Wvla"                    # warnings in gettext.h
  nw="$nw -Wnested-externs"         # use of XARGMATCH/verify_function__
  nw="$nw -Wswitch-enum"            # Too many warnings for now
  nw="$nw -Wswitch-default"         # Too many warnings for now
  nw="$nw -Wstack-protector"        # not worth working around
  # things I might fix soon:
  nw="$nw -Wfloat-equal"            # sort.c, seq.c
  nw="$nw -Wmissing-format-attribute" # copy.c
  nw="$nw -Wunsafe-loop-optimizations" # a few src/*.c
  nw="$nw -Winline"                 # system.h's readdir_ignoring_dot_and_dotdot
  nw="$nw -Wstrict-overflow"        # expr.c, pr.c, tr.c, factor.c
  # ?? -Wstrict-overflow

  gl_MANYWARN_ALL_GCC([ws])
  gl_MANYWARN_COMPLEMENT([ws], [$ws], [$nw])
  for w in $ws; do
    gl_WARN_ADD([$w])
  done
  gl_WARN_ADD([-Wno-missing-field-initializers]) # We need this one
  gl_WARN_ADD([-Wno-sign-compare])     # Too many warnings for now
  gl_WARN_ADD([-Wno-pointer-sign])     # Too many warnings for now
  gl_WARN_ADD([-Wno-unused-parameter]) # Too many warnings for now

  # In spite of excluding -Wlogical-op above, it is enabled, as of
  # gcc 4.5.0 20090517, and it provokes warnings in cat.c, dd.c, truncate.c
  gl_WARN_ADD([-Wno-logical-op])

  gl_WARN_ADD([-fdiagnostics-show-option])
  gl_WARN_ADD([-funit-at-a-time])

  AC_SUBST([WARN_CFLAGS])

  AC_DEFINE([lint], [1], [Define to 1 if the compiler is checking for lint.])
  AC_DEFINE([_FORTIFY_SOURCE], [2],
    [enable compile-time and run-time bounds-checking, and some warnings])
  AC_DEFINE([GNULIB_PORTCHECK], [1], [enable some gnulib portability checks])

  # We use a slightly smaller set of warning options for lib/.
  # Remove the following and save the result in GNULIB_WARN_CFLAGS.
  nw=
  #nw="$nw -Wuninitialized"
  nw="$nw -Wunused-macros"
  #nw="$nw -Wmissing-prototypes"
  nw="$nw -Wold-style-definition"
  nw="$nw -Wcast-align"
  nw="$nw -Wunsuffixed-float-constants"
  gl_WARN_ADD([-Wno-format-nonliteral])
  gl_MANYWARN_COMPLEMENT([GNULIB_WARN_CFLAGS], [$WARN_CFLAGS], [$nw])
  AC_SUBST([GNULIB_WARN_CFLAGS])
fi

# By default, argmatch should fail calling usage (EXIT_FAILURE).
AC_DEFINE([ARGMATCH_DIE], [usage (EXIT_FAILURE)],
          [Define to the function xargmatch calls on failures.])
AC_DEFINE([ARGMATCH_DIE_DECL], [void usage (int _e)],
          [Define to the declaration of the xargmatch failure function.])

dnl Checks for header files.
AC_HEADER_STDC
AC_HEADER_DIRENT

dnl Checks for functions.
AC_FUNC_CLOSEDIR_VOID

AC_CHECK_FUNCS_ONCE(isascii setlocale)

dnl I18N feature
AM_GNU_GETTEXT_VERSION([0.18])
AM_GNU_GETTEXT([external])

dnl Some installers want to be informed if we do not use our regex.
dnl For example, if the host platform uses dynamic linking and the installer
dnl knows that the grep may be invoked on other hosts with buggy libraries,
dnl then the installer should configure --with-included-regex.
AM_CONDITIONAL([USE_INCLUDED_REGEX], [test "$ac_use_included_regex" = yes])
if test "$ac_use_included_regex" = no; then
  AC_MSG_WARN([Included lib/regex.c not used])
fi

gl_FUNC_PCRE

case $host_os in
  mingw*) suffix=w32 ;;
  *) suffix=posix ;;
esac
AC_CONFIG_LINKS([lib/colorize.c:lib/colorize-$suffix.c])

AC_CONFIG_FILES([
  Makefile
  lib/Makefile
  src/Makefile
  tests/Makefile
  po/Makefile.in
  doc/Makefile
  gnulib-tests/Makefile
])
GREP="$ac_abs_top_builddir/src/grep"
EGREP="$ac_abs_top_builddir/src/grep -E"
AC_OUTPUT
