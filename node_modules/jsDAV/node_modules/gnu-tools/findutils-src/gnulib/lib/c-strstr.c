/* c-strstr.c -- substring search in C locale
   Copyright (C) 2005-2007 Free Software Foundation, Inc.
   Written by Bruno Haible <bruno@clisp.org>, 2005, 2007.

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

#include <config.h>

/* Specification.  */
#include "c-strstr.h"

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "malloca.h"

/* Knuth-Morris-Pratt algorithm.
   See http://en.wikipedia.org/wiki/Knuth-Morris-Pratt_algorithm
   Return a boolean indicating success.  */
static bool
knuth_morris_pratt (const char *haystack, const char *needle,
		    const char **resultp)
{
  size_t m = strlen (needle);

  /* Allocate the table.  */
  size_t *table = (size_t *) malloca (m * sizeof (size_t));
  if (table == NULL)
    return false;
  /* Fill the table.
     For 0 < i < m:
       0 < table[i] <= i is defined such that
       rhaystack[0..i-1] == needle[0..i-1] and rhaystack[i] != needle[i]
       implies
       forall 0 <= x < table[i]: rhaystack[x..x+m-1] != needle[0..m-1],
       and table[i] is as large as possible with this property.
     table[0] remains uninitialized.  */
  {
    size_t i, j;

    table[1] = 1;
    j = 0;
    for (i = 2; i < m; i++)
      {
	unsigned char b = (unsigned char) needle[i - 1];

	for (;;)
	  {
	    if (b == (unsigned char) needle[j])
	      {
		table[i] = i - ++j;
		break;
	      }
	    if (j == 0)
	      {
		table[i] = i;
		break;
	      }
	    j = j - table[j];
	  }
      }
  }

  /* Search, using the table to accelerate the processing.  */
  {
    size_t j;
    const char *rhaystack;
    const char *phaystack;

    *resultp = NULL;
    j = 0;
    rhaystack = haystack;
    phaystack = haystack;
    /* Invariant: phaystack = rhaystack + j.  */
    while (*phaystack != '\0')
      if ((unsigned char) needle[j] == (unsigned char) *phaystack)
	{
	  j++;
	  phaystack++;
	  if (j == m)
	    {
	      /* The entire needle has been found.  */
	      *resultp = rhaystack;
	      break;
	    }
	}
      else if (j > 0)
	{
	  /* Found a match of needle[0..j-1], mismatch at needle[j].  */
	  rhaystack += table[j];
	  j -= table[j];
	}
      else
	{
	  /* Found a mismatch at needle[0] already.  */
	  rhaystack++;
	  phaystack++;
	}
  }

  freea (table);
  return true;
}

/* Find the first occurrence of NEEDLE in HAYSTACK.  */
char *
c_strstr (const char *haystack, const char *needle)
{
  /* Be careful not to look at the entire extent of haystack or needle
     until needed.  This is useful because of these two cases:
       - haystack may be very long, and a match of needle found early,
       - needle may be very long, and not even a short initial segment of
         needle may be found in haystack.  */
  if (*needle != '\0')
    {
      /* Minimizing the worst-case complexity:
	 Let n = strlen(haystack), m = strlen(needle).
	 The naïve algorithm is O(n*m) worst-case.
	 The Knuth-Morris-Pratt algorithm is O(n) worst-case but it needs a
	 memory allocation.
	 To achieve linear complexity and yet amortize the cost of the memory
	 allocation, we activate the Knuth-Morris-Pratt algorithm only once
	 the naïve algorithm has already run for some time; more precisely,
	 when
	   - the outer loop count is >= 10,
	   - the average number of comparisons per outer loop is >= 5,
	   - the total number of comparisons is >= m.
	 But we try it only once.  If the memory allocation attempt failed,
	 we don't retry it.  */
      bool try_kmp = true;
      size_t outer_loop_count = 0;
      size_t comparison_count = 0;
      size_t last_ccount = 0;			/* last comparison count */
      const char *needle_last_ccount = needle;	/* = needle + last_ccount */

      /* Speed up the following searches of needle by caching its first
	 character.  */
      unsigned char b = (unsigned char) *needle;

      needle++;
      for (;; haystack++)
	{
	  if (*haystack == '\0')
	    /* No match.  */
	    return NULL;

	  /* See whether it's advisable to use an asymptotically faster
	     algorithm.  */
	  if (try_kmp
	      && outer_loop_count >= 10
	      && comparison_count >= 5 * outer_loop_count)
	    {
	      /* See if needle + comparison_count now reaches the end of
		 needle.  */
	      if (needle_last_ccount != NULL)
		{
		  needle_last_ccount +=
		    strnlen (needle_last_ccount, comparison_count - last_ccount);
		  if (*needle_last_ccount == '\0')
		    needle_last_ccount = NULL;
		  last_ccount = comparison_count;
		}
	      if (needle_last_ccount == NULL)
		{
		  /* Try the Knuth-Morris-Pratt algorithm.  */
		  const char *result;
		  bool success =
		    knuth_morris_pratt (haystack, needle - 1, &result);
		  if (success)
		    return (char *) result;
		  try_kmp = false;
		}
	    }

	  outer_loop_count++;
	  comparison_count++;
	  if ((unsigned char) *haystack == b)
	    /* The first character matches.  */
	    {
	      const char *rhaystack = haystack + 1;
	      const char *rneedle = needle;

	      for (;; rhaystack++, rneedle++)
		{
		  if (*rneedle == '\0')
		    /* Found a match.  */
		    return (char *) haystack;
		  if (*rhaystack == '\0')
		    /* No match.  */
		    return NULL;
		  comparison_count++;
		  if ((unsigned char) *rhaystack != (unsigned char) *rneedle)
		    /* Nothing in this round.  */
		    break;
		}
	    }
	}
    }
  else
    return (char *) haystack;
}
