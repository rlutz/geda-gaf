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

/*! \file x_lowlevel.c
 * \brief Low-level page-related functions.
 *
 * These functions don't usually interact with the user.  In
 * particular, they don't switch pages and don't ask for confirmation
 * before carrying out potentially destructive actions.  However, they
 * *do* warn if an error occurred during an operation.
 *
 * For user-interface actions, the functions in x_highlevel.c should
 * be used instead.
 */
#include <config.h>

#include "gschem.h"
#include "actions.decl.x"


/*! \brief Create a new untitled page with a titleblock.
 *
 * If \a filename is \c NULL, the name of the new page is build from
 * configuration data ('untitled-name') and a counter for uniqueness.
 *
 * This function doesn't change the current page of \a w_current.
 *
 * \param [in] w_current  the toplevel environment
 * \param [in] filename   the filename for the new page, or \c NULL to
 *                        generate an untitled filename
 *
 * \returns a pointer to the new page, or \c NULL if a file with the
 *          specified filename already exists
 */
PAGE *
x_lowlevel_new_page (GschemToplevel *w_current, const gchar *filename)
{
  PAGE *page;
  gchar *fn;

  TOPLEVEL *toplevel = gschem_toplevel_get_toplevel (w_current);
  g_return_val_if_fail (toplevel != NULL, NULL);

  if (filename == NULL) {
    /* Generate untitled filename if none was specified */
    gchar *cwd, *tmp, *untitled_name;
    EdaConfig *cfg;
    cwd = g_get_current_dir ();
    cfg = eda_config_get_context_for_path (cwd);
    untitled_name = eda_config_get_string (cfg, "gschem", "default-filename",
                                           NULL);
    fn = NULL;
    do {
      g_free (fn);
      tmp = g_strdup_printf ("%s_%d.sch", untitled_name,
                             ++w_current->num_untitled);
      fn = g_build_filename (cwd, tmp, NULL);
      g_free (tmp);
    } while (g_file_test (fn, G_FILE_TEST_EXISTS));
    g_free (untitled_name);
    g_free (cwd);
  } else {
    if (g_file_test (filename, G_FILE_TEST_EXISTS)) {
      GtkWidget *dialog = gtk_message_dialog_new_with_markup (
        GTK_WINDOW (w_current->main_window),
        GTK_DIALOG_DESTROY_WITH_PARENT,
        GTK_MESSAGE_ERROR,
        GTK_BUTTONS_CLOSE,
        _("<b>Can't create \"%s\".</b>"),
        filename);
      g_object_set (dialog, "secondary-text",
                    _("A file with this name already exists."), NULL);
      gtk_window_set_title (GTK_WINDOW (dialog), _("gschem"));
      gtk_dialog_run (GTK_DIALOG (dialog));
      gtk_widget_destroy (dialog);
      return NULL;
    }
    fn = g_strdup (filename);
  }

  PAGE *saved_page = toplevel->page_current;

  page = s_page_new (toplevel, fn);
  page->is_untitled = filename == NULL;
  s_page_goto (toplevel, page);
  gschem_toplevel_page_changed (w_current);

  /* print a message */
  if (!quiet_mode)
    s_log_message (_("New file [%s]\n"),
                   toplevel->page_current->page_filename);

  g_run_hook_page (w_current, "%new-page-hook", toplevel->page_current);

  o_undo_savestate (w_current, toplevel->page_current, UNDO_ALL, NULL);

  g_free (fn);

  if (saved_page != NULL) {
    s_page_goto (toplevel, saved_page);
    gschem_toplevel_page_changed (w_current);
  }

  x_pagesel_update (w_current);
  return page;
}


/*! \brief Open a new page from a file, or find an existing page.
 *
 * Creates a new page and loads the file in it.  If there is already a
 * matching page in \a w_current, returns a pointer to the existing
 * page instead.
 *
 * This function doesn't change the current page of \a w_current.
 *
 * \param [in] w_current  the toplevel environment
 * \param [in] filename   the name of the file to open
 *
 * \returns a pointer to the page, or \c NULL if the file couldn't be
 *          loaded
 */
PAGE *
x_lowlevel_open_page (GschemToplevel *w_current, const gchar *filename)
{
  TOPLEVEL *toplevel = gschem_toplevel_get_toplevel (w_current);
  g_return_val_if_fail (toplevel != NULL, NULL);
  g_return_val_if_fail (filename != NULL, NULL);

  gchar *full_filename;
  gchar *furi;

  PAGE *page;

  /* Return existing page if it is already loaded */
  full_filename = f_normalize_filename (filename, NULL);
  page = s_page_search (toplevel, full_filename);
  g_free (full_filename);
  if (page != NULL) {
    gtk_recent_manager_add_item (
      recent_manager, g_filename_to_uri (page->page_filename, NULL, NULL));
    return page;
  }

  PAGE *saved_page = toplevel->page_current;

  page = s_page_new (toplevel, filename);
  s_page_goto (toplevel, page);
  gschem_toplevel_page_changed (w_current);

  /* Load from file */
  GError *err = NULL;
  if (!quiet_mode)
    s_log_message (_("Loading schematic [%s]\n"), filename);

  if (!f_open (toplevel, page, (gchar *) filename, &err)) {
    GtkWidget *dialog;

    g_warning ("%s\n", err->message);
    dialog = gtk_message_dialog_new_with_markup (
      GTK_WINDOW (w_current->main_window),
      GTK_DIALOG_DESTROY_WITH_PARENT,
      GTK_MESSAGE_ERROR,
      GTK_BUTTONS_CLOSE,
      _("<b>An error occurred while loading the requested file.</b>\n\n"
        "Loading from '%s' failed: %s. "
        "The gschem log may contain more information."),
      page->page_filename, err->message);
    gtk_window_set_title (GTK_WINDOW (dialog), _("Failed to load file"));

    s_page_delete (toplevel, page);
    if (saved_page != NULL)
      s_page_goto (toplevel, saved_page);
    gschem_toplevel_page_changed (w_current);

    gtk_dialog_run (GTK_DIALOG (dialog));
    gtk_widget_destroy (dialog);
    g_error_free (err);

    return NULL;
  }

  furi = g_filename_to_uri (page->page_filename, NULL, NULL);
  gtk_recent_manager_add_item (recent_manager, furi);
  g_free (furi);

  o_undo_savestate (w_current, toplevel->page_current, UNDO_ALL, NULL);

  if (saved_page != NULL) {
    s_page_goto (toplevel, saved_page);
    gschem_toplevel_page_changed (w_current);
  }

  x_pagesel_update (w_current);
  return page;
}


/*! \brief Save a page to a given filename.
 *
 * \a page doesn't have to be the current page of \a w_current.
 * This function doesn't change the current page of \a w_current.
 *
 * If \a filename is different from the current filename of \a page,
 * the page's filename is updated.
 *
 * \param [in] w_current  the toplevel environment
 * \param [in] page       the page to save
 * \param [in] filename   the name of the file to which to save the page
 *
 * \returns \c TRUE if the page could be saved, \c FALSE otherwise
 */
gboolean
x_lowlevel_save_page (GschemToplevel *w_current, PAGE *page,
                      const gchar *filename)
{
  TOPLEVEL *toplevel = gschem_toplevel_get_toplevel (w_current);
  const gchar *log_msg, *state_msg;
  gboolean success;
  GError *err = NULL;

  g_return_val_if_fail (toplevel != NULL, FALSE);
  g_return_val_if_fail (page     != NULL, FALSE);
  g_return_val_if_fail (filename != NULL, FALSE);

  /* try saving page to filename */
  success = f_save (toplevel, page, filename, &err) == 1;

  if (!success) {
    log_msg = _("Could NOT save page [%s]\n");
    state_msg = _("Error while trying to save");

    GtkWidget *dialog;

    dialog = gtk_message_dialog_new (GTK_WINDOW (w_current->main_window),
                                     GTK_DIALOG_DESTROY_WITH_PARENT,
                                     GTK_MESSAGE_ERROR,
                                     GTK_BUTTONS_CLOSE,
                                     "%s",
                                     err->message);
    gtk_window_set_title (GTK_WINDOW (dialog), _("Failed to save file"));
    gtk_dialog_run (GTK_DIALOG (dialog));
    gtk_widget_destroy (dialog);
    g_clear_error (&err);
  } else {
    /* successful save of page to file, update page... */
    /* change page name if necessary and prepare log message */
    if (g_ascii_strcasecmp (page->page_filename, filename) != 0) {
      g_free (page->page_filename);
      page->page_filename = g_strdup (filename);

      log_msg = _("Saved as [%s]\n");
    } else
      log_msg = _("Saved [%s]\n");

    state_msg = _("Saved");

    page->is_untitled = FALSE;

    /* reset page CHANGED flag */
    page->CHANGED = 0;

    /* add to recent file list */
    gtk_recent_manager_add_item (recent_manager,
                                 g_filename_to_uri (filename, NULL, NULL));

    /* i_set_filename (w_current, page->page_filename); */
    x_pagesel_update (w_current);

    if (page == gschem_toplevel_get_toplevel (w_current)->page_current) {
      gschem_action_set_sensitive (action_page_revert, TRUE, w_current);
      /* if the file change notification bar was visible, hide it */
      x_window_update_file_change_notification (w_current, page);
    }
  }

  /* log status of operation */
  s_log_message (log_msg, filename);

  i_set_state_msg (w_current, SELECT, state_msg);

  return success;
}


/*! \brief Revert a page, no questions asked.
 *
 * Closes the page, creates a new page, and reads the file back from
 * disk.  If the reverted page was the current page before, switches
 * to the newly created page.
 *
 * Does not ask for confirmation.  If the user should be asked for
 * confirmation, use \ref x_highlevel_revert_page instead.
 *
 * \param [in] w_current  the toplevel environment
 * \param [in] page       the page to revert
 *
 * \returns \c TRUE if the page was successfully reloaded, \c FALSE otherwise
 */
gboolean
x_lowlevel_revert_page (GschemToplevel *w_current, PAGE *page)
{
  TOPLEVEL *toplevel = gschem_toplevel_get_toplevel (w_current);
  g_return_val_if_fail (toplevel != NULL, FALSE);
  g_return_val_if_fail (page != NULL,     FALSE);
  g_return_val_if_fail (page->pid != -1,  FALSE);

  if (page->is_untitled)
    /* can't revert untitled page */
    return FALSE;

  /* If we're reverting whilst inside an action, re-wind the
     page contents back to their state before we started */
  if (w_current->inside_action)
    i_cancel (w_current);

  /* save this for later */
  gchar *filename = g_strdup (page->page_filename);
  int page_control = page->page_control;
  int up = page->up;
  gchar *patch_filename = g_strdup (page->patch_filename);
  gboolean patch_descend = page->patch_descend;
  gboolean was_current_page = toplevel->page_current == page;

  /* delete the page, then re-open the file as a new page */
  s_log_message (page->CHANGED ? _("Discarding page [%s]\n")
                               : _("Closing [%s]\n"),
                 page->page_filename);
  s_page_delete (toplevel, page);
  gschem_toplevel_page_changed (w_current);

  /* Force symbols to be re-loaded from disk */
  s_clib_refresh ();

  page = x_lowlevel_open_page (w_current, filename);
  g_free (filename);

  if (page == NULL) {
    /* don't leave without a current page set */
    if (toplevel->page_current == NULL) {
      GList *pages = geda_list_get_glist (toplevel->pages);
      if (pages != NULL)
        page = (PAGE *) pages->data;

      /* create a new page if we closed the last one */
      if (page == NULL)
        page = x_lowlevel_new_page (w_current, NULL);

      x_window_set_current_page (w_current, page);
    }

    /* x_lowlevel_open_page has already displayed an error message */
    return FALSE;
  }

  /* make sure we maintain the hierarchy info */
  page->page_control = page_control;
  page->up = up;
  g_free (page->patch_filename);
  page->patch_filename = patch_filename;
  page->patch_descend = patch_descend;

  if (was_current_page)
    x_window_set_current_page (w_current, page);

  x_pagesel_update (w_current);
  return TRUE;
}


/*! \brief Close a page, no questions asked.
 *
 * Switches to the next valid page if necessary.  If this was the last
 * page of the toplevel, a new untitled page is created.
 *
 * Does not ask for confirmation.  If the user should be asked for
 * confirmation, use \ref x_highlevel_close_page instead.
 *
 * \param [in] w_current  the toplevel environment
 * \param [in] page       the page to close
 */
void
x_lowlevel_close_page (GschemToplevel *w_current, PAGE *page)
{
  TOPLEVEL *toplevel = gschem_toplevel_get_toplevel (w_current);
  PAGE *new_current = NULL;
  GList *iter;

  g_return_if_fail (toplevel != NULL);
  g_return_if_fail (page != NULL);
  g_return_if_fail (page->pid != -1);

  /* If we're closing whilst inside an action, re-wind the
   * page contents back to their state before we started */
  if (w_current->inside_action)
    i_cancel (w_current);

  if (page == toplevel->page_current) {
    /* as it will delete current page, select new current page */
    /* first look up in page hierarchy */
    new_current = s_page_search_by_page_id (toplevel->pages, page->up);

    if (new_current == NULL) {
      /* no up in hierarchy, choice is prev, next, new page */
      iter = g_list_find (geda_list_get_glist (toplevel->pages), page);

      if (g_list_previous (iter))
        new_current = (PAGE *) g_list_previous (iter)->data;
      else if (g_list_next (iter))
        new_current = (PAGE *) g_list_next (iter)->data;
      else
        /* need to add a new untitled page */
        new_current = NULL;
    }
    /* new_current will be the new current page at the end of the function */
  }

  s_log_message (page->CHANGED ? _("Discarding page [%s]\n")
                               : _("Closing [%s]\n"),
                 page->page_filename);
  /* remove page from toplevel list of page and free */
  s_page_delete (toplevel, page);
  gschem_toplevel_page_changed (w_current);

  /* Switch to a different page if we just removed the current */
  if (toplevel->page_current == NULL) {
    if (new_current == NULL)
      /* Create a new page if there wasn't another to switch to */
      new_current = x_lowlevel_new_page (w_current, NULL);

    /* change to new_current and update display */
    x_window_set_current_page (w_current, new_current);
  }

  x_pagesel_update (w_current);
}
