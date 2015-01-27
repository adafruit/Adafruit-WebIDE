# fchdir.m4 serial 4
dnl Copyright (C) 2006-2007 Free Software Foundation, Inc.
dnl This file is free software; the Free Software Foundation
dnl gives unlimited permission to copy and/or distribute it,
dnl with or without modifications, as long as this notice is preserved.

AC_DEFUN([gl_FUNC_FCHDIR],
[
  AC_REQUIRE([gl_UNISTD_H_DEFAULTS])
  AC_CHECK_FUNCS_ONCE([fchdir])
  if test $ac_cv_func_fchdir = no; then
    REPLACE_FCHDIR=1
    AC_LIBOBJ([fchdir])
    gl_PREREQ_FCHDIR
    AC_DEFINE([FCHDIR_REPLACEMENT], 1,
      [Define if gnulib's fchdir() replacement is used.])
    gl_CHECK_NEXT_HEADERS([dirent.h])
    DIRENT_H='dirent.h'
  else
    DIRENT_H=
  fi
  AC_SUBST([DIRENT_H])
])

# Prerequisites of lib/fchdir.c.
AC_DEFUN([gl_PREREQ_FCHDIR], [:])
