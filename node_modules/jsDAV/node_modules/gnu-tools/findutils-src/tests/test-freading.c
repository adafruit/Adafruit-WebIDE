/* Test of freading() function.
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

#include "freading.h"

#include <stdio.h>
#include <stdlib.h>

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

#define TESTFILE "t-freading.tmp"

int
main ()
{
  FILE *fp;

  /* Create a file with some contents.  Write-only file is never reading.  */
  fp = fopen (TESTFILE, "w");
  if (fp == NULL)
    goto skip;
  ASSERT (!freading (fp));
  if (fwrite ("foobarsh", 1, 8, fp) < 8)
    goto skip;
  ASSERT (!freading (fp));
  if (fclose (fp))
    goto skip;

  /* Open it in read-only mode.  Read-only file is always reading.  */
  fp = fopen (TESTFILE, "r");
  if (fp == NULL)
    goto skip;
  ASSERT (freading (fp));
  if (fgetc (fp) != 'f')
    goto skip;
  ASSERT (freading (fp));
  if (fseek (fp, 2, SEEK_CUR))
    goto skip;
  ASSERT (freading (fp));
  if (fgetc (fp) != 'b')
    goto skip;
  ASSERT (freading (fp));
  fflush (fp);
  ASSERT (freading (fp));
  if (fgetc (fp) != 'a')
    goto skip;
  ASSERT (freading (fp));
  if (fseek (fp, 0, SEEK_END))
    goto skip;
  ASSERT (freading (fp));
  if (fclose (fp))
    goto skip;

  /* Open it in read-write mode.  POSIX requires a reposition (fseek,
     fsetpos, rewind) or EOF when transitioning from read to write;
     freading is only deterministic after input or output, but this
     test case should be portable even on open, after reposition, and
     at EOF.  */
  /* First a scenario with only fgetc, fseek, fputc.  */
  fp = fopen (TESTFILE, "r+");
  if (fp == NULL)
    goto skip;
  ASSERT (!freading (fp));
  if (fgetc (fp) != 'f')
    goto skip;
  ASSERT (freading (fp));
  if (fseek (fp, 2, SEEK_CUR))
    goto skip;
  /* freading (fp) is undefined here, but fwriting (fp) is false.  */
  if (fgetc (fp) != 'b')
    goto skip;
  ASSERT (freading (fp));
  /* This fseek call is necessary when switching from reading to writing.
     See the description of fopen(), ISO C 99 7.19.5.3.(6).  */
  if (fseek (fp, 0, SEEK_CUR) != 0)
    goto skip;
  /* freading (fp) is undefined here, but fwriting (fp) is false.  */
  if (fputc ('x', fp) != 'x')
    goto skip;
  ASSERT (!freading (fp));
  if (fseek (fp, 0, SEEK_END))
    goto skip;
  /* freading (fp) is undefined here, because on some implementations (e.g.
     glibc) fseek causes a buffer to be read.
     fwriting (fp) is undefined as well.  */
  if (fclose (fp))
    goto skip;

  /* Open it in read-write mode.  POSIX requires a reposition (fseek,
     fsetpos, rewind) or EOF when transitioning from read to write;
     freading is only deterministic after input or output, but this
     test case should be portable even on open, after reposition, and
     at EOF.  */
  /* Here a scenario that includes fflush.  */
  fp = fopen (TESTFILE, "r+");
  if (fp == NULL)
    goto skip;
  ASSERT (!freading (fp));
  if (fgetc (fp) != 'f')
    goto skip;
  ASSERT (freading (fp));
  if (fseek (fp, 2, SEEK_CUR))
    goto skip;
  /* freading (fp) is undefined here, but fwriting (fp) is false.  */
  if (fgetc (fp) != 'b')
    goto skip;
  ASSERT (freading (fp));
  fflush (fp);
  /* freading (fp) is undefined here, but fwriting (fp) is false.  */
  if (fgetc (fp) != 'x')
    goto skip;
  ASSERT (freading (fp));
  /* This fseek call is necessary when switching from reading to writing.
     See the description of fopen(), ISO C 99 7.19.5.3.(6).  */
  if (fseek (fp, 0, SEEK_CUR) != 0)
    goto skip;
  /* freading (fp) is undefined here, but fwriting (fp) is false.  */
  if (fputc ('z', fp) != 'z')
    goto skip;
  ASSERT (!freading (fp));
  if (fseek (fp, 0, SEEK_END))
    goto skip;
  /* freading (fp) is undefined here, because on some implementations (e.g.
     glibc) fseek causes a buffer to be read.
     fwriting (fp) is undefined as well.  */
  if (fclose (fp))
    goto skip;

  /* Open it in append mode.  Write-only file is never reading.  */
  fp = fopen (TESTFILE, "a");
  if (fp == NULL)
    goto skip;
  ASSERT (!freading (fp));
  if (fwrite ("bla", 1, 3, fp) < 3)
    goto skip;
  ASSERT (!freading (fp));
  if (fclose (fp))
    goto skip;

  return 0;

 skip:
  fprintf (stderr, "Skipping test: file operations failed.\n");
  return 77;
}
