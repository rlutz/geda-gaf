/* gEDA - GPL Electronic Design Automation
 * gschem - gEDA Schematic Capture
 * Copyright (C) 1998-2010 Ales Hvezda
 * Copyright (C) 1998-2019 gEDA Contributors (see ChangeLog for details)
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

/*! \file x_hierarchy.c
 * \brief Hierarchy navigation functions.
 */
#include <config.h>

#include "gschem.h"


static PAGE *
load_source (GschemToplevel *w_current, const gchar *filename,
             int *page_control)
{
  TOPLEVEL *toplevel = gschem_toplevel_get_toplevel (w_current);
  PAGE *child;

  GError *err = NULL;
  s_log_message (_("Searching for source [%s]\n"), filename);
  PAGE *saved_page = toplevel->page_current;

  PAGE *parent = toplevel->page_current;
  gchar *string;
  PAGE *found = NULL;

  g_return_val_if_fail (toplevel != NULL, NULL);
  g_return_val_if_fail (filename != NULL, NULL);
  g_return_val_if_fail (parent != NULL, NULL);

  string = s_slib_search_single (filename);
  if (string == NULL) {
    g_set_error (&err, EDA_ERROR, EDA_ERROR_NOLIB,
                 _("Schematic not found in source library."));
    goto error;
  }

  gchar *normalized_filename = f_normalize_filename (string, NULL);
  found = s_page_search (toplevel, normalized_filename);
  g_free (normalized_filename);

  if (found) {
    /* check whether this page is in the parents list */
    PAGE *forbear = parent;
    while (forbear != NULL && found->pid != forbear->pid && forbear->up >= 0)
      forbear = s_page_search_by_page_id (toplevel->pages, forbear->up);

    if (forbear != NULL && found->pid == forbear->pid) {
      g_set_error (&err, EDA_ERROR, EDA_ERROR_LOOP,
                   _("Hierarchy contains a circular dependency."));
      goto error;
    }
    s_page_goto (toplevel, found);
    if (*page_control != 0)
      found->page_control = *page_control;
  } else {
    found = s_page_new (toplevel, string);

    f_open (toplevel, found, found->page_filename, NULL);

    if (*page_control == 0) {
      page_control_counter++;
      found->page_control = page_control_counter;
    } else
      found->page_control = *page_control;
  }
  found->up = parent->pid;
  g_free (string);
  child = found;

  /* down_schematic_single() will not zoom the loaded page */
  gtk_recent_manager_add_item (recent_manager,
                               g_filename_to_uri (child->page_filename,
                                                  NULL, NULL));

  s_page_goto (toplevel, child);
  gschem_toplevel_page_changed (w_current);
  gschem_page_view_zoom_extents (
    gschem_toplevel_get_current_page_view (w_current), NULL);
  o_undo_savestate_old (w_current, UNDO_ALL, NULL);

  if (saved_page != NULL) {
    s_page_goto (toplevel, saved_page);
    gschem_toplevel_page_changed (w_current);
  }

  *page_control = child->page_control;
  return child;

error:
  if (saved_page != NULL) {
    s_page_goto (toplevel, saved_page);
    gschem_toplevel_page_changed (w_current);
  }

  const char *msg = err != NULL ? err->message : "Unknown error.";
  char *secondary =
    g_strdup_printf (_("Failed to descend hierarchy into '%s': %s\n\n"
                       "The gschem log may contain more information."),
                     filename, msg);

  s_log_message (_("Failed to descend into '%s': %s\n"),
                 filename, msg);

  GtkWidget *dialog =
    gtk_message_dialog_new (GTK_WINDOW (w_current->main_window),
                            GTK_DIALOG_MODAL, GTK_MESSAGE_ERROR,
                            GTK_BUTTONS_OK,
                            _("Failed to descend hierarchy."));
  g_object_set (G_OBJECT (dialog), "secondary-text", secondary, NULL);
  gtk_dialog_run (GTK_DIALOG (dialog));
  gtk_widget_destroy (dialog);
  g_free (secondary);
  g_error_free (err);

  return NULL;
}


static GSList *
get_source_filenames (GschemToplevel *w_current, OBJECT *object)
{
  char *attrib = NULL;
  int count = 0;
  int looking_inside = FALSE;
  int pcount = 0;
  char *current_filename = NULL;
  GSList *filenames = NULL;

  /* only allow going into symbols */
  if (object->type != OBJ_COMPLEX)
    return NULL;

  attrib = o_attrib_search_attached_attribs_by_name (object, "source", count);

  /* if above is null, then look inside symbol */
  if (attrib == NULL) {
    attrib =
      o_attrib_search_inherited_attribs_by_name (object, "source", count);
    looking_inside = TRUE;
  }

  while (attrib != NULL) {
    /* look for source=filename,filename, ... */
    pcount = 0;
    current_filename = u_basic_breakup_string (attrib, ',', pcount);

    /* loop over all filenames */
    while (current_filename != NULL) {
      filenames = g_slist_prepend (filenames, current_filename);
      pcount++;
      current_filename = u_basic_breakup_string (attrib, ',', pcount);
    }

    g_free (attrib);
    g_free (current_filename);

    count++;
    attrib = looking_inside ?
      o_attrib_search_inherited_attribs_by_name (object, "source", count) :
      o_attrib_search_attached_attribs_by_name (object, "source", count);
  }

  return g_slist_reverse (filenames);
}


void
x_hierarchy_down_schematic (GschemToplevel *w_current, OBJECT *object)
{
  GSList *filenames = get_source_filenames (w_current, object);
  PAGE *child = NULL;
  int page_control = 0;
  PAGE *first_page = NULL;

  for (const GSList *l = filenames; l != NULL; l = l->next) {
    gchar *current_filename = (gchar *) l->data;
    child = load_source (w_current, current_filename, &page_control);

    /* save the first page */
    if (first_page == NULL)
      first_page = child;

    g_free (current_filename);
  }

  g_slist_free (filenames);

  if (first_page != NULL)
    x_window_set_current_page (w_current, first_page);
}


/*! \bug may cause problems with non-directory symbols */
void
x_hierarchy_down_symbol (GschemToplevel *w_current, OBJECT *object)
{
  TOPLEVEL *toplevel = gschem_toplevel_get_toplevel (w_current);
  const CLibSymbol *sym;

  /* only allow going into symbols */
  if (object->type != OBJ_COMPLEX)
    return;

  if (object->complex_embedded) {
    s_log_message (_("Cannot descend into embedded symbol!\n"));
    return;
  }
  s_log_message (_("Searching for symbol [%s]\n"),
                 object->complex_basename);
  sym = s_clib_get_symbol_by_name (object->complex_basename);
  if (sym == NULL)
    return;
  gchar *filename = s_clib_symbol_get_filename (sym);
  if (filename == NULL) {
    s_log_message (_("Symbol is not a real file. Symbol cannot be loaded.\n"));
    return;
  }
  g_free (filename);
  PAGE *saved_page = toplevel->page_current;

  PAGE *parent = toplevel->page_current;
  PAGE *page;

  filename = s_clib_symbol_get_filename (sym);

  page = s_page_search (toplevel, filename);
  if (page) {
    /* change link to parent page since we can come here from any
       parent and must come back to the same page */
    page->up = parent->pid;
    s_page_goto (toplevel, page);
    g_free (filename);
  } else {
    page = s_page_new (toplevel, filename);
    g_free (filename);

    s_page_goto (toplevel, page);

    f_open (toplevel, page, page->page_filename, NULL);

    page->up = parent->pid;
    page_control_counter++;
    page->page_control = page_control_counter;
  }

  page = toplevel->page_current;
  if (saved_page != NULL) {
    s_page_goto (toplevel, saved_page);
    gschem_toplevel_page_changed (w_current);
  }
  gtk_recent_manager_add_item (recent_manager,
                               g_filename_to_uri (page->page_filename,
                                                  NULL, NULL));

  x_window_set_current_page (w_current, page);
  /* down_symbol() will not zoom the loaded page */
  gschem_page_view_zoom_extents (
    gschem_toplevel_get_current_page_view (w_current), NULL);
  o_undo_savestate_old (w_current, UNDO_ALL, NULL);
}


void
x_hierarchy_up (GschemToplevel *w_current)
{
  TOPLEVEL *toplevel = gschem_toplevel_get_toplevel (w_current);
  PAGE *page = NULL;
  PAGE *up_page = NULL;

  page = toplevel->page_current;
  g_return_if_fail (page != NULL);

  if (page->up < 0) {
    s_log_message (_("There are no schematics above the current one!\n"));
    s_log_message (_("Cannot find any schematics above the current one!\n"));
    return;
  }

  up_page = s_page_search_by_page_id (toplevel->pages, page->up);
  if (up_page == NULL) {
    s_log_message (_("Cannot find any schematics above the current one!\n"));
    return;
  }

  if (!x_highlevel_close_page (w_current, page))
    return;
  x_window_set_current_page (w_current, up_page);
}
