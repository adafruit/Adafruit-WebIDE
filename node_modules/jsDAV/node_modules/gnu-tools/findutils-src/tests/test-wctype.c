/* Test of <wctype.h> substitute.
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

#include <wctype.h>

int
main ()
{
  /* Check that the isw* functions exist as functions or as macros.  */
  iswalnum (0);
  iswalpha (0);
#if 0 /* not portable: missing on mingw */
  iswblank (0);
#endif
  iswcntrl (0);
  iswdigit (0);
  iswgraph (0);
  iswlower (0);
  iswprint (0);
  iswpunct (0);
  iswspace (0);
  iswupper (0);
  iswxdigit (0);

  return 0;
}
