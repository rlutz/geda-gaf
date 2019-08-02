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
 * created / opened page, ask for confirmation for potentially
 * destructive actions (switching pages if necessary), and warn about
 * major symbol changes.
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
 * Creates a new page, loads the file in it, and shows the "Major
 * symbol changes" dialog if major symversion mismatches were found.
 * If there is already a matching page in \a w_current, returns a
 * pointer to the existing page instead.
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

  /* if there were any symbols which had major changes, put up an
     error dialog box */
  major_changed_dialog (w_current);

  return page;
}


/*! \brief Open multiple pages from files.
 *
 * For each file specified in \a filenames that is not already opened,
 * creates a new page and loads the file in it.  If major symversion
 * mismatches were found in any of the files, the "Major symbol
 * changes" dialog is shown.
 *
 * The first page that could be opened or already existed becomes the
 * new current page of \a w_current.
 *
 * \param [in] w_current  the toplevel environment
 * \param [in] filenames  a GSList of filenames to open
 *
 * \returns \c TRUE if all files could be opened, \c FALSE otherwise
 */
gboolean
x_highlevel_open_pages (GschemToplevel *w_current, GSList *filenames)
{
  PAGE *first_page = NULL;
  gboolean success = TRUE;

  /* open each file */
  for (GSList *l = filenames; l != NULL; l = l->next) {
    PAGE *page = x_lowlevel_open_page (w_current, (gchar *) l->data);
    if (page == NULL)
      success = FALSE;
    else if (first_page == NULL)
      first_page = page;
  }

  if (first_page != NULL)
    x_window_set_current_page (w_current, first_page);

  /* if there were any symbols which had major changes, put up an
     error dialog box */
  major_changed_dialog (w_current);

  return success;
}


/*! \brief Save a page.
 *
 * Saves a page to its current filename.  If the page is untitled,
 * makes it the current page of \a w_current and shows a file chooser
 * dialog.
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

  if (page == NULL) {
    page = toplevel->page_current;
    g_return_val_if_fail (page != NULL, FALSE);
  }

  if (page->is_untitled) {
    x_window_set_current_page (w_current, page);

    return x_fileselect_save (w_current);
  }

  return x_lowlevel_save_page (w_current, page, page->page_filename);
}


/*! \brief Reload a page from disk.
 *
 * Closes the page, creates a new page, and reads the file back from
 * disk.  If the page has been changed, makes it the current page of
 * \a w_current and asks the user for confirmation.
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

  if (page == NULL) {
    page = toplevel->page_current;
    g_return_val_if_fail (page != NULL, FALSE);
  }

  if (page->CHANGED) {
    GtkWidget *dialog;
    int response;

    x_window_set_current_page (w_current, page);

    gchar *basename = g_path_get_basename (page->page_filename);
    dialog = gtk_message_dialog_new_with_markup (
      GTK_WINDOW (w_current->main_window),
      GTK_DIALOG_DESTROY_WITH_PARENT,
      GTK_MESSAGE_WARNING,
      GTK_BUTTONS_NONE,
      _("<big><b>Really revert \"%s\"?</b></big>"),
      basename);
    g_free (basename);
    g_object_set (dialog, "secondary-text",
                  _("By reverting the file to the state saved on disk, "
                    "you will lose your unsaved changes, including all "
                    "undo information."), NULL);
    gtk_dialog_add_buttons (GTK_DIALOG (dialog),
                            GTK_STOCK_CANCEL,          GTK_RESPONSE_CANCEL,
                            GTK_STOCK_REVERT_TO_SAVED, GTK_RESPONSE_ACCEPT,
                            NULL);

    /* Set the alternative button order (ok, cancel, help) for other systems */
    gtk_dialog_set_alternative_button_order (GTK_DIALOG (dialog),
                                             GTK_RESPONSE_ACCEPT,
                                             GTK_RESPONSE_CANCEL,
                                             -1);

    gtk_dialog_set_default_response (GTK_DIALOG (dialog), GTK_RESPONSE_ACCEPT);
    gtk_window_set_title (GTK_WINDOW (dialog), _("gschem"));

    response = gtk_dialog_run (GTK_DIALOG (dialog));
    gtk_widget_destroy (dialog);

    if (response != GTK_RESPONSE_ACCEPT)
      return FALSE;
  }

  return x_lowlevel_revert_page (w_current, page);
}


/*! \brief Close a page.
 *
 * If the page has been changed, makes it the current page of \a
 * toplevel and asks the user for confirmation.
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

  if (page->CHANGED) {
    /* Setting the current page is redundant as the close confirmation
       dialog switches to the current page (is has to, since it may be
       called when the window is closed while there's a single changed
       page in the background), but it doesn't hurt, either. */
    x_window_set_current_page (w_current, page);

    if (!x_dialog_close_changed_page (w_current, page))
      return FALSE;
  }

  x_lowlevel_close_page (w_current, page);
  return TRUE;
}
