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

/*! \file x_highlevel.c
 * \brief High-level page-related functions.
 *
 * As opposed to the low-level page functions, the high-level page
 * functions do interact with the user.  They switch to a newly
 * created / opened page and ask for confirmation for potentially
 * destructive actions (switching pages if necessary).
 */
#include <config.h>

#include <sys/types.h>
#include <sys/stat.h>

#include "gschem.h"


/*! \brief Check if file on disk is more recent than saved timestamp.
 *
 * \param [in] filename         filename on disk
 * \param [in] has_known_mtime  whether \a known_mtime is assumed to
 *                              contain a valid value
 * \param [in] known_mtime      timestamp to compare to
 *
 * \returns If the file is missing, returns \c FALSE.  If no timestamp
 *          was recorded (\a has_known_mtime is \c FALSE) but the file
 *          exists, returns \c TRUE.  Otherwise, returns whether the
 *          file on disk is more recent than the saved timestamp.
 */
static gboolean
file_changed_since (const gchar *filename,
                    gboolean has_known_mtime,
                    struct timespec known_mtime)
{
  struct stat buf;

  if (stat (filename, &buf) == -1)
    /* file currently doesn't exist */
    return FALSE;

  if (!has_known_mtime)
    /* file has been created on disk */
    return TRUE;

  if (buf.st_mtim.tv_sec > known_mtime.tv_sec)
    /* disk seconds are more recent than known seconds */
    return TRUE;
  if (buf.st_mtim.tv_sec < known_mtime.tv_sec)
    /* disk seconds are older than known seconds (file went back in time?) */
    return FALSE;

  if (buf.st_mtim.tv_nsec > known_mtime.tv_nsec)
    /* disk nanoseconds are more recent than known nanoseconds */
    return TRUE;
  /* disk nanoseconds are equal or older than known nanoseconds */
  return FALSE;
}


/*! \brief Create a new page with a titleblock.
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


/*! \brief Show "File changed. Reread from disk?" dialog.
 */
static gint
x_dialog_reread_from_disk (GschemToplevel *w_current, PAGE *page)
{
  const gchar *fmt, *secondary_text;
  GtkWidget *dialog, *button;
  gint response_id;

  if (page->exists_on_disk) {
    fmt = _("The file \"%s\" has changed on disk."),
    secondary_text = page->CHANGED
      ? _("Do you want to drop your changes and reload the file?")
      : _("Do you want to reload it?");
  } else {
    fmt = _("The file \"%s\" has been created on disk."),
    secondary_text = page->CHANGED
      ? _("Do you want to drop your changes and load the file?")
      : _("Do you want to open it?");
  }

  dialog = gtk_message_dialog_new (GTK_WINDOW (w_current->main_window),
                                   GTK_DIALOG_MODAL |
                                     GTK_DIALOG_DESTROY_WITH_PARENT,
                                   GTK_MESSAGE_WARNING,
                                   GTK_BUTTONS_NONE,
                                   fmt, page->page_filename);
  g_object_set (dialog, "secondary-text", secondary_text, NULL);
  gtk_dialog_add_buttons (GTK_DIALOG (dialog),
                          GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                          NULL);
  button = gtk_button_new_with_mnemonic (page->CHANGED ? _("_Revert") :
                                         page->exists_on_disk ? _("_Reload") :
                                         _("_Open"));
  gtk_widget_set_can_default (button, TRUE);
  gtk_button_set_image (GTK_BUTTON (button),
                        gtk_image_new_from_stock (
                          page->CHANGED ? GTK_STOCK_REVERT_TO_SAVED :
                          page->exists_on_disk ? GTK_STOCK_REFRESH :
                          GTK_STOCK_OPEN,
                          GTK_ICON_SIZE_BUTTON));
  gtk_widget_show (button);
  gtk_dialog_add_action_widget (GTK_DIALOG (dialog), button,
                                GTK_RESPONSE_ACCEPT);
  gtk_dialog_set_alternative_button_order (GTK_DIALOG (dialog),
                                           GTK_RESPONSE_ACCEPT,
                                           GTK_RESPONSE_CANCEL,
                                           -1);
  gtk_dialog_set_default_response (GTK_DIALOG (dialog),
                                   page->CHANGED ? GTK_RESPONSE_CANCEL
                                                 : GTK_RESPONSE_ACCEPT);
  gtk_window_set_title (GTK_WINDOW (dialog), _("gschem"));
  response_id = gtk_dialog_run (GTK_DIALOG (dialog));
  gtk_widget_destroy (dialog);

  return response_id;
}


/*! \brief Helper function for \c x_highlevel_open_page(s).
 *
 * If there is a matching page in \a w_current, returns a pointer to
 * the existing page.  If the file exists, opens a new page from the
 * file.  Otherwise, asks the user for confirmation and, if confirmed,
 * creates a new page with that name.
 *
 * \param [in] w_current          the toplevel environment
 * \param [in] filename           the name of the file to open/create
 * \param [in] already_confirmed  whether the user has already
 *                                  confirmed creating the file
 *
 * \returns a pointer to the page, or \c NULL if the file couldn't be
 *          loaded or the user didn't confirm creating a page
 */
static PAGE *
open_or_create_page (GschemToplevel *w_current, const gchar *filename,
                     gboolean already_confirmed)
{
  TOPLEVEL *toplevel = gschem_toplevel_get_toplevel (w_current);
  PAGE *page;
  gchar *full_filename;

  /* If a page has already been created for this filename, return the
     existing page.
     The filename is intentionally not normalized here.  If the file
     doesn't exists, the normalized filename would be NULL; if it does
     exist, x_lowlevel_open_page() will take care of finding and
     returning the correct page. */
  page = s_page_search (toplevel, filename);
  if (page != NULL) {
    if (page->page_filename != NULL &&
        file_changed_since (page->page_filename,
                            page->exists_on_disk,
                            page->last_modified) &&
        x_dialog_reread_from_disk (w_current, page) == GTK_RESPONSE_ACCEPT) {
      x_lowlevel_revert_page (w_current, page);
      return s_page_search (toplevel, filename);
    }
    return page;
  }

  /* open page if file exists */
  full_filename = f_normalize_filename (filename, NULL);
  if (full_filename != NULL) {
    g_free (full_filename);
    return x_lowlevel_open_page (w_current, filename);
  }

  if (!already_confirmed &&
      !x_dialog_confirm_create (
        GTK_WINDOW (w_current->main_window),
        _("Couldn't find \"%s\".\n\n"
          "Do you want to create a new file with this name?"),
        filename))
    return NULL;

  return x_lowlevel_new_page (w_current, filename);
}


/*! \brief Open a new page from a file, or find an existing page.
 *
 * Creates a new page and loads the file in it.  If there is already a
 * matching page in \a w_current, returns a pointer to the existing
 * page instead.
 *
 * The page becomes the new current page of \a w_current.  If there
 * was exactly one untitled, unchanged page before this operation, the
 * existing page is closed.
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
  TOPLEVEL *toplevel = gschem_toplevel_get_toplevel (w_current);
  GList *pages = geda_list_get_glist (toplevel->pages);
  PAGE *sole_page = g_list_length (pages) == 1 ? (PAGE *) pages->data : NULL;

  PAGE *page = open_or_create_page (w_current, filename, FALSE);

  if (sole_page != NULL && page != NULL && page != sole_page &&
      g_list_find (geda_list_get_glist (toplevel->pages), sole_page) != NULL &&
      sole_page->is_untitled && !sole_page->CHANGED)
    x_lowlevel_close_page (w_current, sole_page);

  if (page != NULL)
    x_window_set_current_page (w_current, page);

  return page;
}


/*! \brief Open multiple pages from files.
 *
 * For each file specified in \a filenames that is not already opened,
 * creates a new page and loads the file in it.
 *
 * The first page that could be opened or already existed becomes the
 * new current page of \a w_current.  If there was exactly one
 * untitled, unchanged page before this operation, the existing page
 * is closed.
 *
 * \param [in] w_current          the toplevel environment
 * \param [in] filenames          a GSList of filenames to open
 * \param [in] already_confirmed  whether the user has already
 *                                  confirmed creating the file(s)
 *
 * \returns \c TRUE if all files could be opened, \c FALSE otherwise
 */
gboolean
x_highlevel_open_pages (GschemToplevel *w_current, GSList *filenames,
                        gboolean already_confirmed)
{
  TOPLEVEL *toplevel = gschem_toplevel_get_toplevel (w_current);
  GList *pages = geda_list_get_glist (toplevel->pages);
  PAGE *sole_page = g_list_length (pages) == 1 ? (PAGE *) pages->data : NULL;

  PAGE *first_page = NULL;
  gboolean success = TRUE;

  /* open each file */
  for (GSList *l = filenames; l != NULL; l = l->next) {
    PAGE *page = open_or_create_page (w_current, (gchar *) l->data,
                                      already_confirmed);
    if (page == NULL)
      success = FALSE;
    else if (first_page == NULL)
      first_page = page;
  }

  if (sole_page != NULL && first_page != NULL && first_page != sole_page &&
      g_list_find (geda_list_get_glist (toplevel->pages), sole_page) != NULL &&
      sole_page->is_untitled && !sole_page->CHANGED)
    x_lowlevel_close_page (w_current, sole_page);

  if (first_page != NULL)
    x_window_set_current_page (w_current, first_page);

  return success;
}


/*! \brief Show "File changed. Save anyway?" dialog.
 */
static gint
x_dialog_save_anyway (GschemToplevel *w_current, PAGE *page)
{
  gchar *basename;
  GtkWidget *dialog;
  gint response_id;

  basename = g_path_get_basename (page->page_filename);
  dialog = gtk_message_dialog_new (GTK_WINDOW (w_current->main_window),
                                   GTK_DIALOG_MODAL |
                                     GTK_DIALOG_DESTROY_WITH_PARENT,
                                   GTK_MESSAGE_WARNING,
                                   GTK_BUTTONS_NONE,
                                   _("\"%s\" changed since visited or "
                                     "saved."),
                                   basename);
  g_free (basename);
  g_object_set (dialog, "secondary-text", _("Save anyway?"), NULL);
  gtk_dialog_add_buttons (GTK_DIALOG (dialog),
                          GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                          GTK_STOCK_SAVE, GTK_RESPONSE_ACCEPT,
                          NULL);
  gtk_dialog_set_alternative_button_order (GTK_DIALOG (dialog),
                                           GTK_RESPONSE_ACCEPT,
                                           GTK_RESPONSE_CANCEL,
                                           -1);
  gtk_dialog_set_default_response (GTK_DIALOG (dialog), GTK_RESPONSE_CANCEL);
  gtk_window_set_title (GTK_WINDOW (dialog), _("gschem"));
  response_id = gtk_dialog_run (GTK_DIALOG (dialog));
  gtk_widget_destroy (dialog);

  return response_id;
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
    x_window_present (w_current);

    return x_fileselect_save (w_current);
  }

  if (page->page_filename != NULL &&
      file_changed_since (page->page_filename,
                          page->exists_on_disk,
                          page->last_modified) &&
      x_dialog_save_anyway (w_current, page) != GTK_RESPONSE_ACCEPT)
    return FALSE;

  return x_lowlevel_save_page (w_current, page, page->page_filename);
}


/*! \brief Save all changed pages.
 *
 * For untitled pages, a file chooser dialog is shown.
 *
 * \param [in] w_current  the toplevel environment
 *
 * \returns \c TRUE if all changed pages were saved, \c FALSE otherwise
 */
gboolean
x_highlevel_save_all (GschemToplevel *w_current)
{
  TOPLEVEL *toplevel = gschem_toplevel_get_toplevel (w_current);
  gboolean saved_any = FALSE, success = TRUE;

  for (const GList *l = geda_list_get_glist (toplevel->pages);
       l != NULL; l = l->next) {
    PAGE *page = (PAGE *) l->data;
    if (!page->CHANGED)
      /* ignore unchanged pages */
      continue;

    if (x_highlevel_save_page (w_current, page))
      saved_any = TRUE;
    else
      success = FALSE;
  }

  if (!success)
    i_set_state_msg (w_current, SELECT, _("Failed to Save All"));
  else if (saved_any)
    i_set_state_msg (w_current, SELECT, _("Saved All"));
  else
    i_set_state_msg (w_current, SELECT, _("No files need saving"));

  return success;
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

  if (page->is_untitled)
    /* can't revert untitled page */
    return FALSE;

  if (page->CHANGED) {
    GtkWidget *dialog;
    int response;

    x_window_set_current_page (w_current, page);
    x_window_present (w_current);

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
       page in the background) but doesn't hurt, either. */
    x_window_set_current_page (w_current, page);
    x_window_present (w_current);

    if (!x_dialog_close_changed_page (w_current, page))
      return FALSE;
  }

  x_lowlevel_close_page (w_current, page);
  return TRUE;
}
