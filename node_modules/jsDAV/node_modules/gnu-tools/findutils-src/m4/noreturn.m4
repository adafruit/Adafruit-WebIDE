dnl Copyright (C) 2007 Free Software Foundation, Inc.
dnl This file is free software; the Free Software Foundation
dnl gives unlimited permission to copy and/or distribute it,
dnl with or without modifications, as long as this notice is preserved.
dnl
dnl This file can can be used in projects which are not available under
dnl the GNU General Public License or the GNU Library General Public
dnl License but which still want to provide support for the GNU gettext
dnl functionality.
dnl Please note that the actual code of the GNU gettext library is covered
dnl by the GNU Library General Public License, and the rest of the GNU
dnl gettext package package is covered by the GNU General Public License.
dnl They are *not* in the public domain.

dnl Authors:
dnl   James Youngman <jay@gnu.org>, 2007.

AC_DEFUN([jy_AC_ATTRIBUTE_NORETURN],
[AC_CACHE_CHECK([for __attribute__ ((__noreturn__)) support],
	[jy_cv_cc_attribute_noreturn],
	[AC_COMPILE_IFELSE([AC_LANG_PROGRAM([],
		[void report_fatal_error(void) __attribute__ ((__noreturn__));])],
		[jy_cv_cc_attribute_noreturn=yes],
		[jy_cv_cc_attribute_noreturn=no])])
if test $jy_cv_cc_attribute_noreturn = yes; then
  HAVE_ATTRIBUTE_NORETURN=1
else
  HAVE_ATTRIBUTE_NORETURN=0
fi
AC_SUBST([HAVE_ATTRIBUTE_NORETURN])
AC_DEFINE_UNQUOTED([HAVE_ATTRIBUTE_NORETURN],[$HAVE_ATTRIBUTE_NORETURN],
  [Define to 1 if the compiler supports __attribute__ ((__noreturn__))])
])
