# wcwidth.m4 serial 13
dnl Copyright (C) 2006, 2007 Free Software Foundation, Inc.
dnl This file is free software; the Free Software Foundation
dnl gives unlimited permission to copy and/or distribute it,
dnl with or without modifications, as long as this notice is preserved.

AC_DEFUN([gl_FUNC_WCWIDTH],
[
  AC_REQUIRE([gl_WCHAR_H_DEFAULTS])

  dnl Persuade glibc <wchar.h> to declare wcwidth().
  AC_REQUIRE([AC_USE_SYSTEM_EXTENSIONS])

  AC_REQUIRE([gt_TYPE_WCHAR_T])
  AC_REQUIRE([gt_TYPE_WINT_T])

  AC_CHECK_HEADERS_ONCE([wchar.h])
  AC_CHECK_FUNCS_ONCE([wcwidth])

  AC_CHECK_DECLS([wcwidth], [], [], [
/* AIX 3.2.5 declares wcwidth in <string.h>. */
#include <string.h>
/* Tru64 with Desktop Toolkit C has a bug: <stdio.h> must be included before
   <wchar.h>.
   BSD/OS 4.0.1 has a bug: <stddef.h>, <stdio.h> and <time.h> must be included
   before <wchar.h>.  */
#include <stddef.h>
#include <stdio.h>
#include <time.h>
#include <wchar.h>
])
  if test $ac_cv_have_decl_wcwidth != yes; then
    HAVE_DECL_WCWIDTH=0
  fi

  if test $ac_cv_func_wcwidth = no; then
    REPLACE_WCWIDTH=1
  else
    dnl On MacOS X 10.3, wcwidth(0x0301) (COMBINING ACUTE ACCENT) returns 1.
    dnl On OSF/1 5.1, wcwidth(0x200B) (ZERO WIDTH SPACE) returns 1.
    dnl This leads to bugs in 'ls' (coreutils).
    AC_CACHE_CHECK([whether wcwidth works reasonably in UTF-8 locales],
      [gl_cv_func_wcwidth_works],
      [
        AC_TRY_RUN([
#include <locale.h>
/* AIX 3.2.5 declares wcwidth in <string.h>. */
#include <string.h>
/* Tru64 with Desktop Toolkit C has a bug: <stdio.h> must be included before
   <wchar.h>.
   BSD/OS 4.0.1 has a bug: <stddef.h>, <stdio.h> and <time.h> must be included
   before <wchar.h>.  */
#include <stddef.h>
#include <stdio.h>
#include <time.h>
#include <wchar.h>
#if !HAVE_DECL_WCWIDTH
extern
# ifdef __cplusplus
"C"
# endif
int wcwidth (int);
#endif
int main ()
{
  if (setlocale (LC_ALL, "fr_FR.UTF-8") != NULL)
    if (wcwidth (0x0301) > 0 || wcwidth (0x200B) > 0)
      return 1;
  return 0;
}], [gl_cv_func_wcwidth_works=yes], [gl_cv_func_wcwidth_works=no],
          [gl_cv_func_wcwidth_works="guessing no"])
      ])
    case "$gl_cv_func_wcwidth_works" in
      *yes) ;;
      *no) REPLACE_WCWIDTH=1 ;;
    esac
  fi
  if test $REPLACE_WCWIDTH = 1; then
    AC_LIBOBJ([wcwidth])
  fi

  if test $REPLACE_WCWIDTH = 1 || test $HAVE_DECL_WCWIDTH = 0; then
    WCHAR_H=wchar.h
  fi
])
