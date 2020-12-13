/* gEDA - GPL Electronic Design Automation
 * gschem - gEDA Schematic Capture
 * Copyright (C) 1998-2010 Ales Hvezda
 * Copyright (C) 1998-2020 gEDA Contributors (see ChangeLog for details)
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */
#include <config.h>
#include <math.h>
#include "gschem.h"


/* \brief Invalidate temporary origin marker.
 */
void
o_ognrst_invalidate_rubber (GschemToplevel *w_current)
{
  g_return_if_fail (w_current != NULL);

  GschemPageView *page_view = gschem_toplevel_get_current_page_view (w_current);
  gschem_page_view_invalidate_all (page_view);
}

/*! \brief Perform an origin reset operation.
 *
 * Ends the process of interactively resetting the origin of the
 * current sheet by translating all objects, panning the view
 * accordingly, and leaving origin reset mode.  If the old origin is
 * selected, origin reset mode is left without performing an operation.
 *
 * The arguments \a w_x and \w_y are discarded; the last coordinates
 * saved in \b w_current->first_wx and \b w_current->first_wy are used
 * instead.
 */
void
o_ognrst_end (GschemToplevel *w_current, int w_x, int w_y)
{
  if (w_current->first_wx == 0 && w_current->first_wy == 0) {
    s_log_message (_("Not translating schematic\n"));
    o_ognrst_invalidate_rubber (w_current);
    i_action_stop (w_current);
    i_set_state (w_current, SELECT);
    return;
  }

  GschemPageView *page_view = gschem_toplevel_get_current_page_view (w_current);
  g_return_if_fail (page_view != NULL);

  PAGE *page = gschem_page_view_get_page (page_view);
  g_return_if_fail (page != NULL);

  TOPLEVEL *toplevel = page->toplevel;
  g_return_if_fail (toplevel != NULL);

  gschem_bottom_widget_set_coordinates (
    GSCHEM_BOTTOM_WIDGET (w_current->bottom_widget), 0, 0);


  /* translate schematic */
  for (const GList *l = s_page_objects (page); l != NULL; l = l->next)
    s_conn_remove_object_connections (toplevel, (OBJECT *) l->data);

  s_log_message (_("Translating schematic [%d %d]\n"), -w_current->first_wx,
                                                       -w_current->first_wy);
  o_glist_translate_world (s_page_objects (page), -w_current->first_wx,
                                                  -w_current->first_wy);

  for (const GList *l = s_page_objects (page); l != NULL; l = l->next)
    s_conn_update_object (page, (OBJECT *) l->data);


  /* pan view */
  GschemPageGeometry *geometry =
    gschem_page_view_get_page_geometry (page_view);
  gschem_page_view_pan_general (
    page_view,
    (geometry->viewport_right +
     geometry->viewport_left) / 2 - w_current->first_wx,
    (geometry->viewport_top +
     geometry->viewport_bottom) / 2 - w_current->first_wy, 1.);
  gschem_page_view_invalidate_all (page_view);


  gschem_toplevel_page_content_changed (w_current, page);
  o_undo_savestate (w_current, page, UNDO_ALL, _("Place Origin"));
  if (page->undo_current != NULL) {
    page->undo_current->tx = -w_current->first_wx;
    page->undo_current->ty = -w_current->first_wy;
  }

  i_action_stop (w_current);
  i_set_state (w_current, SELECT);
}

/*! \brief Draw temporary origin while moving the mouse cursor.
 *
 * Manages the erase/update/draw process of the temporary origin while
 * moving the mouse in origin reset mode.
 *
 * \param [in] w_current  The GschemToplevel object.
 * \param [in] w_x        Current X coordinate of pointer in world units.
 * \param [in] w_y        Current Y coordinate of pointer in world units.
 */
void
o_ognrst_motion (GschemToplevel *w_current, int w_x, int w_y)
{
  if (w_current->rubber_visible)
    o_ognrst_invalidate_rubber (w_current);

  /* The coordinates of the temporary origin marker are updated.  Its
   * new coordinates are in \b w_x and \b w_y parameters and saved to
   * \b w_current->first_wx and \b w_current->first_wy, respectively. */
  w_current->first_wx = w_x;
  w_current->first_wy = w_y;

  o_ognrst_invalidate_rubber (w_current);
  w_current->rubber_visible = 1;
}

/*! \brief Draw temporary origin marker.
 *
 * Draws a temporary origin marker at the point (\b
 * w_current->first_wx, \b w_current->first_wy).
 */
void
o_ognrst_draw_rubber (GschemToplevel *w_current, EdaRenderer *renderer,
                      int x, int y, int width, int height)
{
  GschemPageView *page_view =
    gschem_toplevel_get_current_page_view (w_current);
  if (page_view != NULL && page_view->doing_pan)
    return;

  cairo_t *cr = eda_renderer_get_cairo_context (renderer);

  double x_start = x - 1;
  double y_start = y + height + 1;
  double x_end = x + width + 1;
  double y_end = y - 1;
  cairo_device_to_user (cr, &x_start, &y_start);
  cairo_device_to_user (cr, &x_end, &y_end);

  cairo_matrix_t user_to_device_matrix;
  cairo_get_matrix (cr, &user_to_device_matrix);

  cairo_save (cr);
  cairo_identity_matrix (cr);
  cairo_translate (cr, .5, .5);

  COLOR *c = x_color_lookup (PLACE_ORIGIN_COLOR);
  cairo_set_source_rgba (cr, (double)c->r / 255.,
                             (double)c->g / 255.,
                             (double)c->b / 255.,
                             (double)c->a / 255.);
  cairo_set_line_width (cr, 1.);
  cairo_set_line_cap (cr, CAIRO_LINE_CAP_SQUARE);

  double x0, y0, x1, y1;

  x0 = floor (x_start);
  y0 = w_current->first_wy;
  x1 = ceil (x_end);
  y1 = w_current->first_wy;

  cairo_matrix_transform_point (&user_to_device_matrix, &x0, &y0);
  cairo_matrix_transform_point (&user_to_device_matrix, &x1, &y1);

  cairo_move_to (cr, (int)(x0 + .5), (int)(y0 + .5));
  cairo_line_to (cr, (int)(x1 + .5), (int)(y1 + .5));
  cairo_stroke (cr);

  x0 = w_current->first_wx;
  y0 = floor (y_start);
  x1 = w_current->first_wx;
  y1 = ceil (y_end);

  cairo_matrix_transform_point (&user_to_device_matrix, &x0, &y0);
  cairo_matrix_transform_point (&user_to_device_matrix, &x1, &y1);

  cairo_move_to (cr, (int)(x0 + .5), (int)(y0 + .5));
  cairo_line_to (cr, (int)(x1 + .5), (int)(y1 + .5));
  cairo_stroke (cr);

  cairo_restore (cr);
}
