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


/*!
 *  \brief Search for schematic associated source files and load them.
 *  \par Function Description
 *  This function searches the associated source file refered by the
 *  <B>filename</B> and loads it.  If the page is already in the list of
 *  pages it will return the <B>pid</B> of that page.
 *
 *  \param [in] toplevel      The TOPLEVEL object.
 *  \param [in] filename      Schematic file name.
 *  \param [in] parent        The parent page of the schematic.
 *  \param [in] page_control
 *  \param [out] err         Location to return a GError on failure.
 *  \return The page loaded, or NULL if failed.
 *
 *  \note
 *  This function finds the associated source files and
 *  loads all up
 *  It only works for schematic files though
 *  this is basically push
 */
static PAGE *
down_schematic_single (TOPLEVEL *toplevel, const gchar *filename,
                       PAGE *parent, int page_control,
                       GError **err)
{
  gchar *string;
  PAGE *found = NULL;

  g_return_val_if_fail (toplevel != NULL, NULL);
  g_return_val_if_fail (filename != NULL, NULL);
  g_return_val_if_fail (parent != NULL, NULL);

  string = s_slib_search_single (filename);
  if (string == NULL) {
    g_set_error (err, EDA_ERROR, EDA_ERROR_NOLIB,
                 _("Schematic not found in source library."));
    return NULL;
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
      g_set_error (err, EDA_ERROR, EDA_ERROR_LOOP,
                   _("Hierarchy contains a circular dependency."));
      return NULL;  /* error signal */
    }
    s_page_goto (toplevel, found);
    if (page_control != 0)
      found->page_control = page_control;
    found->up = parent->pid;
    g_free (string);
    return found;
  }

  found = s_page_new (toplevel, string);

  f_open (toplevel, found, found->page_filename, NULL);

  if (page_control == 0) {
    page_control_counter++;
    found->page_control = page_control_counter;
  } else
    found->page_control = page_control;

  found->up = parent->pid;

  g_free (string);

  return found;
}


static PAGE *
load_source (GschemToplevel *w_current, const gchar *filename,
             int *page_control)
{
  TOPLEVEL *toplevel = gschem_toplevel_get_toplevel (w_current);
  PAGE *child;

  GError *err = NULL;
  s_log_message (_("Searching for source [%s]\n"), filename);
  PAGE *saved_page = toplevel->page_current;
  child = down_schematic_single (toplevel, filename, toplevel->page_current,
                                 *page_control, &err);

  /* down_schematic_single() will not zoom the loaded page */
  if (child != NULL) {
    gtk_recent_manager_add_item (recent_manager,
                                 g_filename_to_uri (child->page_filename,
                                                    NULL, NULL));

    s_page_goto (toplevel, child);
    gschem_toplevel_page_changed (w_current);
    gschem_page_view_zoom_extents (
      gschem_toplevel_get_current_page_view (w_current), NULL);
    o_undo_savestate_old (w_current, UNDO_ALL, NULL);
  }
  if (saved_page != NULL) {
    s_page_goto (toplevel, saved_page);
    gschem_toplevel_page_changed (w_current);
  }

  /* now do some error fixing */
  if (child == NULL) {
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
  } else
    *page_control = child->page_control;

  return child;
}


void
x_hierarchy_down_schematic (GschemToplevel *w_current, OBJECT *object)
{
  char *attrib = NULL;
  int count = 0;
  int looking_inside = FALSE;
  int pcount = 0;
  char *current_filename = NULL;
  PAGE *child = NULL;
  int page_control = 0;
  PAGE *first_page = NULL;

  /* only allow going into symbols */
  if (object->type != OBJ_COMPLEX)
    return;

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
      child = load_source (w_current, current_filename, &page_control);

      /* save the first page */
      if (first_page == NULL)
        first_page = child;

      g_free (current_filename);
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

  if (first_page != NULL)
    x_window_set_current_page (w_current, first_page);
}


/*! \todo Finish function documentation!!!
 *  \brief
 *  \par Function Description
 *
 */
static void
down_symbol (TOPLEVEL *toplevel, const CLibSymbol *symbol, PAGE *parent)
{
  PAGE *page;
  gchar *filename;

  filename = s_clib_symbol_get_filename (symbol);

  page = s_page_search (toplevel, filename);
  if (page) {
    /* change link to parent page since we can come here from any
       parent and must come back to the same page */
    page->up = parent->pid;
    s_page_goto (toplevel, page);
    g_free (filename);
    return;
  }

  page = s_page_new (toplevel, filename);
  g_free (filename);

  s_page_goto (toplevel, page);

  f_open (toplevel, page, page->page_filename, NULL);

  page->up = parent->pid;
  page_control_counter++;
  page->page_control = page_control_counter;
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
  down_symbol (toplevel, sym, toplevel->page_current);
  PAGE *page = toplevel->page_current;
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


/*! \brief Search for the parent page of a page in hierarchy.
 *  \par Function Description
 *  This function searches the parent page of page \a page in the
 *  hierarchy. It checks all the pages in the list \a page_list.
 *
 *  It returns a pointer on the page if found, NULL otherwise.
 *
 *  \note
 *  The page \a current_page must be in the list \a page_list.
 *
 *  \param [in] page_list    The list of pages in which to search.
 *  \param [in] current_page The reference page for the search.
 *  \returns A pointer on the page found or NULL if not found.
 */
static PAGE *
find_up_page (GedaPageList *page_list, PAGE *current_page)
{
  g_return_val_if_fail (current_page != NULL, NULL);
  if (current_page->up < 0) {
    s_log_message (_("There are no schematics above the current one!\n"));
    return NULL;
  }

  return s_page_search_by_page_id (page_list, current_page->up);
}


void
x_hierarchy_up (GschemToplevel *w_current)
{
  TOPLEVEL *toplevel = gschem_toplevel_get_toplevel (w_current);
  PAGE *page = NULL;
  PAGE *up_page = NULL;

  page = toplevel->page_current;
  g_return_if_fail (page != NULL);

  up_page = find_up_page (toplevel->pages, page);
  if (up_page == NULL) {
    s_log_message (_("Cannot find any schematics above the current one!\n"));
    return;
  }

  if (!x_highlevel_close_page (w_current, page))
    return;
  x_window_set_current_page (w_current, up_page);
}
