info_TEXINFOS = find.texi find-maint.texi
find_TEXINFOS = perm.texi getdate.texi regexprops.texi fdl.texi
find_maint_TEXINFOS = fdl.texi
MOSTLYCLEANFILES = find.cps 
CLEANFILES = find.txt find_mono.html findutils.texi_html_node.tar.gz

# To build regexprops.texi, we need to build the regexprops program and 
# run it on the host.  If we are cross compiling, we may not have a native
# binary.  When this is the case, we use a workaround; don't delete the 
# file for 'make clean'.  This fixes Savannah bug #19658.
if !CROSS_COMPILING
CLEANFILES += regexprops.texi regexprops-generic.texi
endif

MAKEINFOTXT = $(MAKEINFO) --plaintext

find.txt: find.texi $(srcdir)/version.texi $(find_TEXINFOS)

# find.txt is a file which we need to know how to build 
# because it gets put on the www.gnu.org website.
# This rule is derived from the .texi.html rule.
.texi.txt:
	rm -rf $(@:.txt=.tmp)
	if $(MAKEINFOTXT) $(AM_MAKEINFOTXTFLAGS) $(MAKEINFOFLAGS) -I $(srcdir) \
	 -o $(@:.txt=.tmp) $<; \
	then \
	  rm -rf $@; \
	  if test ! -d $(@:.txt=.tmp) && test -d $(@:.txt=); then \
	    mv $(@:.txt=) $@; else mv $(@:.txt=.tmp) $@; fi; \
	else \
	  if test ! -d $(@:.txt=.tmp) && test -d $(@:.txt=); then \
	    rm -rf $(@:.txt=); else rm -Rf $(@:.txt=.tmp) $@; fi; \
	  exit 1; \
	fi


# find_mono.html is a file which we need to know how to build 
# because it gets put on the www.gnu.org website.
# This rule is derived from the generic .texi.html rule.
find_mono.html: find.texi
	rm -rf $(@:.html=.htp)
	if $(MAKEINFOHTML) --no-split $(AM_MAKEINFOHTMLFLAGS) $(MAKEINFOFLAGS) -I $(srcdir) \
	 -o $(@:.html=.htp) $<; \
	then \
	  rm -rf $@; \
	  if test ! -d $(@:.html=.htp) && test -d $(@:.html=); then \
	    mv $(@:.html=) $@; else mv $(@:.html=.htp) $@; fi; \
	else \
	  if test ! -d $(@:.html=.htp) && test -d $(@:.html=); then \
	    rm -rf $(@:.html=); else rm -Rf $(@:.html=.htp) $@; fi; \
	  exit 1; \
	fi


# findutils.texi_html_node.tar.gz is a file which we need to know
# how to build because it gets put on the www.gnu.org website.
# This rule depends on GNU tar, but it's principally used 
# by the maintainer, and we don't need to build the file 
# for "make all" or "make install" (or even "make check").
findutils.texi_html_node.tar.gz: find.html
	tar zcf $@ $<

if CROSS_COMPILING
regexprops.texi regexprops-generic.texi: ../gnulib/lib/regex.h
	echo "WARNING: $? is newer than $@ but $@ cannot be rebuilt because we are cross-compiling.   Continuing anyway." >&2
else
regexprops.texi: ../gnulib/lib/regex.h ../lib/regexprops.c
	cd ../lib && $(MAKE) $(AM_MAKEFLAGS) regexprops$(EXEEXT)
	../lib/regexprops$(EXEEXT) "Regular Expressions" findutils > $@
	rm ../lib/regexprops$(EXEEXT)
regexprops-generic.texi: ../gnulib/lib/regex.h ../lib/regexprops.c
	cd ../lib && $(MAKE) $(AM_MAKEFLAGS) regexprops$(EXEEXT)
	../lib/regexprops$(EXEEXT) "Regular Expressions" generic > $@
	rm ../lib/regexprops$(EXEEXT)
endif
