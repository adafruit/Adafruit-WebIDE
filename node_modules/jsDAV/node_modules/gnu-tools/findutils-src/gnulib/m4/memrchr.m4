# memrchr.m4 serial 8
dnl Copyright (C) 2002, 2003, 2005, 2006, 2007 Free Software Foundation, Inc.
dnl This file is free software; the Free Software Foundation
dnl gives unlimited permission to copy and/or distribute it,
dnl with or without modifications, as long as this notice is preserved.

AC_DEFUN([gl_FUNC_MEMRCHR],
[
  dnl Persuade glibc <string.h> to declare memrchr().
  AC_REQUIRE([AC_USE_SYSTEM_EXTENSIONS])

  AC_REQUIRE([gl_HEADER_STRING_H_DEFAULTS])
  AC_CHECK_DECLS_ONCE([memrchr])
  if test $ac_cv_have_decl_memrchr = no; then
    HAVE_DECL_MEMRCHR=0
  fi

  AC_REPLACE_FUNCS(memrchr)
  if test $ac_cv_func_memrchr = no; then
    gl_PREREQ_MEMRCHR
  fi
])

# Prerequisites of lib/memrchr.c.
AC_DEFUN([gl_PREREQ_MEMRCHR], [:])
