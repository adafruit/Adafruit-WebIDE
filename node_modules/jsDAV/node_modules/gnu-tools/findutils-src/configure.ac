dnl Process this file with autoconf to produce a configure script.
AC_INIT([GNU findutils], 4.4.2, [bug-findutils@gnu.org])
AC_CONFIG_AUX_DIR(build-aux)
AM_INIT_AUTOMAKE

AC_CONFIG_SRCDIR([find/pred.c])
AC_CONFIG_HEADERS([config.h])
AC_CANONICAL_HOST
AC_CONFIG_MACRO_DIR(gnulib/m4)

dnl Set of available languages.
ALL_LINGUAS="be bg ca cs da de el eo es et fi fr ga gl hr hu id it ja ko lg lt ms nl pl pt pt_BR ro ru rw sk sl sr sv tr uk vi zh_CN zh_TW"

AC_SUBST(INCLUDES)dnl
AC_SUBST(AUXDIR,$ac_aux_dir)
dnl check for --with-fts
FIND_WITH_FTS

AC_ARG_ENABLE(id-cache,
[  --enable-id-cache       cache all UIDs & GIDs; avoid if using NIS or Hesiod],
  AC_DEFINE(CACHE_IDS, 1, [Define if you want find -nouser and -nogroup to make tables of
   used UIDs and GIDs at startup instead of using getpwuid or
   getgrgid when needed.  Speeds up -nouser and -nogroup unless you
   are running NIS or Hesiod, which make password and group calls
   very expensive.]))

AC_ARG_ENABLE(debug,
	AS_HELP_STRING(--enable-debug,Enable debugging output which is likely to be interesting to people debugging findutils),
	[ac_cv_debug=$enableval],[ac_cv_debug=no])

AC_ARG_ENABLE(leaf-optimisation,
	AS_HELP_STRING(--enable-leaf-optimisation,Enable an optimisation which saves lstat calls to identify subdirectories on filesystems having traditional Unix semantics),
	[ac_cv_leaf_optimisation=$enableval],[ac_cv_leaf_optimisation=yes])

AC_ARG_ENABLE(d_type-optimization,
	AS_HELP_STRING(--enable-d_type-optimization,Make use of the file type data returned in struct dirent.d_type by readdir()),
	[ac_cv_d_type=$enableval],[ac_cv_d_type=yes])

dnl This one has no default, because otherwise we would have to say
dnl both --enable-d_type-optimisation=no and --enable-d_type-optimization=no
dnl to turn it off.
AC_ARG_ENABLE(d_type-optimisation,
	AS_HELP_STRING(--enable-d_type-optimisation,Synonym for --enable-d_type-optimization),
	[ac_cv_d_type=$enableval],[])


AC_MSG_CHECKING([whether debug output should be produced])
if test x$ac_cv_debug = xno; then
   AC_MSG_RESULT([no])
else
   AC_MSG_RESULT([yes])
   AC_DEFINE(DEBUG, 1, [Define if you want to see find's innards])
   AC_DEFINE(DEBUG_STAT, 1, [Define if you want to see a message every time find calls the stat() system call])
fi

AC_MSG_CHECKING([for leaf optimisation])
if test x$ac_cv_leaf_optimisation = xno; then
   AC_MSG_RESULT([no])
else
   AC_MSG_RESULT([yes])
   AC_DEFINE(LEAF_OPTIMISATION, 1, [Define if you want to use the leaf optimisation (this can still be turned off with -noleaf)])
fi

AC_ARG_VAR([DEFAULT_ARG_SIZE], [Default size of arguments to child processes
of find and xargs, 128k if unspecified])
if test -n "$DEFAULT_ARG_SIZE"; then
   AC_DEFINE_UNQUOTED([DEFAULT_ARG_SIZE], [$DEFAULT_ARG_SIZE],
     [If defined, the default argument size used in child processes])
fi



dnl Checks for programs.
AC_PROG_CC
AC_PROG_CPP

dnl for gnulib
gl_EARLY

AM_C_PROTOTYPES

AC_PROG_INSTALL
AC_PROG_RANLIB
dnl AC_PROG_LIBTOOL
AC_PROG_MAKE_SET
AC_SYS_LARGEFILE


gl_INIT



dnl Older versions of gnulib/m4/nls.m4 provide AM_MKINSTALLDIRS.
dnl The current version of gnulib does not, but the version of
dnl po/Makefile.in.in that comes with gettext-0.14.6 expects
dnl that @MKINSTALLDIRS@ will be expanded.
AM_MKINSTALLDIRS

dnl lib/regexprops needs to be a native program, because we need to
dnl run it in order to generate the documentation about the properties
dnl of regular expressions.  See lib/Makefile.am.
AM_CONDITIONAL(CROSS_COMPILING, [[test "x$cross_compiling" = xyes]])

dnl Try to get a POSIX.1 environment.

dnl Checks for libraries.  If getpwnam() isn't present in the
dnl C library, try -lsun.
AC_CHECK_FUNC(getpwnam, [],
[AC_CHECK_LIB(sun, getpwnam)])

AC_CHECK_LIB([m],[modf],[FINDLIBS="-lm $FINDLIBS"]
  AC_DEFINE_UNQUOTED(HAVE_MODF_IN_LIBM,1,[modf is defined in -lm]))
AC_CHECK_LIB([m],[fabs],[FINDLIBS="-lm $FINDLIBS"]
  AC_DEFINE_UNQUOTED(HAVE_FABS_IN_LIBM,1,[fabs is defined in -lm]))
AC_SUBST([FINDLIBS])

dnl Checks for header files.
AC_HEADER_STDC
dnl Assume unistd.h is present - coreutils does too.
AC_CHECK_HEADERS(fcntl.h string.h limits.h errno.h stdlib.h stddef.h)
AC_CHECK_HEADERS(unistd.h sys/types.h inttypes.h fcntl.h locale.h stdint.h)
AC_CHECK_HEADERS(sys/param.h mntent.h sys/mnttab.h sys/mntio.h sys/mkdev.h)
dnl find.c needs sys/utsname.h because it calls uname(2).
AC_CHECK_HEADERS(sys/utsname.h)
AC_HEADER_MAJOR
AC_HEADER_DIRENT
AC_HEADER_STAT
AC_HEADER_SYS_WAIT


dnl Checks for typedefs, structures, and compiler characteristics.

AC_TYPE_UID_T
AC_TYPE_SIZE_T
AC_CHECK_TYPE(ssize_t, int)
AC_TYPE_PID_T
AC_CHECK_TYPE(ino_t, unsigned long)
AC_CHECK_TYPE(dev_t, unsigned long)
AC_TYPE_MODE_T
AC_STRUCT_ST_BLOCKS
AC_CHECK_MEMBERS([struct stat.st_rdev])

AC_MSG_CHECKING([whether we should use struct dirent.d_type, if available])
if test x$ac_cv_d_type = xno; then
   AC_MSG_RESULT([no])
else
   AC_MSG_RESULT([yes])
   AC_DEFINE(USE_STRUCT_DIRENT_D_TYPE, 1,
   [Defined if you specify --enable-d_type-optimisation to configure.])
   AC_CHECK_MEMBERS([struct dirent.d_type],[
   AC_DEFINE(HAVE_STRUCT_DIRENT_D_TYPE, 1,
   [Define to 1 if your 'struct dirent' has a d_type member])],
   [],[
#include <sys/types.h>
#include <dirent.h>])
fi


AC_STRUCT_TM
AC_STRUCT_TIMEZONE

dnl Checks for library functions that are provided by gnulib.
AC_FUNC_STRFTIME
AC_REPLACE_FUNCS(memcmp memset stpcpy strdup strstr strtol strtoul)
AC_CHECK_FUNCS(fchdir getcwd strerror endgrent endpwent setlocale)
AC_FUNC_VPRINTF
AC_FUNC_ALLOCA
AC_FUNC_CLOSEDIR_VOID

dnl We don't just use AC_FUNC_GETMNTENT directly because it
dnl will try to use -lsun on platforms which have getmntent() in the
dnl C library, for example UNICOS.
AC_CHECK_FUNC(getmntent, [], [AC_FUNC_GETMNTENT])
AC_CHECK_FUNCS(getmntent)
AC_CHECK_FUNCS(setmntent endmntent)


dnl Checks for library functions that are provided by findlib.
FINDLIB_REPLACE_FUNCS(waitpid strspn)
FINDLIB_REPLACE_FUNCS(forcefindlib)


# Check for common but not-POSIX functions.
AC_CHECK_FUNCS(setgroups)


dnl gl_XALLOC
gl_FUNC_ALLOCA

dnl Use gl_INCLUDED_REGEX so that findutils will build on systems like
dnl Solaris, which lacks those functions in libc (see GNU Savannah bug
dnl #11710) (Sun Sep  4 20:15:11 2005: gl_INCLUDED_REGEX no longer seems
dnl to be available in gnulib CVS)
gl_REGEX

AC_PREREQ(2.59)

jy_SORTZ


AC_CHECK_MEMBER(struct dirent.d_type,,,[
#include <sys/types.h>
#include <dirent.h>])

jy_AC_ATTRIBUTE_NORETURN

dnl internationalization macros
AM_GNU_GETTEXT([external])
AM_GNU_GETTEXT_VERSION(0.14.5)

dnl regextype.c and regexprops.c are designed to be usable outside findutils,
dnl but findutils doesn't want to support all the regex types in gnulib,
dnl and wants to support an additional one (RE_SYNTAX_EMACS|RE_DOT_NEWLINE).
dnl Hence they need to know if they are being compiled into findutils or not.
AC_DEFINE([FINDUTILS], 1, [Define if we are compiling GNU findutils])
AC_DEFINE([ALREADY_INCLUDED_CONFIG_H], 1, [Define so that source code can verify that config.h was already included])

# This is necessary so that .o files in LIBOBJS are also built via
# the ANSI2KNR-filtering rules.
#LIBOBJS=`echo $LIBOBJS|sed 's/\.o /\$U.o /g;s/\.o$/\$U.o/'`
##AC_CONFIG_SUBDIRS(gnulib)
AC_CONFIG_FILES(gnulib/Makefile gnulib/lib/Makefile)
AC_CONFIG_FILES(m4/Makefile)
AC_CONFIG_FILES([Makefile
         tests/Makefile build-aux/Makefile
	find/Makefile find/testsuite/Makefile
	xargs/Makefile xargs/testsuite/Makefile
	locate/Makefile locate/testsuite/Makefile
	 po/Makefile.in po/Makefile
	doc/Makefile lib/Makefile
	])
AC_OUTPUT
dnl intl/Makefile
