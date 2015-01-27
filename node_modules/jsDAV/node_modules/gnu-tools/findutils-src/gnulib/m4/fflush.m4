# fflush.m4 serial 4

# Copyright (C) 2007 Free Software Foundation, Inc.
# This file is free software; the Free Software Foundation
# gives unlimited permission to copy and/or distribute it,
# with or without modifications, as long as this notice is preserved.

dnl From Eric Blake

dnl Find out how to obey POSIX semantics of fflush(stdin) discarding
dnl unread input on seekable streams, rather than C99 undefined semantics.

AC_DEFUN([gl_FUNC_FFLUSH],
[
  AC_CACHE_CHECK([whether fflush works on input streams],
    [gl_cv_func_fflush_stdin],
    [echo hello world > conftest.txt
     AC_RUN_IFELSE([AC_LANG_PROGRAM(
       [[
#include <stdio.h>
#include <unistd.h>
       ]], [[FILE *f = fopen ("conftest.txt", "r");
	 char buffer[10];
	 int fd = fileno (f);
	 if (!f || 0 > fd || fread (buffer, 1, 5, f) != 5)
	   return 2;
	 /* For deterministic results, ensure f read a bigger buffer.  */
	 if (lseek (fd, 0, SEEK_CUR) == 5)
	   return 3;
	 /* POSIX requires fflush-fseek to set file offset of fd.  */
	 if (fflush (f) != 0 || fseek (f, 0, SEEK_CUR) != 0)
	   return 4;
	 return !(lseek (fd, 0, SEEK_CUR) == 5);
       ]])], [gl_cv_func_fflush_stdin=yes], [gl_cv_func_fflush_stdin=no],
     [dnl Pessimistically assume fflush is broken.  This is wrong for
      dnl at least glibc and cygwin; but lib/fflush.c takes this into account.
      gl_cv_func_fflush_stdin=no])
     rm conftest.txt
    ])
  if test $gl_cv_func_fflush_stdin = no; then
    gl_REPLACE_FFLUSH
  fi
])

AC_DEFUN([gl_REPLACE_FFLUSH],
[
  AC_LIBOBJ([fflush])
  AC_LIBOBJ([fseeko])
  AC_REQUIRE([gl_STDIO_H_DEFAULTS])
  REPLACE_FFLUSH=1
  REPLACE_FSEEKO=1
])
