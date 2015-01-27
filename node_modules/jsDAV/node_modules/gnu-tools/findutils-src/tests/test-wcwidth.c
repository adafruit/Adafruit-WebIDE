/* Test of wcwidth() function.
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

#include <wchar.h>

#include <locale.h>
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

int
main ()
{
  wchar_t wc;

  /* Test width of ASCII characters.  */
  for (wc = 0x20; wc < 0x7F; wc++)
    ASSERT (wcwidth (wc) == 1);

  /* Switch to an UTF-8 locale.  */
  if (setlocale (LC_ALL, "fr_FR.UTF-8") != NULL)
    {
      /* Test width of ASCII characters.  */
      for (wc = 0x20; wc < 0x7F; wc++)
	ASSERT (wcwidth (wc) == 1);

      /* Test width of some non-spacing characters.  */
      ASSERT (wcwidth (0x0301) == 0);
      ASSERT (wcwidth (0x05B0) == 0);

      /* Test width of some format control characters.  */
      ASSERT (wcwidth (0x200E) <= 0);
      ASSERT (wcwidth (0x2060) == 0);
#if 0  /* wchar_t may be only 16 bits.  */
      ASSERT (wcwidth (0xE0001) <= 0);
      ASSERT (wcwidth (0xE0044) <= 0);
#endif

      /* Test width of some zero width characters.  */
      ASSERT (wcwidth (0x200B) == 0);
      ASSERT (wcwidth (0xFEFF) <= 0);

      /* Test width of some CJK characters.  */
      ASSERT (wcwidth (0x3000) == 2);
      ASSERT (wcwidth (0xB250) == 2);
      ASSERT (wcwidth (0xFF1A) == 2);
#if 0  /* wchar_t may be only 16 bits.  */
      ASSERT (wcwidth (0x20369) == 2);
      ASSERT (wcwidth (0x2F876) == 2);
#endif
    }

  return 0;
}
