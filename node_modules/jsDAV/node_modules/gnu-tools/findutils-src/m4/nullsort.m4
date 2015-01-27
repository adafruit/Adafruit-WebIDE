AC_DEFUN([jy_SORTZ],
  [AC_PATH_PROG([SORT], [sort], [sort])
  AC_MSG_CHECKING([if $SORT supports the -z option])
if test "x$cross_compiling" = xyes
then
	# We are cross-compiling, so cannot test the target's "sort".
    	ac_sort_has_z=false
else
    # find out if the sort command has a working -z option.
    if $SORT -z -c < "${srcdir:-.}/m4/order-good.bin" 2>/dev/null  >/dev/null
    then
    	# sort has a -z option, but we have not yet established that 
    	# sort thinks there is more than one input line there.   We have 
    	# to make sort -c do its thing with the input lines in the wrong 
    	# order to determine that (we can't do it in one shot because 
    	# if sort returns nonzero we cant tell that it wasn't just 
    	# complaining about this unknown -z option.
    	if $SORT -z -c < "${srcdir:-.}/m4/order-bad.bin" 2>/dev/null >/dev/null
    	then
    		# sort likes -z but it doesn't seem to make \0 
    		# a delimiter.
    		ac_sort_has_z=false
    	else
    		ac_sort_has_z=true
    	fi
    else
    	# Doesn't like the z option.
    	ac_sort_has_z=false
    fi
fi

if test "x$cross_compiling" = xyes
then
	AC_MSG_RESULT([assume no, since we are cross compiling])
	AC_SUBST(SORT_SUPPORTS_Z,[false])
elif $ac_sort_has_z
then
	AC_MSG_RESULT([yes])
	AC_SUBST(SORT_SUPPORTS_Z,[true])
else
	AC_MSG_RESULT([no])
	AC_SUBST(SORT_SUPPORTS_Z,[false])
fi
])

