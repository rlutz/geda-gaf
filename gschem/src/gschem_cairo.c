/* gEDA - GPL Electronic Design Automation
 * gschem - gEDA Schematic Capture
 * Copyright (C) 1998-2007 Ales Hvezda
 * Copyright (C) 1998-2007 gEDA Contributors (see ChangeLog for details)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111 USA
 */


#include <config.h>

#include <cairo.h>

#include "gschem.h"

#ifdef HAVE_LIBDMALLOC
#include <dmalloc.h>
#endif


void gschem_cairo_line (cairo_t *cr, int line_end, int width,
                        int x1, int y1, int x2, int y2)
{
  double offset = ((width % 2) == 0) ? 0 : 0.5;
  double xoffset = 0;
  double yoffset = 0;
  int horizontal = 0;
  int vertical = 0;

  if (width == 0)
    return;

  if (y1 == y2) horizontal = 1;
  if (x1 == x2) vertical = 1;

  /* Hint so the length of the line runs along a pixel boundary */

  if (horizontal)
    yoffset = offset;
  else if (vertical)
    xoffset = offset;

  /* Now hint the ends of the lines */

  switch (line_end) {
    case END_NONE:
      /* Line terminates at the passed coordinate */
      /* Do nothing */
      break;
    case END_SQUARE:
      /* Line terminates half a width away from the passed coordinate */
      if (horizontal) {
        xoffset = offset;
      } else if (vertical) {
        yoffset = offset;
      }
    case END_ROUND:
      /* Do nothing */
      break;
  }

  cairo_move_to (cr, x1 + xoffset, y1 + yoffset);
  cairo_line_to (cr, x2 + xoffset, y2 + yoffset);
}


void gschem_cairo_box (cairo_t *cr, int width,
                       int x1, int y1, int x2, int y2)
{
  double offset = ((width % 2) == 0) ? 0 : 0.5;
  cairo_move_to (cr, x2 + offset, y2 + offset);
  cairo_line_to (cr, x1 + offset, y2 + offset);
  cairo_line_to (cr, x1 + offset, y1 + offset);
  cairo_line_to (cr, x2 + offset, y1 + offset);
  cairo_close_path (cr);
}


void gschem_cairo_stroke (cairo_t *cr, int line_type, int line_end,
                          int width, int length, int space)
{
  double dashes[4];
  cairo_line_cap_t cap;
  int num_dashes;

  cairo_set_line_width (cr, width);
  cairo_set_line_join (cr, CAIRO_LINE_JOIN_MITER);

  switch (line_end) {
    case END_NONE:   cap = CAIRO_LINE_CAP_BUTT;   break;
    case END_SQUARE: cap = CAIRO_LINE_CAP_SQUARE; break;
    case END_ROUND:  cap = CAIRO_LINE_CAP_ROUND;  break;
    default:
      fprintf(stderr, _("Unknown end for line (%d)\n"), line_end);
      cap = CAIRO_LINE_CAP_BUTT;
    break;
  }

  switch (line_type) {

    default:
      fprintf(stderr, _("Unknown type for stroke (%d) !\n"), line_type);
      /* Fall through */

    case TYPE_SOLID:
      num_dashes = 0;

      cairo_set_dash (cr, dashes, num_dashes, 0.);
      cairo_set_line_cap (cr, cap);
      cairo_stroke (cr);
      break;

    case TYPE_DOTTED:
      dashes[0] = 0;                    /* DOT */
      dashes[1] = space;
      num_dashes = 2;

      cairo_set_dash (cr, dashes, num_dashes, 0.);
      cairo_set_line_cap (cr, CAIRO_LINE_CAP_ROUND);
      cairo_stroke (cr);
      break;

    case TYPE_DASHED:
      dashes[0] = length;               /* DASH */
      dashes[1] = space;
      num_dashes = 2;

      cairo_set_dash (cr, dashes, num_dashes, 0.);
      cairo_set_line_cap (cr, CAIRO_LINE_CAP_BUTT);
      cairo_stroke (cr);
      break;

    case TYPE_CENTER:
      dashes[0] = length;               /* DASH */
      dashes[1] = 2 * space;
      num_dashes = 2;

      cairo_set_dash (cr, dashes, num_dashes, 0.);
      cairo_set_line_cap (cr, CAIRO_LINE_CAP_BUTT);
      cairo_stroke_preserve (cr);

      dashes[0] = 0;                    /* DOT */
      dashes[1] = 2 * space + length;
      num_dashes = 2;

      cairo_set_dash (cr, dashes, num_dashes, -length - space);
      cairo_set_line_cap (cr, CAIRO_LINE_CAP_ROUND);
      cairo_stroke (cr);
      break;

    case TYPE_PHANTOM:
      dashes[0] = length;               /* DASH */
      dashes[1] = 3 * space;
      num_dashes = 2;

      cairo_set_dash (cr, dashes, num_dashes, 0.);
      cairo_set_line_cap (cr, CAIRO_LINE_CAP_BUTT);
      cairo_stroke_preserve (cr);

      dashes[0] = 0;                    /* DOT */
      dashes[1] = space;
      dashes[2] = 0;                    /* DOT */
      dashes[3] = 2 * space + length;
      num_dashes = 4;

      cairo_set_dash (cr, dashes, num_dashes, -length - space);
      cairo_set_line_cap (cr, CAIRO_LINE_CAP_ROUND);
      cairo_stroke (cr);
      break;
  }

  cairo_set_dash (cr, NULL, 0, 0.);
}


void gschem_cairo_set_source_color (cairo_t *cr, COLOR *color)
{
  cairo_set_source_rgba (cr, (double)color->r / 255.0,
                             (double)color->g / 255.0,
                             (double)color->b / 255.0,
                             (double)color->a / 255.0);
}