/* finddata.c -- global data for "find".
   Copyright (C) 1990, 91, 92, 93, 94, 2000, 
                 2003, 2004, 2005, 2007 Free Software Foundation, Inc.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.
   
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
   
   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <config.h>

#include "defs.h"


/* Name this program was run with. */
char *program_name;

struct options options;
struct state state;

/* The full path of the initial working directory, or "." if
   STARTING_DESC is nonnegative.  */
char const *starting_dir = ".";

/* A file descriptor open to the initial working directory.
   Doing it this way allows us to work when the i.w.d. has
   unreadable parents.  */
int starting_desc;

