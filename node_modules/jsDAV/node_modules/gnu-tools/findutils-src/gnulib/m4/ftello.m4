# ftello.m4 serial 3
dnl Copyright (C) 2007 Free Software Foundation, Inc.
dnl This file is free software; the Free Software Foundation
dnl gives unlimited permission to copy and/or distribute it,
dnl with or without modifications, as long as this notice is preserved.

AC_DEFUN([gl_FUNC_FTELLO],
[
  AC_REQUIRE([gl_STDIO_H_DEFAULTS])
  AC_REQUIRE([AC_PROG_CC])
  AC_REQUIRE([gl_STDIN_LARGE_OFFSET])
  AC_CACHE_CHECK([for ftello], [gl_cv_func_ftello],
    [
      AC_TRY_LINK([#include <stdio.h>], [ftello (stdin);],
	[gl_cv_func_ftello=yes], [gl_cv_func_ftello=no])
    ])
  if test $gl_cv_func_ftello = no; then
    HAVE_FTELLO=0
    gl_REPLACE_FTELLO
  elif test $gl_cv_var_stdin_large_offset = no; then
    gl_REPLACE_FTELLO
  fi
])

AC_DEFUN([gl_REPLACE_FTELLO],
[
  AC_LIBOBJ([ftello])
  AC_REQUIRE([gl_STDIO_H_DEFAULTS])
  REPLACE_FTELLO=1
])
