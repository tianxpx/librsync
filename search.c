/* -*- mode: c; c-file-style: "gnu" -*-  */

/* hash -- manage hashtables of checksums and blocks

   Copyright (C) 1999-2000 by Martin Pool.
   Copyright (C) 1999 by Andrew Tridgell

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License as
   published by the Free Software Foundation; either version 2 of the
   License, or (at your option) any later version.
   
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
   
   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA */

/* TODO: The common case is that the next block in both streams match.
   Can we make that a bit faster at all? */

#include "includes.h"
#include "hsync.h"
#include "private.h"

#define TABLESIZE (1<<16)
#define NULL_TAG (-1)


#define gettag2(s1,s2) (((s1) + (s2)) & 0xFFFF)
#define gettag(sum) gettag2((sum)&0xFFFF,(sum)>>16)

static int
compare_targets (struct target *t1, struct target *t2)
{
  return ((int) t1->t - (int) t2->t);
}


int
_hs_build_hash_table (struct sum_struct *sums)
{
  int i;

  sums->tag_table = calloc (TABLESIZE, sizeof (int));
  sums->targets = calloc (sums->count, sizeof (struct target));

  for (i = 0; i < sums->count; i++)
    {
      sums->targets[i].i = i;
      sums->targets[i].t = gettag (sums->sums[i].sum1);
    }

  qsort (sums->targets, sums->count,
	 sizeof (sums->targets[0]), (int (*)()) compare_targets);

  for (i = 0; i < TABLESIZE; i++)
    sums->tag_table[i] = NULL_TAG;

  for (i = sums->count - 1; i >= 0; i--)
    {
      sums->tag_table[sums->targets[i].t] = i;
    }

  return 0;
}



/* Return the positive token number for a match of the current block,
   or -1 if there is none.

   If we don't find a match on the weak checksum, then we just give
   up.  If we do find a weak match, then we proceed to calculate the
   strong checksum for the current block, and see if it will match
   anything. */
int
_hs_find_in_hash (rollsum_t *rollsum,
		  char const *inbuf, int block_len,
		  struct sum_struct const *sums)
{
  int tag = gettag (rollsum->weak_sum);
  int j = sums->tag_table[tag];
  char strong_sum[SUM_LENGTH];

  if (j == NULL_TAG)
    {
      return -1;
    }

  _hs_calc_strong_sum(inbuf, block_len, strong_sum);

  for (; j < sums->count && sums->targets[j].t == tag; j++)
    {
      int i = sums->targets[j].i;

      if (rollsum->weak_sum != sums->sums[i].sum1)
	continue;

      /* also make sure the two blocks are the same length */
/*      l = MIN(s->n,len-offset); */
/*      if (l != s->sums[i].len) continue;			 */

/*      if (!done_csum2) { */
/*        map = (schar *)map_ptr(buf,offset,l); */
/*        get_checksum2((char *)map,l,sum2); */
/*        done_csum2 = 1; */
/*      } */

      if (memcmp (strong_sum, sums->sums[i].sum2, SUM_LENGTH) == 0)
	{
	  return sums->sums[i].i;
	}
    }

  return -1;
}