/* Test of execution of file name canonicalization.
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

const char *program_name = "test-canonicalize";

int
main ()
{
  /* Check that the symbolic link to a file can be resolved.  */
  {
    char *result1 = canonicalize_file_name ("t-can.tmp/huk");
    char *result2 = canonicalize_file_name ("t-can.tmp/tra");
    char *result3 = canonicalize_filename_mode ("t-can.tmp/huk", CAN_EXISTING);
    ASSERT (result1 != NULL);
    ASSERT (result2 != NULL);
    ASSERT (result3 != NULL);
    ASSERT (strcmp (result1, result2) == 0);
    ASSERT (strcmp (result2, result3) == 0);
    ASSERT (strcmp (result1 + strlen (result1) - 14, "/t-can.tmp/tra") == 0);
    free (result1);
    free (result2);
    free (result3);
  }

  /* Check that the symbolic link to a directory can be resolved.  */
  {
    char *result1 = canonicalize_file_name ("t-can.tmp/plo");
    char *result2 = canonicalize_file_name ("t-can.tmp/bef");
    char *result3 = canonicalize_file_name ("t-can.tmp/lum");
    char *result4 = canonicalize_filename_mode ("t-can.tmp/plo", CAN_EXISTING);
    ASSERT (result1 != NULL);
    ASSERT (result2 != NULL);
    ASSERT (result3 != NULL);
    ASSERT (result4 != NULL);
    ASSERT (strcmp (result1, result2) == 0);
    ASSERT (strcmp (result2, result3) == 0);
    ASSERT (strcmp (result3, result4) == 0);
    ASSERT (strcmp (result1 + strlen (result1) - 14, "/t-can.tmp/lum") == 0);
    free (result1);
    free (result2);
    free (result3);
    free (result4);
  }

  /* Check that a symbolic link to a nonexistent file yields NULL.  */
  {
    char *result1 = canonicalize_file_name ("t-can.tmp/ouk");
    char *result2 = canonicalize_filename_mode ("t-can.tmp/ouk", CAN_EXISTING);
    ASSERT (result1 == NULL);
    ASSERT (result2 == NULL);
  }

  /* Check that a loop of symbolic links is detected.  */
  {
    char *result1 = canonicalize_file_name ("ise");
    char *result2 = canonicalize_filename_mode ("ise", CAN_EXISTING);
    ASSERT (result1 == NULL);
    ASSERT (result2 == NULL);
  }

  /* Check that alternate modes can resolve missing basenames.  */
  {
    char *result1 = canonicalize_filename_mode ("t-can.tmp/zzz", CAN_ALL_BUT_LAST);
    char *result2 = canonicalize_filename_mode ("t-can.tmp/zzz", CAN_MISSING);
    ASSERT (result1 != NULL);
    ASSERT (result2 != NULL);
    ASSERT (strcmp (result1, result2) == 0);
    ASSERT (strcmp (result1 + strlen (result1) - 14, "/t-can.tmp/zzz") == 0);
    free (result1);
    free (result2);
  }

  /* Check that alternate modes can resolve broken symlink basenames.  */
  {
    char *result1 = canonicalize_filename_mode ("t-can.tmp/ouk", CAN_ALL_BUT_LAST);
    char *result2 = canonicalize_filename_mode ("t-can.tmp/ouk", CAN_MISSING);
    ASSERT (result1 != NULL);
    ASSERT (result2 != NULL);
    ASSERT (strcmp (result1, result2) == 0);
    ASSERT (strcmp (result1 + strlen (result1) - 14, "/t-can.tmp/wum") == 0);
    free (result1);
    free (result2);
  }

  /* Check that alternate modes can handle missing dirnames.  */
  {
    char *result1 = canonicalize_filename_mode ("t-can.zzz/zzz", CAN_ALL_BUT_LAST);
    char *result2 = canonicalize_filename_mode ("t-can.zzz/zzz", CAN_MISSING);
    ASSERT (result1 == NULL);
    ASSERT (result2 != NULL);
    ASSERT (strcmp (result2 + strlen (result2) - 14, "/t-can.zzz/zzz") == 0);
    free (result2);
  }

  /* Ensure that the following is resolved properly.
     Before 2007-09-27, it would mistakenly report a loop.  */
  {
    char *result1 = canonicalize_filename_mode ("t-can.tmp", CAN_EXISTING);
    char *result2 = canonicalize_filename_mode ("t-can.tmp/p/1", CAN_EXISTING);
    ASSERT (result1 != NULL);
    ASSERT (result2 != NULL);
    ASSERT (strcmp (result2 + strlen (result1), "/d/2") == 0);
    free (result1);
    free (result2);
  }

  return 0;
}
