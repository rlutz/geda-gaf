/*
   libstroke - a stroke interface library
   Copyright (c) 1996,1997,1998,1999,2000  Mark F. Willey, ETLA Technical

   Permission is hereby granted, free of charge, to any person obtaining
   a copy of this software and associated documentation files (the
   "Software"), to deal in the Software, including the rights to use, copy,
   modify, merge, publish, and distribute copies of the Software, and to
   permit persons to whom the Software is furnished to do so, subject to
   the following conditions:

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; version 2.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

   The above copyright notice and this permission notice shall be included
   in all copies or substantial portions of the Software.

   See the file "LICENSE" for a copy of the GNU GPL terms.

   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
   OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
   MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
   IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY CLAIM, DAMAGES OR
   OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
   ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
   OTHER DEALINGS IN THE SOFTWARE.

   Except as contained in this notice, the name of the author shall
   name be used in advertising or otherwise to promote the sale, use or
   other dealings in this Software without prior written authorization
   from the author.

   Non-GPL commercial use licenses are available - contact copyright holder.

   Author: Mark F. Willey  --  willey@etla.net
   http://www.etla.net/
*/

#include "config.h"
#include "stroke.h"

#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>

#ifndef TRUE
#define TRUE 1
#define FALSE 0
#endif


/* structure for holding point data */
typedef struct s_point *p_point;

struct s_point {
  int x;
  int y;
  p_point next;
};

/* point list head and tail */
static p_point point_list_head;
static p_point point_list_tail;



/* determine which bin a point falls in */
static int stroke_bin (p_point point_p, int bound_x_1, int bound_x_2,
                       int bound_y_1, int bound_y_2);



/* point list head and tail */
static p_point point_list_head=NULL;
static p_point point_list_tail=NULL;

/* metrics fo input stroke */
static int min_x = 10000;
static int min_y = 10000;
static int max_x = -1;
static int max_y = -1;
static int point_count = 0;

static void init_stroke_data (void)
{
  while (point_list_head != NULL) {
    point_list_tail = point_list_head;
    point_list_head = point_list_head->next;
    free (point_list_tail);
  }
  point_list_tail = NULL;
}

void stroke_init (void)
{
  init_stroke_data ();
}



int stroke_trans (char *sequence)
{
  /* number of bins recorded in the stroke */
  int sequence_count = 0;

  /* points-->sequence translation scratch variables */
  int prev_bin = 0;
  int current_bin = 0;
  int bin_count = 0;

  /* flag indicating the start of a stroke - always count it in the sequence */
  int first_bin = TRUE;

  /* bin boundary and size variables */
  int delta_x, delta_y;
  int bound_x_1, bound_x_2;
  int bound_y_1, bound_y_2;

  /* determine size of grid */
  delta_x = max_x - min_x;
  delta_y = max_y - min_y;

  /* calculate bin boundary positions */
  bound_x_1 = min_x + (delta_x / 3);
  bound_x_2 = min_x + 2 * (delta_x / 3);

  bound_y_1 = min_y + (delta_y / 3);
  bound_y_2 = min_y + 2 * (delta_y / 3);

  if (delta_x > STROKE_SCALE_RATIO * delta_y) {
      bound_y_1 = (max_y + min_y - delta_x) / 2 + (delta_x / 3);
      bound_y_2 = (max_y + min_y - delta_x) / 2 + 2 * (delta_x / 3);
  } else if (delta_y > STROKE_SCALE_RATIO * delta_x) {
      bound_x_1 = (max_x + min_x - delta_y) / 2 + (delta_y / 3);
      bound_x_2 = (max_x + min_x - delta_y) / 2 + 2 * (delta_y / 3);
  }

  /*
    printf ("DEBUG:: point count: %d\n",point_count);
    printf ("DEBUG:: min_x: %d\n",min_x);
    printf ("DEBUG:: max_x: %d\n",max_x);
    printf ("DEBUG:: min_y: %d\n",min_y);
    printf ("DEBUG:: max_y: %d\n",max_y);
    printf ("DEBUG:: delta_x: %d\n",delta_x);
    printf ("DEBUG:: delta_y: %d\n",delta_y);
    printf ("DEBUG:: bound_x_1: %d\n",bound_x_1);
    printf ("DEBUG:: bound_x_2: %d\n",bound_x_2);
    printf ("DEBUG:: bound_y_1: %d\n",bound_y_1);
    printf ("DEBUG:: bound_y_2: %d\n",bound_y_2);
  */

  /*
    build string by placing points in bins, collapsing bins and
    discarding those with too few points...
  */

  while (point_list_head != NULL) {

    /* figure out which bin the point falls in */
    current_bin = stroke_bin(point_list_head,bound_x_1, bound_x_2,
                             bound_y_1, bound_y_2);
    /* if this is the first point, consider it the previous bin,
       too. */
    prev_bin = (prev_bin == 0) ? current_bin : prev_bin;
    /*    printf ("DEBUG:: current bin: %d\n",current_bin); */

    if (prev_bin == current_bin)
      bin_count++;
    else {  /* we are moving to a new bin -- consider adding to the
               sequence */
      if ((bin_count > (point_count * STROKE_BIN_COUNT_PERCENT))
          || (first_bin == TRUE)) {
        first_bin = FALSE;
        sequence[sequence_count++] = '0' + prev_bin;
        /*        printf ("DEBUG:: adding sequence: %d\n",prev_bin);
         */
      }

      /* restart counting points in the new bin */
      bin_count=0;
      prev_bin = current_bin;
    }

    /* move to next point, freeing current point from list */
    point_list_tail = point_list_head;
    point_list_head = point_list_head->next;
    free (point_list_tail);
  }
  point_list_tail = NULL;

    /* add the last run of points to the sequence */
    sequence[sequence_count++] = '0' + current_bin;
    /*    printf ("DEBUG:: adding final sequence: %d\n",current_bin); */

  /* bail out on error cases */
  if ((point_count < STROKE_MIN_POINTS)
      || (sequence_count > STROKE_MAX_SEQUENCE)) {
    point_count = 0;
    strcpy (sequence,"0");
    return FALSE;
  }

  /* add null termination and leave */
  point_count = 0;
  sequence[sequence_count] = '\0';
  return TRUE;
}

/* figure out which bin the point falls in */
static int stroke_bin (p_point point_p, int bound_x_1, int bound_x_2,
                int bound_y_1, int bound_y_2)
{
  int bin_num = 1;
  if (point_p->x > bound_x_1) bin_num += 1;
  if (point_p->x > bound_x_2) bin_num += 1;
  if (point_p->y > bound_y_1) bin_num += 3;
  if (point_p->y > bound_y_2) bin_num += 3;

  return bin_num;
}

void stroke_record (int x, int y)
{
  p_point new_point_p;
  int delx, dely;
  float ix, iy;

  if (point_count < STROKE_MAX_POINTS) {
    new_point_p = (p_point) malloc (sizeof(struct s_point));

    if (point_list_tail == NULL) {

      /* first point in list - initialize list and metrics */
      point_list_head = point_list_tail = new_point_p;
      min_x = 10000;
      min_y = 10000;
      max_x = -1;
      max_y = -1;
      point_count = 0;

    } else {
      /* interpolate between last and current point */
      delx = x - point_list_tail->x;
      dely = y - point_list_tail->y;

      /* step by the greatest delta direction */
      if (abs(delx) > abs(dely)) {
        iy = point_list_tail->y;

     /* go from the last point to the current, whatever direction it
        may be */
        for (ix = point_list_tail->x;
             (delx > 0) ? (ix < x) : (ix > x);
             ix += (delx > 0) ? 1 : -1) {

          /* step the other axis by the correct increment */
          iy += fabs(((float) dely
                      / (float) delx)) * (float) ((dely < 0) ? -1.0 : 1.0);

          /* add the interpolated point */
          point_list_tail->next = new_point_p;
          point_list_tail = new_point_p;
          new_point_p->x = ix;
          new_point_p->y = iy;
          new_point_p->next = NULL;

          /* update metrics */
          if (((int) ix) < min_x) min_x = (int) ix;
          if (((int) ix) > max_x) max_x = (int) ix;
          if (((int) iy) < min_y) min_y = (int) iy;
          if (((int) iy) > max_y) max_y = (int) iy;
          point_count++;

          new_point_p = (p_point) malloc (sizeof(struct s_point));
        }
      } else {  /* same thing, but for dely larger than delx case... */
        ix = point_list_tail->x;

        /* go from the last point to the current, whatever direction
           it may be */
        for (iy = point_list_tail->y; (dely > 0) ? (iy < y) : (iy > y);
             iy += (dely > 0) ? 1 : -1) {

          /* step the other axis by the correct increment */
          ix += fabs(((float) delx / (float) dely))
            * (float) ((delx < 0) ? -1.0 : 1.0);

          /* add the interpolated point */
          point_list_tail->next = new_point_p;
          point_list_tail = new_point_p;
          new_point_p->y = iy;
          new_point_p->x = ix;
          new_point_p->next = NULL;

          /* update metrics */
          if (((int) ix) < min_x) min_x = (int) ix;
          if (((int) ix) > max_x) max_x = (int) ix;
          if (((int) iy) < min_y) min_y = (int) iy;
          if (((int) iy) > max_y) max_y = (int) iy;
          point_count++;

          new_point_p = (p_point) malloc (sizeof(struct s_point));
        }
      }

      /* add the sampled point */
      point_list_tail->next = new_point_p;
      point_list_tail = new_point_p;
    }

    /* record the sampled point values */
    new_point_p->x = x;
    new_point_p->y = y;
    new_point_p->next = NULL;
  }
}
