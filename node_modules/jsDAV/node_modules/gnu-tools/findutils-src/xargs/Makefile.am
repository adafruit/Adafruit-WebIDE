AUTOMAKE_OPTIONS = std-options
localedir = $(datadir)/locale
bin_PROGRAMS = xargs
man_MANS = xargs.1
INCLUDES = -I.. -I../gnulib/lib -I$(top_srcdir)/gnulib/lib -I$(top_srcdir)/lib -I../intl -DLOCALEDIR=\"$(localedir)\"
LDADD = ../lib/libfind.a ../gnulib/lib/libgnulib.a @INTLLIBS@
EXTRA_DIST = $(man_MANS)
SUBDIRS = . testsuite

$(PROGRAMS): ../lib/libfind.a ../gnulib/lib/libgnulib.a

dist-hook: findutils-check-manpages

findutils-check-manpages:
	$(top_srcdir)/build-aux/man-lint.sh $(srcdir) $(man_MANS)
