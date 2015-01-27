/* Auxiliary program to detect support for a locale.
   Copyright 2010-2012 Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street - Fifth Floor, Boston, MA
   02110-1301, USA.  */

#include <config.h>
#include <locale.h>
#include <stdio.h>
#include <stdlib.h>

#include "progname.h"

int
main (int argc, char **argv)
{
  set_program_name (argv[0]);
  if (1 < argc && setlocale (LC_ALL, argv[1]))
    {
      printf ("%d\n", (int) MB_CUR_MAX);
      exit (EXIT_SUCCESS);
    }

  exit (EXIT_FAILURE);
}
