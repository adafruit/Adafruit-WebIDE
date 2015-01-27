AUTOMAKE_OPTIONS=gnits
# readme-alpha

EXTRA_DIST = COPYING ChangeLog TODO config.h.in stamp-h.in \
		THANKS import-gnulib.sh import-gnulib.config \
		tool-versions.txt README-hacking
DISTCLEANFILES = tool-versions.txt


# "tests" is the gnulib unit test dir.
SUBDIRS = gnulib tests build-aux lib find xargs locate doc po m4 

ACLOCAL_AMFLAGS = -I gnulib/m4 -I m4

TESTFILE_SUFFIXES = .exp .xo .xe .xi

# CONFIG_CLEAN_FILES = gnulib/lib/regex.c

tool-versions.txt: Makefile
	( automake --version ; echo ; \
	autoconf --version ; echo ; \
	$(CC) --version ; echo ; \
	m4 --version ; echo ; \
	gettext --version ; echo ; \
	runtest --version ; echo ; \
	makeinfo --version ) > $@


dist-hook: jy-regex-fix findutils-check-pofiles findutils-check-testfiles


## regex.c seems to get left out if I use automake-1.9 but not 
## if I use automake-1.7.   Hence dist-hook has to be able to 
## copy regex.c into the relevant (read-only) directory if it 
## is not already there, but needs to avoid doing so if the 
## file is already in place.   Ugh.  
## 
## This problem appears not to apply to Automake-1.10, but since
## gnulib only requires Automake-1.9.6, we should support that.  
## Therefore we probably can't remove this kludge yet.
## 
##                             -- James Youngman <jay@gnu.org>
## 
jy-regex-fix:
	if test -f $(distdir)/gnulib/lib/regex.c ; then \
		echo regex.c is already in place.  Great. ; \
	else \
		echo Making $(distdir)/gnulib/lib writable ... ; \
		chmod +w $(distdir)/gnulib/lib ; \
		echo Copying $(srcdir)/gnulib/lib/regex.c to $(distdir)/gnulib/lib ; \
		cp $(srcdir)/gnulib/lib/regex.c $(distdir)/gnulib/lib/regex.c ; \
	fi
#	ls -ld $(srcdir)/gnulib/lib/regex.c $(distdir)/gnulib/lib

## Check that we actually shipped all the .po files.  If this rule fails, 
## check ALL_LINGUAS in configure.in against the po files in the source
## directory (their names, not their contents)
findutils-check-pofiles:
	@echo ; echo Checking to see if we distributed the full set of .po files
	distcount=`ls $(distdir)/po/*.po | wc -l` ; srccount=`ls $(srcdir)/po/*.po | wc -l` ; test $$distcount -eq $$srccount || ( echo FAILED: Please check the value of ALL_LINGUAS in configure.in against the actual set of ".po" files >&2 ; false )
	@echo All .po files distributed OK.

## Check that we actually shipped all the test files that exist in the source.
## runtest will run all the .exp files it finds, and so if we don't ship all 
## of them, there will be some tests which people using the CVS code will be
## running, but people using the source distribution will not.
findutils-check-testfiles:
	@echo
	$(AUXDIR)/check-testfiles.sh "$(distdir)" "$(srcdir)" $(TESTFILE_SUFFIXES)


findutils-check-smells:
	find  $(srcdir) \( -path $(srcdir)/autom4te.cache -o  \
                           -path $(srcdir)/gnulib-git     -o  \
                           -name .git             -o  \
                           \( -type d -name CVS \)  \
                       \) -prune -o                  \
	\( -type f -o -type l \)  \
	\! \( -name '*~' -o -name '*.xo' -o -name '*.xi' \) \
	-print0 | \
	xargs -0 python $(AUXDIR)/src-sniff.py
