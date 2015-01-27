# pcre.m4 - check for libpcre support
# serial 1

# Copyright (C) 2010-2012 Free Software Foundation, Inc.
# This file is free software; the Free Software Foundation
# gives unlimited permission to copy and/or distribute it,
# with or without modifications, as long as this notice is preserved.

AC_DEFUN([gl_FUNC_PCRE],
[
  AC_ARG_ENABLE([perl-regexp],
    AC_HELP_STRING([--disable-perl-regexp],
                   [disable perl-regexp (pcre) support]),
    [case $enableval in
       yes|no) test_pcre=$enableval;;
       *) AC_MSG_ERROR([invalid value $enableval for --disable-perl-regexp]);;
     esac],
    [test_pcre=yes])

  LIB_PCRE=
  AC_SUBST([LIB_PCRE])
  use_pcre=no

  if test x"$test_pcre" = x"yes"; then
    AC_CHECK_HEADERS([pcre.h])
    AC_CHECK_HEADERS([pcre/pcre.h])
    if test $ac_cv_header_pcre_h = yes \
        || test $ac_cv_header_pcre_pcre_h = yes; then
      pcre_saved_LIBS=$LIBS
      AC_SEARCH_LIBS([pcre_compile], [pcre],
        [test "$ac_cv_search_pcre_compile" = "none required" ||
         LIB_PCRE=$ac_cv_search_pcre_compile])
      AC_CHECK_FUNCS([pcre_compile])
      LIBS=$pcre_saved_LIBS
      if test $ac_cv_func_pcre_compile = yes; then
        use_pcre=yes
      fi
    fi
    if test $use_pcre = no; then
      AC_MSG_WARN([libpcre development library was not found or not usable.])
      AC_MSG_WARN([AC_PACKAGE_NAME will be built without pcre support.])
    fi
  fi

  AC_DEFINE_UNQUOTED([HAVE_LIBPCRE], [`test $use_pcre != yes; echo $?`],
    [Define to 1 if you have the `pcre' library (-lpcre).])
])
