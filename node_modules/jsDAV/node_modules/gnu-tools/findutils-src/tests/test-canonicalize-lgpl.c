/* Test of execution of program termination handlers.
   Copyright (C) 2007 Free Software Foundation, Inc.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

/* Written by Bruno Haible <bruno@clisp.org>, 2007.  */

#include <config.h>

#include "canonicalize.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define ASSERT(expr) \
  do									     \
    {									     \
      if (!(expr))							     \
        {								     \
          fprintf (stderr, "%s:%d: assertion failed\n", __FILE__, __LINE__); \
          abort ();							     \
        }								     \
    }									     \
  while (0)

int
main ()
{
#ifdef GNULIB_CANONICALIZE
  /* No need to test canonicalize-lgpl module if canonicalize is also
     in use.  */
  return 0;
#endif

  /* Check that the symbolic link to a file can be resolved.  */
  {
    char *result1 = canonicalize_file_name ("t-can-lgpl.tmp/huk");
    char *result2 = canonicalize_file_name ("t-can-lgpl.tmp/tra");
    ASSERT (result1 != NULL);
    ASSERT (result2 != NULL);
    ASSERT (strcmp (result1, result2) == 0);
    ASSERT (strcmp (result1 + strlen (result1) - 19, "/t-can-lgpl.tmp/tra") == 0);
    free (result1);
    free (result2);
  }

  /* Check that the symbolic link to a directory can be resolved.  */
  {
    char *result1 = canonicalize_file_name ("t-can-lgpl.tmp/plo");
    char *result2 = canonicalize_file_name ("t-can-lgpl.tmp/bef");
    char *result3 = canonicalize_file_name ("t-can-lgpl.tmp/lum");
    ASSERT (result1 != NULL);
    ASSERT (result2 != NULL);
    ASSERT (result3 != NULL);
    ASSERT (strcmp (result1, result2) == 0);
    ASSERT (strcmp (result2, result3) == 0);
    ASSERT (strcmp (result1 + strlen (result1) - 19, "/t-can-lgpl.tmp/lum") == 0);
    free (result1);
    free (result2);
    free (result3);
  }

  /* Check that a symbolic link to a nonexistent file yields NULL.  */
  {
    char *result = canonicalize_file_name ("t-can-lgpl.tmp/ouk");
    ASSERT (result == NULL);
  }

  /* Check that a loop of symbolic links is detected.  */
  {
    char *result = canonicalize_file_name ("ise");
    ASSERT (result == NULL);
  }

  return 0;
}
