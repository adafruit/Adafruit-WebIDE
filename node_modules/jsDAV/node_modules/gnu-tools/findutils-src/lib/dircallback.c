/* listfile.c -- run a function in a specific directory
   Copyright (C) 2007, 2008, 2009 Free Software Foundation, Inc.

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

/* This file was written by James Youngman, based on gnulib'c at-func.c. 
 */


#include <config.h>


#include "openat.h"
#include <stdarg.h>
#include <stddef.h>
#include <errno.h>

#include "fcntl--.h"
#include "lstat.h"
#include "save-cwd.h"


#ifdef HAVE_LOCALE_H
#include <locale.h>
#endif

#if ENABLE_NLS
# include <libintl.h>
# define _(Text) gettext (Text)
#else
# define _(Text) Text
#define textdomain(Domain)
#define bindtextdomain(Package, Directory)
#endif
#ifdef gettext_noop
# define N_(String) gettext_noop (String)
#else
/* See locate.c for explanation as to why not use (String) */
# define N_(String) String
#endif



int
run_in_dir (int dir_fd, int (*callback)(void*), void *usercontext)
{
  if (dir_fd == AT_FDCWD)
    {
      return (*callback)(usercontext);
    }
  else
    {
      struct saved_cwd saved_cwd;
      int saved_errno;
      int err;
      
      if (save_cwd (&saved_cwd) != 0)
	openat_save_fail (errno);
      
      if (fchdir (dir_fd) != 0)
	{
	  saved_errno = errno;
	  free_cwd (&saved_cwd);
	  errno = saved_errno;
      return -1;
	}
      
      err = (*callback)(usercontext);
      saved_errno = (err < 0 ? errno : 0);
      
      if (restore_cwd (&saved_cwd) != 0)
	openat_restore_fail (errno);
      
      free_cwd (&saved_cwd);
      
      if (saved_errno)
	errno = saved_errno;
      return err;
    }
}
