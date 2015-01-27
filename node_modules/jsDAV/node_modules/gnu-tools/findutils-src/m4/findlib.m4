# FINDLIB_REPLACE_FUNCS(FUNCTION...)
# -----------------------------
AC_DEFUN([FINDLIB_REPLACE_FUNCS],
[AC_FOREACH([AC_Func], [$1], [jy_FINDLIBSOURCE(AC_Func.c)])dnl
AC_CHECK_FUNCS([$1], , [_jy_FINDLIBOBJ($ac_func)])
])



# jy_FINDLIBSOURCE(FILENAME)
# ----------------------
# Announce we might need the file `FILENAME'.
m4_define([jy_FINDLIBSOURCE], [])



# jy_FINDLIBOBJ(FILENAME-NOEXT, ACTION-IF-INDIR)
# -------------------------------------------
# We need `FILENAME-NOEXT.o', save this into `FINDLIBOBJS'.
# We don't use AC_SUBST/2 because it forces an unnecessary eol.
m4_define([_jy_FINDLIBOBJ],
[AS_LITERAL_IF([$1],
               [jy_FINDLIBSOURCE([$1.c])],
               [$2])dnl
AC_SUBST([FINDLIB@&t@OBJS])dnl
case $FINDLIB@&t@OBJS in
    "$1.$ac_objext"   | \
  *" $1.$ac_objext"   | \
    "$1.$ac_objext "* | \
  *" $1.$ac_objext "* ) ;;
  *) FINDLIB@&t@OBJS="$FINDLIB@&t@OBJS $1.$ac_objext" ;;
esac
])



# jy_FINDLIBOBJ(FILENAME-NOEXT)
# -------------------------
# We need `FILENAME-NOEXT.o', save this into `FINDLIBOBJS'.
# We don't use AC_SUBST/2 because it forces an unnecessary eol.
m4_define([jy_FINDLIBOBJ],
[_jy_FINDLIBOBJ([$1],
            [AC_DIAGNOSE(syntax,
                         [$0($1): you should use literals])])dnl
])


# _jy_FINDLIBOBJS_NORMALIZE 
# ---------------------
# Adapted from autoconf's general.m4.
# Clean up FINDLIBOBJS abd LTFINDLIBOBJS 
# Used with AC_CONFIG_COMMANDS_PRE.
AC_DEFUN([_jy_FINDLIBOBJS_NORMALIZE],
[ac_findlibobjs=
ac_ltfindlibobjs=
for ac_i in : $FINDLIB@&t@OBJS; do test "x$ac_i" = x: && continue
  # 1. Remove the extension, and $U if already installed.
  ac_i=`echo "$ac_i" |
         sed 's/\$U\././;s/\.o$//;s/\.obj$//'`
  # 2. Add them.
  ac_findlibobjs="$ac_libobjs $ac_i\$U.$ac_objext"
  ac_ltfindlibobjs="$ac_ltlibobjs $ac_i"'$U.lo'
done
AC_SUBST([FINDLIB@&t@OBJS], [$ac_findlibobjs])
AC_SUBST([LTFINDLIBOBJS], [$ac_ltfindlibobjs])
])


dnl AC_CONFIG_COMMANDS_PRE(_jy_FINDLIBOBJS_NORMALIZE)
