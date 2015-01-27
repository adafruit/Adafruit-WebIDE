/* findutils-version.c -- show version information for findutils
   Copyright (C) 2007 Free Software Foundation, Inc.

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
#include "stdio-safer.h"
#include "version-etc.h"
#include "gnulib-version.h"



#if ENABLE_NLS
# include <libintl.h>
# define _(Text) gettext (Text)
#else
# define _(Text) Text
#endif
#ifdef gettext_noop
# define N_(String) gettext_noop (String)
#else
/* See locate.c for explanation as to why not use (String) */
# define N_(String) String
#endif



#ifdef _LIBC
/* In the GNU C library, there is a predefined variable for this.  */
# define program_name program_invocation_name
#endif


extern char *program_name;
char *version_string = VERSION;


void
display_findutils_version(const char *official_name)
{
  /* We use official_name rather than program name in the version 
   * information.  This is deliberate, it is specified by the 
   * GNU coding standard.
   */
  fflush (stderr);
  version_etc(stdout,
	      official_name, PACKAGE_NAME, version_string,
	      _("Eric B. Decker"),
	      _("James Youngman"),
	      _("Kevin Dalley"),
	      (const char*) NULL);
  printf (_("Built using GNU gnulib version %s\n"), gnulib_version);
}
