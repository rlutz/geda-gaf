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
  PAGE *parent = toplevel->page_current;
  gchar *string;
  PAGE *page;
  PAGE *forbear;

  g_return_val_if_fail (toplevel != NULL, NULL);
  g_return_val_if_fail (filename != NULL, NULL);
  g_return_val_if_fail (parent != NULL, NULL);

  s_log_message (_("Searching for source [%s]\n"), filename);

  string = s_slib_search_single (filename);
  if (string == NULL) {
    s_log_message (_("Failed to descend into '%s': "
                     "Schematic not found in source library.\n"),
                   filename);

    GtkWidget *dialog =
      gtk_message_dialog_new (GTK_WINDOW (w_current->main_window),
                              GTK_DIALOG_MODAL, GTK_MESSAGE_ERROR,
                              GTK_BUTTONS_CLOSE,
                              _("Failed to descend into \"%s\"."), filename);
    g_object_set (G_OBJECT (dialog), "secondary-text",
                  _("Schematic not found in source library."), NULL);
    gtk_window_set_title (GTK_WINDOW (dialog), _("gschem"));
    gtk_dialog_run (GTK_DIALOG (dialog));
    gtk_widget_destroy (dialog);
    return NULL;
  }

  page = x_lowlevel_open_page (w_current, string);
  g_free (string);

  if (page == NULL)
    /* Some error occurred while loading the schematic.  In this case,
       x_lowlevel_open_page already displayed an error message. */
    return NULL;

  /* check whether this page is in the parents list */
  forbear = parent;
  while (forbear != NULL && page->pid != forbear->pid && forbear->up >= 0)
    forbear = s_page_search_by_page_id (toplevel->pages, forbear->up);

  if (forbear != NULL && page->pid == forbear->pid) {
    s_log_message (_("Failed to descend into '%s': "
                     "Hierarchy contains a circular dependency.\n"),
                   filename);

    GtkWidget *dialog =
      gtk_message_dialog_new (GTK_WINDOW (w_current->main_window),
                              GTK_DIALOG_MODAL, GTK_MESSAGE_ERROR,
                              GTK_BUTTONS_CLOSE,
                              _("Failed to descend into \"%s\"."), filename);
    g_object_set (G_OBJECT (dialog), "secondary-text",
                  _("The hierarchy contains a circular dependency."), NULL);
    gtk_window_set_title (GTK_WINDOW (dialog), _("gschem"));
    gtk_dialog_run (GTK_DIALOG (dialog));
    gtk_widget_destroy (dialog);
    return NULL;
  }

  if (*page_control == 0) {
    page_control_counter++;
    *page_control = page_control_counter;
  }
  page->page_control = *page_control;
  page->up = parent->pid;

  return page;
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
  int page_control = 0;
  PAGE *first_page = NULL;

  for (const GSList *l = filenames; l != NULL; l = l->next) {
    PAGE *page = load_source (w_current, (gchar *) l->data, &page_control);

    if (first_page == NULL)
      first_page = page;
  }

  g_slist_free_full (filenames, g_free);

  if (first_page != NULL)
    x_window_set_current_page (w_current, first_page);
}


/*! \bug may cause problems with non-directory symbols */
void
x_hierarchy_down_symbol (GschemToplevel *w_current, OBJECT *object)
{
  TOPLEVEL *toplevel = gschem_toplevel_get_toplevel (w_current);
  const CLibSymbol *sym;
  gchar *filename;
  PAGE *parent = toplevel->page_current;
  PAGE *page;

  /* only allow going into symbols */
  if (object->type != OBJ_COMPLEX)
    return;

  if (object->complex_embedded) {
    s_log_message (_("Cannot descend into embedded symbol!\n"));
    return;
  }

  s_log_message (_("Searching for symbol [%s]\n"), object->complex_basename);
  sym = s_clib_get_symbol_by_name (object->complex_basename);
  if (sym == NULL)
    return;

  filename = s_clib_symbol_get_filename (sym);
  if (filename == NULL) {
    s_log_message (_("Symbol is not a real file. Symbol cannot be loaded.\n"));
    return;
  }

  page = x_lowlevel_open_page (w_current, filename);
  g_free (filename);

  if (page == NULL)
    /* Some error occurred while loading the symbol.  In this case,
       x_lowlevel_open_page already displayed an error message. */
    return;

  page_control_counter++;
  page->page_control = page_control_counter;
  /* change link to parent page even if the page existed since we can
     come here from any parent and must come back to the same page */
  page->up = parent->pid;

  x_window_set_current_page (w_current, page);
}


void
x_hierarchy_up (GschemToplevel *w_current)
{
  TOPLEVEL *toplevel = gschem_toplevel_get_toplevel (w_current);
  PAGE *page = toplevel->page_current;
  PAGE *parent;
  g_return_if_fail (page != NULL);

  if (page->up < 0) {
    s_log_message (_("There are no schematics above the current one!\n"));
    return;
  }

  parent = s_page_search_by_page_id (toplevel->pages, page->up);
  if (parent == NULL) {
    s_log_message (_("Cannot find any schematics above the current one!\n"));
    return;
  }

  if (!x_highlevel_close_page (w_current, page))
    return;
  x_window_set_current_page (w_current, parent);
}
