AC_DEFUN([FIND_WITH_FTS],
[AC_ARG_WITH([fts],
[  --without-fts           Use an older mechanism for searching the filesystem, instead of using fts()],[with_fts=$withval],[])
  case $with_fts in 
	yes|no) ;;
	'')     with_fts=yes ;;
  	*) AC_MSG_ERROR([Invalid value for --with-fts: $with_fts])
  esac
  AM_CONDITIONAL(WITH_FTS, [[test x"${with_fts-no}" != xno]])
  if test x"${with_fts-no}" != xno ; then
  	AC_DEFINE(WITH_FTS, 1, [Define if you want to use fts() to do the filesystem search.])
  fi
])
