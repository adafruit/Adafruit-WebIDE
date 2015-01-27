/* Test of yesno module.
   Copyright (C) 2007 Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <config.h>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "closein.h"
#include "yesno.h"

char *program_name;

/* Test yesno.  Without arguments, read one line.  If first argument
   is zero, close stdin before attempting to read one line.
   Otherwise, read the number of lines specified by first
   argument.  */
int
main (int argc, char **argv)
{
  int i = 1;
  program_name = argv[0];
  /* yesno recommends that all clients use close_stdin in main.  */
  atexit (close_stdin);

  if (1 < argc)
    i = atoi (argv[1]);
  if (!i)
    {
      i = 1;
      close (0);
    }
  while (i--)
    puts (yesno () ? "Y" : "N");
  return 0;
}
