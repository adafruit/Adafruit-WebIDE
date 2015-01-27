## Process this file with automake to create Makefile.in
# Copyright 1997-1998, 2005-2012 Free Software Foundation, Inc.
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 3, or (at your option)
# any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.

LN = ln

AM_CFLAGS = $(WARN_CFLAGS) $(WERROR_CFLAGS)

# Tell the linker to omit references to unused shared libraries.
AM_LDFLAGS = $(IGNORE_UNUSED_LIBRARIES_CFLAGS)

bin_PROGRAMS = grep egrep fgrep
grep_SOURCES  = grep.c
egrep_SOURCES = egrep.c
fgrep_SOURCES = fgrep.c
noinst_HEADERS = grep.h dfa.h kwset.h search.h system.h mbsupport.h

noinst_LIBRARIES = libgrep.a
libgrep_a_SOURCES = kwset.c dfa.c searchutils.c dfasearch.c kwsearch.c \
	pcresearch.c main.c

# Sometimes, the expansion of $(LIBINTL) includes -lc which may
# include modules defining variables like `optind', so libgreputils.a
# must precede $(LIBINTL) in order to ensure we use GNU getopt.
# But libgreputils.a must also follow $(LIBINTL), since libintl uses
# replacement functions defined in libgreputils.a.
LDADD = \
  libgrep.a \
  ../lib/libgreputils.a $(LIBINTL) ../lib/libgreputils.a $(LIBICONV)

grep_LDADD = $(LDADD) $(LIB_PCRE)
localedir = $(datadir)/locale
AM_CPPFLAGS = -I$(top_builddir)/lib -I$(top_srcdir)/lib

EXTRA_DIST = dosbuf.c
