# The default database to build and search.
AUTOMAKE_OPTIONS = std-options
LOCATE_DB = $(localstatedir)/locatedb
localedir = $(datadir)/locale

AM_INSTALLCHECK_STD_OPTIONS_EXEMPT = \
	frcode$(EXEEXT) \
	code$(EXEEXT) \
	bigram$(EXEEXT)
bin_PROGRAMS = locate
libexec_PROGRAMS = frcode code bigram
bin_SCRIPTS = updatedb
man_MANS = locate.1 updatedb.1 locatedb.5
BUILT_SOURCES = dblocation.texi
EXTRA_DIST = locatedb.h updatedb.sh $(man_MANS)
CLEANFILES = updatedb dblocation.texi
locate_SOURCES = locate.c word_io.c
code_SOURCES = code.c word_io.c

INCLUDES = -I$(top_srcdir)/lib -I../gnulib/lib -I$(top_srcdir)/gnulib/lib -I../intl -DLOCATE_DB=\"$(LOCATE_DB)\" -DLOCALEDIR=\"$(localedir)\"

LDADD = ../lib/libfind.a ../gnulib/lib/libgnulib.a @INTLLIBS@

$(PROGRAMS) $(LIBPROGRAMS): ../lib/libfind.a ../gnulib/lib/libgnulib.a

# We generate updatedb from the Makefile rather than with AC_OUTPUT in
# configure.ac in order to allow the user better control of what goes
# into it by setting Makefile variables.
updatedb: updatedb.sh Makefile
	rm -f $@
	find=`echo find|sed '$(transform)'`; \
	frcode=`echo frcode|sed '$(transform)'`; \
	bigram=`echo bigram|sed '$(transform)'`; \
	code=`echo code|sed '$(transform)'`; \
	sed \
	-e "s,@""bindir""@,$(bindir)," \
	-e "s,@""libexecdir""@,$(libexecdir)," \
	-e "s,@""LOCATE_DB""@,$(LOCATE_DB)," \
	-e "s,@""VERSION""@,@VERSION@," \
	-e "s,@""PACKAGE_NAME""@,@PACKAGE_NAME@," \
	-e "s,@""find""@,$${find}," \
	-e "s,@""frcode""@,$${frcode}," \
	-e "s,@""bigram""@,$${bigram}," \
	-e "s,@""code""@,$${code}," \
	-e "s,@""SORT""@,$(SORT)," \
	-e "s,@""SORT_SUPPORTS_Z""@,$(SORT_SUPPORTS_Z)," \
	$(srcdir)/updatedb.sh > $@
	chmod +x $@

install-data-hook:
	$(top_srcdir)/build-aux/mkinstalldirs $(DESTDIR)$(localstatedir)

dblocation.texi: Makefile
	echo '@set LOCATE_DB $(LOCATE_DB)' > $@

SUBDIRS = . testsuite

dist-hook: findutils-check-manpages

findutils-check-manpages:
	$(top_srcdir)/build-aux/man-lint.sh $(srcdir) $(man_MANS)
