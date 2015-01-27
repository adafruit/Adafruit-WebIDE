# strcasestr.m4 serial 6
dnl Copyright (C) 2005, 2007 Free Software Foundation, Inc.
dnl This file is free software; the Free Software Foundation
dnl gives unlimited permission to copy and/or distribute it,
dnl with or without modifications, as long as this notice is preserved.

AC_DEFUN([gl_FUNC_STRCASESTR],
[
  AC_REQUIRE([gl_HEADER_STRING_H_DEFAULTS])
  AC_REPLACE_FUNCS(strcasestr)
  if test $ac_cv_func_strcasestr = no; then
    HAVE_STRCASESTR=0
    gl_PREREQ_STRCASESTR
  fi
])

# Prerequisites of lib/strcasestr.c.
AC_DEFUN([gl_PREREQ_STRCASESTR], [
  :
])
