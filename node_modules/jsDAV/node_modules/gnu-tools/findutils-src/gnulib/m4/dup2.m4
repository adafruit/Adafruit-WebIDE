#serial 5
dnl Copyright (C) 2002, 2005, 2007 Free Software Foundation, Inc.
dnl This file is free software; the Free Software Foundation
dnl gives unlimited permission to copy and/or distribute it,
dnl with or without modifications, as long as this notice is preserved.

AC_DEFUN([gl_FUNC_DUP2],
[
  AC_REQUIRE([gl_UNISTD_H_DEFAULTS])
  AC_CHECK_FUNCS_ONCE([dup2])
  if test $ac_cv_func_dup2 = no; then
    HAVE_DUP2=0
    AC_LIBOBJ([dup2])
  fi
])
