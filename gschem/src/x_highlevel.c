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

/*! \file x_highlevel.c
 * \brief High-level page-related functions.
 *
 * As opposed to the low-level page functions, the high-level page
 * functions do interact with the user.  They switch to the newly
 * created / opened page and ask for confirmation for potentially
 * destructive actions.
 */
#include <config.h>

#include "gschem.h"


/*! \brief Create a new untitled page with a titleblock.
 *
 * If \a filename is \c NULL, the name of the new page is build from
 * configuration data ('untitled-name') and a counter for uniqueness.
 *
 * The page becomes the new current page of \a w_current.
 *
 * \param [in] w_current  the toplevel environment
 * \param [in] filename   the filename for the new page, or \c NULL to
 *                        generate an untitled filename
 *
 * \returns a pointer to the new page, or \c NULL if a file with the
 *          specified filename already exists
 */
PAGE *
x_highlevel_new_page (GschemToplevel *w_current, const gchar *filename)
{
  PAGE *page = x_lowlevel_new_page (w_current, filename);

  if (page != NULL)
    x_window_set_current_page (w_current, page);

  return page;
}


/*! \brief Open a new page from a file, or find an existing page.
 *
 * Creates a new page and loads the file in it.  If there is already a
 * matching page in \a w_current, returns a pointer to the existing
 * page instead.
 *
 * The page becomes the new current page of \a w_current.
 *
 * \param [in] w_current  the toplevel environment
 * \param [in] filename   the name of the file to open
 *
 * \returns a pointer to the page, or \c NULL if the file couldn't be
 *          loaded
 */
PAGE *
x_highlevel_open_page (GschemToplevel *w_current, const gchar *filename)
{
  PAGE *page = x_lowlevel_open_page (w_current, filename);

  if (page != NULL)
    x_window_set_current_page (w_current, page);

  return page;
}


/*! \brief Save a page.
 *
 * Saves a page to its current filename.  If the page is untitled,
 * shows a file chooser dialog.
 *
 * \param [in] w_current  the toplevel environment
 * \param [in] page       the page to save, or \c NULL to save the
 *                        current page
 *
 * \returns \c TRUE if the page was saved, \c FALSE otherwise
 */
gboolean
x_highlevel_save_page (GschemToplevel *w_current, PAGE *page)
{
  TOPLEVEL *toplevel = gschem_toplevel_get_toplevel (w_current);
  EdaConfig *cfg;
  gchar *untitled_name;
  gboolean success;

  if (page == NULL) {
    page = toplevel->page_current;
    g_return_val_if_fail (page != NULL, FALSE);
  }

  /*! \bug This is a dreadful way of figuring out whether a page is
   *  newly-created or not. */
  cfg = eda_config_get_context_for_path (page->page_filename);
  untitled_name = eda_config_get_string (cfg, "gschem", "default-filename", NULL);
  if (strstr(page->page_filename, untitled_name)) {
    success = x_fileselect_save (w_current);
  } else {
    success = x_lowlevel_save_page (w_current, page, page->page_filename);
  }
  g_free (untitled_name);
  return success;
}


/*! \brief Reload a page from disk.
 *
 * Closes the page, creates a new page, and reads the file back from
 * disk.  If the page has been changed, asks the user for confirmation.
 *
 * \param [in] w_current  the toplevel environment
 * \param [in] page       the page to revert, or \c NULL to revert the
 *                        current page
 *
 * \returns \c TRUE if the page was successfully reloaded, \c FALSE otherwise
 */
gboolean
x_highlevel_revert_page (GschemToplevel *w_current, PAGE *page)
{
  TOPLEVEL *toplevel = gschem_toplevel_get_toplevel (w_current);
  int response;
  GtkWidget* dialog;

  if (page == NULL) {
    page = toplevel->page_current;
    g_return_val_if_fail (page != NULL, FALSE);
  }

  if (page->CHANGED) {
    dialog = gtk_message_dialog_new ((GtkWindow *) w_current->main_window,
                                     GTK_DIALOG_DESTROY_WITH_PARENT,
                                     GTK_MESSAGE_QUESTION,
                                     GTK_BUTTONS_YES_NO,
                                     _("Really revert page?"));

    /* Set the alternative button order (ok, cancel, help) for other systems */
    gtk_dialog_set_alternative_button_order (GTK_DIALOG (dialog),
                                             GTK_RESPONSE_YES,
                                             GTK_RESPONSE_NO,
                                             -1);

    response = gtk_dialog_run (GTK_DIALOG (dialog));
    gtk_widget_destroy (dialog);

    if (response != GTK_RESPONSE_YES)
      return FALSE;
  }

  return x_lowlevel_revert_page (w_current, page);
}


/*! \brief Close a page.
 *
 * If the page has been changed, asks the user for confirmation.
 *
 * Switches to the next valid page if necessary.  If this was the last
 * page of the toplevel, a new untitled page is created.
 *
 * \param [in] w_current  the toplevel environment
 * \param [in] page       the page to close, or \c NULL to close the
 *                        current page
 *
 * \returns \c TRUE if the page was closed, \c FALSE otherwise
 */
gboolean
x_highlevel_close_page (GschemToplevel *w_current, PAGE *page)
{
  TOPLEVEL *toplevel = gschem_toplevel_get_toplevel (w_current);

  if (page == NULL) {
    page = toplevel->page_current;
    g_return_val_if_fail (page != NULL, FALSE);
  }

  if (page->CHANGED
      && !x_dialog_close_changed_page (w_current, page)) {
    return FALSE;
  }

  x_lowlevel_close_page (w_current, page);
  return TRUE;
}
