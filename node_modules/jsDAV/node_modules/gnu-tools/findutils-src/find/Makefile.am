AUTOMAKE_OPTIONS = std-options
localedir = $(datadir)/locale
# noinst_PROGRAMS = regexprops
# regexprops_SOURCES = regexprops.c

noinst_LIBRARIES = libfindtools.a
libfindtools_a_SOURCES = finddata.c fstype.c parser.c pred.c tree.c util.c


# We always build two versions of find, one with fts, one without.
# Their names depend on whether the user specified --with-fts.
#
# --with-fts		find 		extra binary
#	yes		with fts	'oldfind', without fts
#        no             without fts     'ftsfind', with fts
# 
if WITH_FTS
bin_PROGRAMS     = find oldfind
find_SOURCES     = ftsfind.c
oldfind_SOURCES  = find.c
else
bin_PROGRAMS      = find ftsfind
find_SOURCES      = find.c
ftsfind_SOURCES   = ftsfind.c
endif

EXTRA_DIST = defs.h $(man_MANS)
INCLUDES = -I../gnulib/lib -I$(top_srcdir)/lib -I$(top_srcdir)/gnulib/lib -I../intl -DLOCALEDIR=\"$(localedir)\"
LDADD = ./libfindtools.a ../lib/libfind.a ../gnulib/lib/libgnulib.a @INTLLIBS@ @LIB_CLOCK_GETTIME@ @FINDLIBS@
man_MANS = find.1
SUBDIRS = . testsuite

#$(PROGRAMS): ../lib/libfind.a

dist-hook: findutils-check-manpages

findutils-check-manpages:
	$(top_srcdir)/build-aux/man-lint.sh $(srcdir) $(man_MANS)
