/* word_io.c -- word oriented I/O
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

#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <stdbool.h>		/* for bool */
#include <assert.h>
#include <stdlib.h>

#include "error.h"
#include "quote.h"
#include "quotearg.h"
#include "locatedb.h"

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
/* We used to use (String) instead of just String, but apparently ISO C
 * doesn't allow this (at least, that's what HP said when someone reported
 * this as a compiler bug).  This is HP case number 1205608192.  See
 * also http://gcc.gnu.org/bugzilla/show_bug.cgi?id=11250 (which references 
 * ANSI 3.5.7p14-15).  The Intel icc compiler also rejects constructs
 * like: static const char buf[] = ("string");
 */
# define N_(String) String
#endif


/* Swap bytes in 32 bit value.  This code is taken from glibc-2.3.3. */
#define bswap_32(x) \
     ((((x) & 0xff000000) >> 24) | (((x) & 0x00ff0000) >>  8) | \
      (((x) & 0x0000ff00) <<  8) | (((x) & 0x000000ff) << 24))

enum { WORDBYTES=4 };

static int 
decode_value(const unsigned char data[], 
	     int limit,
	     GetwordEndianState *endian_state_flag,
	     const char *filename)
{
  int swapped;
  union 
  {
    int ival;			/* native representation */
    unsigned char data[WORDBYTES];
  } u;
  u.ival = 0;
  memcpy(&u.data, data, WORDBYTES);
  swapped = bswap_32(u.ival);	/* byteswapped */
  
  if (*endian_state_flag == GetwordEndianStateInitial)
    {
      if (u.ival <= limit)
	{
	  if (swapped > limit)
	    {
	      /* the native value is inside the limit and the 
	       * swapped value is not.  We take this as proof 
	       * that we should be using the ative byte order. 
	       */
	      *endian_state_flag = GetwordEndianStateNative;
	    }
	  return u.ival;
	}
      else 
	{
	  if (swapped <= limit)
	    {
	      /* Aha, now we know we have to byte-swap. */
	      error(0, 0,
		    _("Warning: locate database %s was "
		      "built with a different byte order"),
		    quotearg_n_style(0, locale_quoting_style, filename));
	      *endian_state_flag = GetwordEndianStateSwab;
	      return swapped;
	    }
	  else
	    {
	      /* u.ival > limit and swapped > limit.  For the moment, assume 
	       * native ordering.
	       */
	      return u.ival;
	    }
	}
    }
  else 
    {
      /* We already know the byte order. */
      if (*endian_state_flag == GetwordEndianStateSwab)
	return swapped;
      else
	return u.ival;
    }
}



int
getword (FILE *fp,
	 const char *filename,
	 size_t minvalue,
	 size_t maxvalue,
	 GetwordEndianState *endian_state_flag)
{
  unsigned char data[4];
  size_t bytes_read;

  clearerr(fp);
  bytes_read = fread(data, WORDBYTES, 1, fp);
  if (bytes_read != 1)
    {
      const char * quoted_name = quotearg_n_style(0, locale_quoting_style,
						  filename);
      /* Distinguish between a truncated database and an I/O error.
       * Either condition is fatal.
       */
      if (feof(fp))
	error(1, 0, _("unexpected EOF in %s"), quoted_name);
      else
	error(1, errno, _("error reading a word from %s"), quoted_name);
      abort ();
    }
  else
    {
      return decode_value(data, maxvalue, endian_state_flag, filename);
    }
}


bool
putword (FILE *fp, int word,
	 GetwordEndianState endian_state_flag)
{
  size_t items_written;

  /* You must decide before calling this function which 
   * endianness you want to use. 
   */
  assert (endian_state_flag != GetwordEndianStateInitial);
  if (GetwordEndianStateSwab == endian_state_flag)
    {
      word = bswap_32(word);
    }
  
  items_written = fwrite(&word, sizeof(word), 1, fp);
  if (1 == items_written)
    return true;
  else
    return false;
}
