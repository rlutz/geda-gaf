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

#include <stdio.h>
#include <sys/stat.h>
#include <math.h>
#ifdef HAVE_STRING_H
#include <string.h>
#endif
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include "gschem.h"

/*! \todo Finish function documentation!!!
 *  \brief
 *  \par Function Description
 *
 */
int o_text_get_rendered_bounds (void *user_data, OBJECT *o_current,
                                int *min_x, int *min_y,
                                int *max_x, int *max_y)
{
  TOPLEVEL *toplevel;
  EdaRenderer *renderer;
  cairo_t *cr;
  cairo_matrix_t render_mtx;
  int result, render_flags = 0;
  double t, l, r, b;
  GschemToplevel *w_current = (GschemToplevel *) user_data;
  g_return_val_if_fail ((w_current != NULL), FALSE);

  GschemPageView *page_view = gschem_toplevel_get_current_page_view (w_current);
  g_return_val_if_fail ((page_view != NULL), FALSE);

  toplevel = gschem_toplevel_get_toplevel (w_current);
  g_return_val_if_fail ((toplevel != NULL), FALSE);

  cr = gdk_cairo_create (gtk_widget_get_window (GTK_WIDGET(page_view)));

  /* Set up renderer based on configuration in w_current. Note that we
   * *don't* enable hinting, because if its enabled the calculated
   * bounds are zoom-level-dependent. */
  if (toplevel->show_hidden_text)
    render_flags |= EDA_RENDERER_FLAG_TEXT_HIDDEN;
  renderer = g_object_ref (w_current->renderer);
  g_object_set (G_OBJECT (renderer),
                "cairo-context", cr,
                "render-flags", render_flags,
                NULL);

  /* We need to transform the cairo context to approximate world
   * coordinates. */
  cairo_matrix_init (&render_mtx, 1, 0, 0, -1, -1, 1);
  cairo_set_matrix (cr, &render_mtx);

  /* Use the renderer to calculate text bounds */
  result = eda_renderer_get_user_bounds (renderer, o_current, &l, &t, &r, &b);

  /* Clean up */
  eda_renderer_destroy (renderer);
  cairo_destroy (cr);

  /* Round bounds to nearest integer */
  if (result) {
    *min_x = lrint (fmin (l, r));
    *min_y = lrint (fmin (t, b));
    *max_x = lrint (fmax (l, r));
    *max_y = lrint (fmax (t, b));
  }

  return result;
}

/*! \todo Finish function documentation!!!
 *  \brief
 *  \par Function Description
 *
 */
void o_text_prepare_place(GschemToplevel *w_current, char *text, int color, int align, int rotate, int size)
{
  GschemPageView *page_view = gschem_toplevel_get_current_page_view (w_current);
  g_return_if_fail (page_view != NULL);

  PAGE *page = gschem_page_view_get_page (page_view);
  if (page == NULL) {
    return;
  }

  TOPLEVEL *toplevel = page->toplevel;
  g_return_if_fail (toplevel != NULL);


  /* Insert the new object into the buffer at world coordinates (0,0).
   * It will be translated to the mouse coordinates during placement. */

  w_current->first_wx = 0;
  w_current->first_wy = 0;

  w_current->last_drawb_mode = LAST_DRAWB_MODE_NONE;

  /* remove the old place list if it exists */
  s_delete_object_glist(toplevel, page->place_list);
  page->place_list = NULL;

  /* here you need to add OBJ_TEXT when it's done */
  page->place_list =
    g_list_append(page->place_list,
                  o_text_new (toplevel, color,
                              0, 0, align, rotate, /* zero is angle */
                              text,
                              size,
                              /* has to be visible so you can place it */
                              /* visibility is set when you create the object */
                              VISIBLE, SHOW_NAME_VALUE));

  i_action_start (w_current);
  i_set_state (w_current, TEXTMODE);
}


/*! \todo Finish function documentation!!!
 *  \brief
 *  \par Function Description
 *
 *  \note
 *  The object passed in should be the REAL object, NOT any copy in any
 *  selection list
 */
void o_text_change(GschemToplevel *w_current, OBJECT *object, char *string,
		   int visibility, int show)
{
  g_return_if_fail (w_current != NULL);

  GschemPageView *page_view = gschem_toplevel_get_current_page_view (w_current);
  g_return_if_fail (page_view != NULL);

  PAGE *page = gschem_page_view_get_page (page_view);
  TOPLEVEL *toplevel = page->toplevel;

  g_return_if_fail (toplevel != NULL);
  g_return_if_fail (page != NULL);

  if (object == NULL) {
    return;
  }

  if (object->type != OBJ_TEXT) {
    return;
  }

  o_text_set_string (toplevel, object, string);

  o_set_visibility (toplevel, object, visibility);
  object->show_name_value = show;
  o_text_recreate(toplevel, object);

  /* handle slot= attribute, it's a special case */
  if (object->attached_to != NULL &&
      g_ascii_strncasecmp (string, "slot=", 5) == 0) {
    o_slot_end (w_current, object->attached_to, string);
  }
}


/*! \brief Toggle a text object's overbar.
 *
 * Adds an overbar marker (backslash-underline) to the begin and the
 * end of the text object's contents if there's not already an overbar
 * marker; otherwise, removes the existing marker.  Begin and end are
 * treated independently, so the parts of the text with and without
 * overbar are complemented.
 *
 * \returns whether the object has been changed
 */
gboolean
o_text_toggle_overbar (GschemToplevel *w_current, OBJECT *object)
{
  TOPLEVEL *toplevel = gschem_toplevel_get_toplevel (w_current);
  gchar *name, *value, *buf, *ptr, *new_string;

  if (object->type != OBJ_TEXT || object->text->string == NULL
                               || object->text->string[0] == '\0')
    return FALSE;

  if (o_attrib_string_get_name_value (object->text->string, &name, &value)) {
    buf = g_strdup_printf ("\\_%s\\_", value);
    g_free (value);
  } else {
    name = NULL;
    buf = g_strdup_printf ("\\_%s\\_", object->text->string);
  }

  ptr = buf + strlen (buf) - 4;
  if (strncmp (ptr, "\\_\\_", 4) == 0)
    *ptr = '\0';
  ptr = buf;
  if (strncmp (ptr, "\\_\\_", 4) == 0)
    ptr += 4;

  if (name == NULL)
    new_string = g_strdup (ptr);
  else {
    new_string = g_strdup_printf ("%s=%s", name, ptr);
    g_free (name);
  }
  g_free (buf);

  o_text_set_string (toplevel, object, new_string);
  /* o_text_recreate is called by o_text_set_string */

  /* handle slot= attribute, it's a special case */
  if (object->attached_to != NULL &&
      g_ascii_strncasecmp (new_string, "slot=", 5) == 0)
    o_slot_end (w_current, object->attached_to, new_string);

  g_free (new_string);
  return TRUE;
}
