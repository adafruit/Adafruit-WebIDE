# Customize maint.mk                           -*- makefile -*-
# Copyright (C) 2009-2012 Free Software Foundation, Inc.

# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.

# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.

# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.

# Used in maint.mk's web-manual rule
manual_title = GNU Grep: Print lines matching a pattern

# Use the direct link.  This is guaranteed to work immediately, while
# it can take a while for the faster mirror links to become usable.
url_dir_list = http://ftp.gnu.org/gnu/$(PACKAGE)

# Tests not to run as part of "make distcheck".
local-checks-to-skip =			\
  sc_texinfo_acronym

# Tools used to bootstrap this package, used for "announcement".
bootstrap-tools = autoconf,automake,gnulib

# Now that we have better tests, make this the default.
export VERBOSE = yes

# Comparing tarball sizes compressed using different xz presets, we see
# that -6e adds only 60 bytes to the size of the tarball, yet reduces
# (from -9) the decompression memory requirement from 64 MiB to 9 MiB.
# Don't be tempted by -5e, since -6 and -5 use the same dictionary size.
# $ for i in {4,5,6,7,8,9}{e,}; do \
#     (n=$(xz -$i < grep-2.11.tar|wc -c);echo $n $i) & done |sort -nr
# 1236632 4
# 1162564 5
# 1140988 4e
# 1139620 6
# 1139480 7
# 1139480 8
# 1139480 9
# 1129552 5e
# 1127616 6e
# 1127556 7e
# 1127556 8e
# 1127556 9e
export XZ_OPT = -6e

old_NEWS_hash = 347e90ee0ec0489707df139ca3539934

# Many m4 macros names once began with `jm_'.
# Make sure that none are inadvertently reintroduced.
sc_prohibit_jm_in_m4:
	@grep -nE 'jm_[A-Z]'						\
		$$($(VC_LIST) m4 |grep '\.m4$$'; echo /dev/null) &&	\
	    { echo '$(ME): do not use jm_ in m4 macro names'		\
	      1>&2; exit 1; } || :

sc_prohibit_echo_minus_en:
	@prohibit='\<echo -[en]'					\
	halt='do not use echo ''-e or echo ''-n; use printf instead'	\
	  $(_sc_search_regexp)

# Indent only with spaces.
sc_prohibit_tab_based_indentation:
	@prohibit='^ *	'						\
	halt='TAB in indentation; use only spaces'			\
	  $(_sc_search_regexp)

# Don't use "indent-tabs-mode: nil" anymore.  No longer needed.
sc_prohibit_emacs__indent_tabs_mode__setting:
	@prohibit='^( *[*#] *)?indent-tabs-mode:'			\
	halt='use of emacs indent-tabs-mode: setting'			\
	  $(_sc_search_regexp)

update-copyright-env = \
  UPDATE_COPYRIGHT_USE_INTERVALS=1 \
  UPDATE_COPYRIGHT_MAX_LINE_LENGTH=79

exclude_file_name_regexp--sc_bindtextdomain = ^tests/get-mb-cur-max\.c$$
exclude_file_name_regexp--sc_prohibit_strcmp = /colorize-.*\.c$$
exclude_file_name_regexp--sc_prohibit_xalloc_without_use = ^src/kwset\.c$$
exclude_file_name_regexp--sc_prohibit_tab_based_indentation = \
  (Makefile|\.(am|mk)$$|^gl/lib/.*\.c\.diff$$)
exclude_file_name_regexp--sc_space_tab = ^gl/lib/.*\.c\.diff$$
exclude_file_name_regexp--sc_error_message_uppercase = ^src/dfa\.c$$
