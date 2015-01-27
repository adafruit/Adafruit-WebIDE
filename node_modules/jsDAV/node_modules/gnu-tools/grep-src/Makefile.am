# Process this file with automake to create Makefile.in
#
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
#
AUTOMAKE_OPTIONS = gnu 1.10
ACLOCAL_AMFLAGS = -I m4

SUBDIRS = po lib doc src tests gnulib-tests

EXTRA_DIST =		\
  ChangeLog-2009	\
  Makefile.boot		\
  README		\
  README-alpha		\
  README-boot		\
  TODO			\
  cfg.mk

# Shortcut target to make it easier to run expensive tests.
.PHONY: check-expensive
check-expensive:
	$(MAKE) check RUN_EXPENSIVE_TESTS=yes

# Run syntax-check rules before creating a distribution tarball.
.PHONY: run-syntax-check
run-syntax-check:
	$(AM_V_GEN)test ! -d .git || $(MAKE) syntax-check

# Arrange so that .tarball-version appears only in the distribution
# tarball, and never in a checked-out repository.
dist-hook: gen-ChangeLog run-syntax-check
	$(AM_V_GEN)echo $(VERSION) > $(distdir)/.tarball-version

gen_start_date = 2009-11-27
.PHONY: gen-ChangeLog
gen-ChangeLog:
	$(AM_V_GEN)if test -d .git; then				\
	  $(top_srcdir)/build-aux/gitlog-to-changelog			\
	    --since=$(gen_start_date) > $(distdir)/cl-t;		\
	  rm -f $(distdir)/ChangeLog;					\
	  mv $(distdir)/cl-t $(distdir)/ChangeLog;			\
	fi
