#serial 12
# Determine whether we need the lchown wrapper.

dnl Copyright (C) 1998, 2001, 2003, 2004, 2005, 2006, 2007 Free
dnl Software Foundation, Inc.

dnl This file is free software; the Free Software Foundation
dnl gives unlimited permission to copy and/or distribute it,
dnl with or without modifications, as long as this notice is preserved.

dnl From Jim Meyering.
dnl Provide lchown on systems that lack it.

AC_DEFUN([gl_FUNC_LCHOWN],
[
  AC_REQUIRE([gl_UNISTD_H_DEFAULTS])
  AC_REQUIRE([gl_FUNC_CHOWN])
  AC_REPLACE_FUNCS(lchown)
  if test $ac_cv_func_lchown = no; then
    REPLACE_LCHOWN=1
  fi
])
