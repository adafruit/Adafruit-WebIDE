# quotearg.m4 serial 6
dnl Copyright (C) 2002, 2004, 2005, 2006, 2007 Free Software Foundation, Inc.
dnl This file is free software; the Free Software Foundation
dnl gives unlimited permission to copy and/or distribute it,
dnl with or without modifications, as long as this notice is preserved.

AC_DEFUN([gl_QUOTEARG],
[
  AC_LIBOBJ([quotearg])

  dnl Prerequisites of lib/quotearg.c.
  AC_CHECK_FUNCS_ONCE([mbsinit])
  AC_TYPE_MBSTATE_T
  gl_FUNC_MBRTOWC
])
