#serial 15
# See if we need to use our replacement for Solaris' openat et al functions.

dnl Copyright (C) 2004, 2005, 2006, 2007 Free Software Foundation, Inc.
dnl This file is free software; the Free Software Foundation
dnl gives unlimited permission to copy and/or distribute it,
dnl with or without modifications, as long as this notice is preserved.

# Written by Jim Meyering.

AC_DEFUN([gl_FUNC_OPENAT],
[
  AC_LIBOBJ([openat-proc])
  AC_REQUIRE([gl_USE_SYSTEM_EXTENSIONS])
  AC_CHECK_FUNCS_ONCE([lchmod])
  AC_CHECK_FUNCS_ONCE([fdopendir])
  AC_REPLACE_FUNCS([fchmodat mkdirat openat])
  case $ac_cv_func_openat+$ac_cv_func_lstat_dereferences_slashed_symlink in
  yes+yes) ;;
  yes+*) AC_LIBOBJ([fstatat]);;
  *)
    AC_DEFINE([__OPENAT_PREFIX], [[rpl_]],
      [Define to rpl_ if the openat replacement function should be used.])
    gl_PREREQ_OPENAT;;
  esac
  gl_FUNC_FCHOWNAT
])

# gl_FUNC_FCHOWNAT_DEREF_BUG([ACTION-IF-BUGGY[, ACTION-IF-NOT_BUGGY]])
AC_DEFUN([gl_FUNC_FCHOWNAT_DEREF_BUG],
[
  AC_CACHE_CHECK([whether fchownat works with AT_SYMLINK_NOFOLLOW],
    gl_cv_func_fchownat_nofollow_works,
    [
     gl_dangle=conftest.dangle
     # Remove any remnants of a previous test.
     rm -f $gl_dangle
     # Arrange for deletion of the temporary file this test creates.
     ac_clean_files="$ac_clean_files $gl_dangle"
     ln -s conftest.no-such $gl_dangle
     AC_RUN_IFELSE(
       [AC_LANG_SOURCE(
	  [[
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/types.h>
int
main ()
{
  return (fchownat (AT_FDCWD, "$gl_dangle", -1, getgid (),
		    AT_SYMLINK_NOFOLLOW) != 0
	  && errno == ENOENT);
}
          ]])],
    [gl_cv_func_fchownat_nofollow_works=yes],
    [gl_cv_func_fchownat_nofollow_works=no],
    [gl_cv_func_fchownat_nofollow_works=no],
    )
  ])
  AS_IF([test $gl_cv_func_fchownat_nofollow_works = no], [$1], [$2])
])

# If we have the fchownat function, and it has the bug (in glibc-2.4)
# that it dereferences symlinks even with AT_SYMLINK_NOFOLLOW, then
# use the replacement function.
# Also use the replacement function if fchownat is simply not available.
AC_DEFUN([gl_FUNC_FCHOWNAT],
[
  # Assume we'll use the replacement function.
  # The only case in which we won't is when we have fchownat, and it works.
  use_replacement_fchownat=yes

  AC_CHECK_FUNC([fchownat], [have_fchownat=yes], [have_fchownat=no])
  if test $have_fchownat = yes; then
    gl_FUNC_FCHOWNAT_DEREF_BUG([], [use_replacement_fchownat=no])
  fi

  if test $use_replacement_fchownat = yes; then
    AC_LIBOBJ(fchownat)
    AC_DEFINE(fchownat, rpl_fchownat,
      [Define to rpl_fchownat if the replacement function should be used.])
  fi
])

AC_DEFUN([gl_PREREQ_OPENAT],
[
  :
])
