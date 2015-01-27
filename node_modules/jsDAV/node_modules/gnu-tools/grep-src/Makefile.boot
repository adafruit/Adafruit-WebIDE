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

# Define for DOS/WIN (not including DJGPP):
#OBJEXT = obj
#EXEEXT = .exe
EXEEXT =
OBJEXT = o

# Source of grep.
grep_OBJS = \
      grep.$(OBJEXT) \
      search.$(OBJEXT) \
      kwset.$(OBJEXT) \
      dfa.$(OBJEXT)
egrep_OBJS = \
      egrep.$(OBJEXT) \
      esearch.$(OBJEXT) \
      kwset.$(OBJEXT) \
      dfa.$(OBJEXT)
fgrep_OBJS = \
      fgrep.$(OBJEXT) \
      fsearch.$(OBJEXT) \
      kwset.$(OBJEXT)

# Supporting routines.
LIB_OBJS_core =  \
      $(libdir)/closeout.$(OBJEXT) \
      $(libdir)/error.$(OBJEXT) \
      $(libdir)/exclude.$(OBJEXT) \
      $(libdir)/hard-locale.$(OBJEXT) \
      $(libdir)/quotearg.$(OBJEXT) \
      $(libdir)/regex.$(OBJEXT) \
      $(libdir)/strtoumax.$(OBJEXT) \
      $(libdir)/xmalloc.$(OBJEXT) \
      $(libdir)/xstrtol.$(OBJEXT) \
      $(libdir)/xstrtoumax.$(OBJEXT)

# Comment out functions already supported as needed.
#LIB_OBJ_atexit   =  $(libdir)/atexit.$(OBJEXT)
#LIB_OBJ_alloca   =  $(libdir)/alloca.$(OBJEXT)
#LIB_OBJ_fnmatch  =  $(libdir)/fnmatch.$(OBJEXT)
LIB_OBJ_getopt   =  $(libdir)/getopt.$(OBJEXT) $(libdir)/getopt1.$(OBJEXT)
#LIB_OBJ_memchr   =  $(libdir)/memchr.$(OBJEXT)
LIB_OBJ_obstack  =  $(libdir)/obstack.$(OBJEXT)
#LIB_OBJ_strtoul  =  $(libdir)/strtoul.$(OBJEXT)

LIB_OBJS = $(LIB_OBJS_core) $(LIB_OBJ_atexit) $(LIB_OBJ_alloca) \
           $(LIB_OBJ_fnmatch) $(LIB_OBJ_getopt) $(LIB_OBJ_memchr) \
           $(LIB_OBJ_obstack) $(LIB_OBJ_strtoul)

# For Linux
#LIB_OBJS = $(LIB_OBJS_core)

# For QNX/Neutrino
#LIB_OBJS = $(LIB_OBJS_core) $(LIB_OBJ_getopt) $(LIB_OBJ_obstack)

# Where is DIR and opendir/readdir defined.
#  or -DHAVE_DIRENT_H
#  or -DHAVE_SYS_NDIR_H
#  or -DHAVE_SYS_DIR_H
#  or -DHAVE_NDIR_H
#
# undef HAVE_STRERROR if lacking strerror()
# undef HAVE_MEMCHR if lacking memchr()
#

# default dry run
DEFS_core = \
           -DSTDC_HEADERS  \
           -DHAVE_MEMCHR \
           -DHAVE_DIRENT_H \
           -DHAVE_STRERROR \
           -Dconst= \
           -Duintmax_t=long

# SunOS-4.1.x k&r cc
#DEFS_sunos =  -DSTDC_HEADERS -DHAVE_MEMCHR -DHAVE_DIRENT_H -Dconst=

# Solaris
#DEFS_solaris = -DSTDC_HEADERS -DHAVE_MEMCHR -DHAVE_DIRENT_H -DHAVE_STRERROR

# DOS/WIN (change also OBJEXT/EXEEXT, see above)
# DOS/DJGPP
DEFS_dos = -DSTDC_HEADERS -DHAVE_MEMCHR -DHAVE_STRERROR -DHAVE_DIRENT_H \
           -DHAVE_DOS_FILE_CONTENTS \
           -DHAVE_DOS_FILE_NAMES -DHAVE_UNISTD_H -DHAVE_SETMODE

# If support ANSI C prototypes
DEFS_ansi_c = -DPROTOTYPES

# No wchar support
# DEFS_wchar = -DUSE_WIDE_CHAR -DHAVE_WCHAR_H
# DEFS_wchar =  -Dwchar_t=int -Dmbstate_t=int
DEFS_wchar =  -DHAVE_WCHAR_H

# Are strtol() and strtoul() declared?
#DEFS_strtol = -DHAVE_DECL_STRTOULL=0 -DHAVE_DECL_STRTOUL=0
DEFS_strtol = -DHAVE_DECL_STRTOULL=1 -DHAVE_DECL_STRTOUL=1

# Define if malloc(0)/realloc(0) works
#DEFS_alloc = -DHAVE_DONE_WORKING_MALLOC_CHECK=0 \
#             -DHAVE_DONE_WORKING_REALLOC_CHECK=0
DEFS_alloc = -DHAVE_DONE_WORKING_MALLOC_CHECK=1 \
             -DHAVE_DONE_WORKING_REALLOC_CHECK=1

DEFS = $(DEFS_core) $(DEFS_ansi_c) $(DEFS_wchar) $(DEFS_strtol) $(DEFS_alloc) \
       -DHAVE_DECL_STRERROR_R=1 -DHAVE_VPRINTF -DCHAR_BIT=8 \
       -DSTDOUT_FILENO=1


####

CFLAGS = $(DEFS) -I. -I.. -I$(libdir) \
	 -DVERSION=\"bootstrap\" -DPACKAGE=\"grep\" \
	 -DPACKAGE_STRING=\"grep\ bootstrap\" \
	 -DPACKAGE_BUGREPORT=\"bug-grep@gnu.org\"

libdir = ../lib

PROGS = grep$(EXEEXT) egrep$(EXEEXT) fgrep$(EXEEXT)

libgreputils_a = $(libdir)/libgreputils.a

all : $(libgreputils_a) $(PROGS)

grep$(EXEEXT)  :  $(grep_OBJS)          $(libgreputils_a)
	$(CC)     $(grep_OBJS) -o  grep $(libgreputils_a)

egrep$(EXEEXT) : $(egrep_OBJS)          $(libgreputils_a)
	$(CC)    $(egrep_OBJS) -o egrep $(libgreputils_a)

fgrep$(EXEEXT) : $(fgrep_OBJS)          $(libgreputils_a)
	$(CC)    $(fgrep_OBJS) -o fgrep $(libgreputils_a)

$(libgreputils_a) : $(LIB_OBJS)
	$(AR) $(ARFLAGS) $(libgreputils_a) $(LIB_OBJS)

clean :
	$(RM)   grep.$(OBJEXT)   egrep.$(OBJEXT)   fgrep.$(OBJEXT)
	$(RM) search.$(OBJEXT) esearch.$(OBJEXT) fsearch.$(OBJEXT)
	$(RM) kwset.$(OBJEXT) dfa.$(OBJEXT)
	$(RM) $(PROGS)
	$(RM) $(libgreputils_a) $(LIB_OBJS)
