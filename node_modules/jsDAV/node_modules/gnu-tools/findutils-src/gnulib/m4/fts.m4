#serial 13
dnl Copyright (C) 2005-2007 Free Software Foundation, Inc.
dnl This file is free software; the Free Software Foundation
dnl gives unlimited permission to copy and/or distribute it,
dnl with or without modifications, as long as this notice is preserved.

AC_DEFUN([gl_FUNC_FTS],
[
  gl_FUNC_FTS_CORE
])

AC_DEFUN([gl_FUNC_FTS_LGPL],
[
  gl_FUNC_FTS_CORE
])

AC_DEFUN([gl_FUNC_FTS_CORE],
[
  dnl Use this version of fts unconditionally, since the GNU libc and
  dnl NetBSD versions have bugs and/or unnecessary limitations.
  AC_LIBOBJ([fts])

  dnl Prerequisites of lib/fts.c.
  gl_FUNC_OPENAT

  # Checks for header files.
  AC_CHECK_HEADERS_ONCE([sys/param.h])dnl
])
