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

#include "gschem.h"


/*! \brief Creates filter for file chooser.
 *  \par Function Description
 *  This function adds file filters to <B>filechooser</B>.
 *
 *  \param [in] filechooser The file chooser to add filter to.
 */
static void
x_fileselect_setup_filechooser_filters (GtkFileChooser *filechooser)
{
  GtkFileFilter *filter;

  /* file filter for schematic files (*.sch) */
  filter = gtk_file_filter_new ();
  gtk_file_filter_set_name (filter, _("Schematics"));
  gtk_file_filter_add_pattern (filter, "*.sch");
  gtk_file_chooser_add_filter (filechooser, filter);
  /* file filter for symbol files (*.sym) */
  filter = gtk_file_filter_new ();
  gtk_file_filter_set_name (filter, _("Symbols"));
  gtk_file_filter_add_pattern (filter, "*.sym");
  gtk_file_chooser_add_filter (filechooser, filter);
  /* file filter for both symbol and schematic files (*.sym+*.sch) */
  filter = gtk_file_filter_new ();
  gtk_file_filter_set_name (filter, _("Schematics and symbols"));
  gtk_file_filter_add_pattern (filter, "*.sym");
  gtk_file_filter_add_pattern (filter, "*.sch");
  gtk_file_chooser_add_filter (filechooser, filter);
  gtk_file_chooser_set_filter (filechooser, filter);
  /* file filter that match any file */
  filter = gtk_file_filter_new ();
  gtk_file_filter_set_name (filter, _("All files"));
  gtk_file_filter_add_pattern (filter, "*");
  gtk_file_chooser_add_filter (filechooser, filter);

}

/*! \brief Updates the preview when the selection changes.
 *  \par Function Description
 *  This is the callback function connected to the 'update-preview'
 *  signal of the <B>GtkFileChooser</B>.
 *
 *  It updates the preview widget with the name of the newly selected
 *  file.
 *
 *  \param [in] chooser   The file chooser to add the preview to.
 *  \param [in] user_data A pointer on the preview widget.
 */
static void
x_fileselect_callback_update_preview (GtkFileChooser *chooser,
                                      gpointer user_data)
{
  GschemPreview *preview = GSCHEM_PREVIEW (user_data);
  gchar *filename, *preview_filename = NULL;

  filename = gtk_file_chooser_get_preview_filename (chooser);
  if (filename != NULL &&
      !g_file_test (filename, G_FILE_TEST_IS_DIR)) {
    preview_filename = filename;
  }

  /* update preview */
  g_object_set (preview,
                "width-request",  160,
                "height-request", 120,
                "filename", preview_filename,
                "active", (preview_filename != NULL),
                NULL);

  g_free (filename);
}

/*! \brief Adds a preview to a file chooser.
 *  \par Function Description
 *  This function adds a preview section to the stock
 *  <B>GtkFileChooser</B>.
 *
 *  The <B>Preview</B> object is inserted in a frame and alignment
 *  widget for accurate positionning.
 *
 *  Other widgets can be added to this preview area for example to
 *  enable/disable the preview. Currently, the preview is always
 *  active.
 *
 *  Function <B>x_fileselect_callback_update_preview()</B> is
 *  connected to the signal 'update-preview' of <B>GtkFileChooser</B>
 *  so that it redraws the preview area every time a new file is
 *  selected.
 *
 *  \param [in] filechooser The file chooser to add the preview to.
 */
static void
x_fileselect_add_preview (GtkFileChooser *filechooser)
{
  GtkWidget *alignment, *frame, *preview;

  frame = GTK_WIDGET (g_object_new (GTK_TYPE_FRAME,
                                    "label", _("Preview"),
                                    NULL));
  alignment = GTK_WIDGET (g_object_new (GTK_TYPE_ALIGNMENT,
                                        "right-padding", 5,
                                        "left-padding", 5,
                                        "xscale", 0.0,
                                        "yscale", 0.0,
                                        "xalign", 0.5,
                                        "yalign", 0.5,
                                        NULL));

  preview = gschem_preview_new ();

  gtk_container_add (GTK_CONTAINER (alignment), preview);
  gtk_container_add (GTK_CONTAINER (frame), alignment);
  gtk_widget_show_all (frame);

  g_object_set (filechooser,
                /* GtkFileChooser */
                "use-preview-label", FALSE,
                "preview-widget", frame,
                NULL);

  /* connect callback to update preview */
  g_signal_connect (filechooser,
                    "update-preview",
                    G_CALLBACK (x_fileselect_callback_update_preview),
                    preview);

}

/*! \brief Opens a file chooser for creating a file.
 *
 * Opens a file chooser dialog and lets the user select a filename.
 * If the dialog is confirmed, creates a page with this name.
 *
 * \param [in] w_current  the toplevel environment
 * \param [in] dirname    the directory in which to create the file,
 *                          or \c NULL to create the file in the
 *                          current page's directory
 * \param [in] basename   the basename of the file to create, or
 *                          \c NULL to not preset a name
 *
 * \returns the newly created page, or \c NULL of the dialog was
 *          cancelled
 */
PAGE *
x_fileselect_create (GschemToplevel *w_current, const gchar *dirname,
                                                const gchar *basename)
{
  gchar *lowercase_basename;
  const gchar *title;
  GtkWidget *dialog, *button;
  PAGE *page = NULL;

  lowercase_basename = g_ascii_strdown (basename, -1);
  if (g_str_has_suffix (lowercase_basename, ".sch"))
    title = _("Create schematic...");
  else if (g_str_has_suffix (lowercase_basename, ".sym"))
    title = _("Create symbol...");
  else
    title = _("Create file...");
  g_free (lowercase_basename);

  dialog = gtk_file_chooser_dialog_new (title,
                                        GTK_WINDOW (w_current->main_window),
                                        GTK_FILE_CHOOSER_ACTION_SAVE,
                                        GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                                        NULL);
  button = gtk_button_new_with_mnemonic (_("_Create"));
  gtk_widget_set_can_default (button, TRUE);
  gtk_button_set_image (GTK_BUTTON (button),
                        gtk_image_new_from_stock (GTK_STOCK_NEW,
                                                  GTK_ICON_SIZE_BUTTON));
  gtk_widget_show (button);
  gtk_dialog_add_action_widget (GTK_DIALOG (dialog), button,
                                GTK_RESPONSE_ACCEPT);
  gtk_dialog_set_alternative_button_order (GTK_DIALOG(dialog),
                                           GTK_RESPONSE_ACCEPT,
                                           GTK_RESPONSE_CANCEL,
                                           -1);
  gtk_dialog_set_default_response (GTK_DIALOG (dialog), GTK_RESPONSE_ACCEPT);

  //x_fileselect_add_preview (GTK_FILE_CHOOSER (dialog));
  x_fileselect_setup_filechooser_filters (GTK_FILE_CHOOSER (dialog));

  if (dirname != NULL)
    gtk_file_chooser_set_current_folder (GTK_FILE_CHOOSER (dialog), dirname);
  else {
    TOPLEVEL *toplevel = gschem_toplevel_get_toplevel (w_current);
    gchar *cwd;
    if (toplevel->page_current == NULL || toplevel->page_current->is_untitled)
      cwd = g_get_current_dir ();
    else
      cwd = g_path_get_dirname (toplevel->page_current->page_filename);
    gtk_file_chooser_set_current_folder (GTK_FILE_CHOOSER (dialog), cwd);
    g_free (cwd);
  }
  gtk_file_chooser_set_current_name (GTK_FILE_CHOOSER (dialog), basename);

  gtk_file_chooser_set_do_overwrite_confirmation (GTK_FILE_CHOOSER (dialog),
                                                  TRUE);

  if (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_ACCEPT) {
    gchar *fn = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (dialog));
    page = x_lowlevel_new_page (w_current, fn);
    g_free (fn);
  }

  gtk_widget_destroy (dialog);
  return page;
}

/*! \brief Response callback for file open dialog.
 *
 * If the user confirmed the dialog, checks for each filename in turn
 * whether it exists and if not, asks the user whether to create that
 * file.  If the user cancels creating a file, returns to the file
 * chooser dialog.
 *
 * Doesn't check anything if the user cancelled the dialog.
 */
static void
confirm_create (GtkDialog *dialog, gint response_id, gpointer user_data)
{
  GschemToplevel *w_current = GSCHEM_TOPLEVEL (user_data);
  TOPLEVEL *toplevel = gschem_toplevel_get_toplevel (w_current);
  GSList *filenames;

  if (response_id != GTK_RESPONSE_ACCEPT)
    return;

  filenames = gtk_file_chooser_get_filenames (GTK_FILE_CHOOSER (dialog));

  for (GSList *l = filenames; l != NULL; l = l->next) {
    gchar *filename = (gchar *) l->data;
    if (!g_file_test (filename, G_FILE_TEST_EXISTS) &&
        s_page_search (toplevel, filename) == NULL &&
        !x_dialog_confirm_create (
          GTK_WINDOW (dialog),
          _("The file \"%s\" doesn't exist.\n\n"
            "Do you want to create it?"),
          filename)) {
      g_signal_stop_emission_by_name (dialog, "response");
      break;
    }
  }

  g_slist_free_full (filenames, g_free);
}

/*! \brief Opens a file chooser for opening one or more schematics.
 *  \par Function Description
 *  This function opens a file chooser dialog and wait for the user to
 *  select at least one file to load as <B>w_current</B>'s new pages.
 *
 *  The function updates the user interface.
 *
 *  At the end of the function, the w_current->toplevel's current page
 *  is set to the page of the last loaded page.
 *
 *  \param [in] w_current The GschemToplevel environment.
 */
void
x_fileselect_open(GschemToplevel *w_current)
{
  GtkWidget *dialog;
  PAGE *page_current;
  gchar *cwd;
  GSList *filenames = NULL;

  dialog = gtk_file_chooser_dialog_new (_("Open..."),
                                        GTK_WINDOW(w_current->main_window),
                                        GTK_FILE_CHOOSER_ACTION_OPEN,
                                        GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                                        GTK_STOCK_OPEN,   GTK_RESPONSE_ACCEPT,
                                        NULL);

  /* Set the alternative button order (ok, cancel, help) for other systems */
  gtk_dialog_set_alternative_button_order(GTK_DIALOG(dialog),
					  GTK_RESPONSE_ACCEPT,
					  GTK_RESPONSE_CANCEL,
					  -1);

  x_fileselect_add_preview (GTK_FILE_CHOOSER (dialog));
  g_object_set (dialog,
                /* GtkFileChooser */
                "select-multiple", TRUE,
                NULL);
  /* add file filters to dialog */
  x_fileselect_setup_filechooser_filters (GTK_FILE_CHOOSER (dialog));
  /* force start in current working directory, not in 'Recently Used' */
  page_current = gschem_toplevel_get_toplevel (w_current)->page_current;
  if (page_current == NULL || page_current->is_untitled)
    cwd = g_get_current_dir ();
  else
    cwd = g_path_get_dirname (page_current->page_filename);
  gtk_file_chooser_set_current_folder (GTK_FILE_CHOOSER (dialog), cwd);
  g_free (cwd);

  /* ask for confirmation if selecting a non-existent file */
  g_signal_connect (dialog, "response",
                    G_CALLBACK (confirm_create), w_current);

  gtk_widget_show (dialog);
  if (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_ACCEPT)
    filenames = gtk_file_chooser_get_filenames (GTK_FILE_CHOOSER (dialog));
  gtk_widget_destroy (dialog);

  x_highlevel_open_pages (w_current, filenames, TRUE);

  /* free the list of filenames */
  g_slist_free_full (filenames, g_free);
}

/*! \brief Show a dialog asking whether to really use a non-canonical
 *         filename.
 *
 * \param [in] parent    the transient parent window
 * \param [in] basename  the selected non-canonical filename
 *
 * \returns whether the user confirmed using the filename
 */
static gboolean
run_extension_confirmation_dialog (GtkWindow *parent, const char *basename)
{
  GtkWidget *dialog = gtk_message_dialog_new (
    parent,
    GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
    GTK_MESSAGE_QUESTION,
    GTK_BUTTONS_NONE,
    _("The selected filename \"%s\" doesn't have a valid gEDA filename "
      "extension (\".sch\" for schematics or \".sym\" for symbols).\n\n"
      "Do you want to save as \"%s\" anyway?"),
    basename, basename);
  gtk_dialog_add_buttons (GTK_DIALOG (dialog),
                          GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                          GTK_STOCK_SAVE, GTK_RESPONSE_ACCEPT,
                          NULL);
  gtk_dialog_set_alternative_button_order (GTK_DIALOG (dialog),
                                           GTK_RESPONSE_ACCEPT,
                                           GTK_RESPONSE_CANCEL,
                                           -1);
  gtk_dialog_set_default_response (GTK_DIALOG (dialog), GTK_RESPONSE_ACCEPT);
  if (gtk_window_has_group (parent))
    gtk_window_group_add_window (gtk_window_get_group (parent),
                                 GTK_WINDOW (dialog));
  gtk_window_set_title (GTK_WINDOW (dialog), _("Confirm filename"));

  gint response_id = gtk_dialog_run (GTK_DIALOG (dialog));
  gtk_widget_destroy (dialog);
  return response_id == GTK_RESPONSE_ACCEPT;
}

/*! \brief Response callback for file save dialog.
 *
 * If the user confirmed the dialog and the selected filename doesn't
 * have \c ".sch" or \c ".sym" as an extention, asks whether to really
 * use that filename.  On cancel, returns to the file chooser dialog.
 *
 * Doesn't check anything if the user cancelled the file chooser dialog.
 */
static void
check_extension (GtkDialog *dialog, gint response_id, gpointer user_data)
{
  if (response_id != GTK_RESPONSE_ACCEPT)
    return;

  gchar *filename = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (dialog));
  gchar *lowercase_filename = g_ascii_strdown (filename, -1);

  if (!g_str_has_suffix (lowercase_filename, ".sch") &&
      !g_str_has_suffix (lowercase_filename, ".sym")) {
    gchar *basename = g_path_get_basename (filename);
    if (!run_extension_confirmation_dialog (GTK_WINDOW (dialog), basename))
      g_signal_stop_emission_by_name (dialog, "response");
    g_free (basename);
  }

  g_free (lowercase_filename);
  g_free (filename);
}

/*! \brief Opens a file chooser for saving the current page.
 *  \par Function Description
 *  This function opens a file chooser dialog and wait for the user to
 *  select a file where the <B>toplevel</B>'s current page will be
 *  saved.
 *
 *  If the user cancels the operation (with the cancel button), the
 *  page is not saved.
 *
 *  The function updates the user interface.
 *
 *  \param [in] w_current The GschemToplevel environment.
 *
 *  \returns \c TRUE if the file was saved, \c FALSE otherwise
 */
gboolean
x_fileselect_save (GschemToplevel *w_current)
{
  TOPLEVEL *toplevel = gschem_toplevel_get_toplevel (w_current);
  GtkWidget *dialog;
  gboolean success = FALSE;

  dialog = gtk_file_chooser_dialog_new (_("Save as..."),
                                        GTK_WINDOW(w_current->main_window),
                                        GTK_FILE_CHOOSER_ACTION_SAVE,
                                        GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                                        GTK_STOCK_SAVE,   GTK_RESPONSE_ACCEPT,
                                        NULL);

  /* Set the alternative button order (ok, cancel, help) for other systems */
  gtk_dialog_set_alternative_button_order(GTK_DIALOG(dialog),
					  GTK_RESPONSE_ACCEPT,
					  GTK_RESPONSE_CANCEL,
					  -1);

  /* set default response signal. This is usually triggered by the
     "Return" key */
  gtk_dialog_set_default_response(GTK_DIALOG(dialog),
				  GTK_RESPONSE_ACCEPT);

  g_object_set (dialog,
                /* GtkFileChooser */
                "select-multiple", FALSE,
                /* only in GTK 2.8 */
                /* "do-overwrite-confirmation", TRUE, */
                NULL);
  /* add file filters to dialog */
  x_fileselect_setup_filechooser_filters (GTK_FILE_CHOOSER (dialog));
  /* set the current filename or directory name if new document */
  if (toplevel->page_current->is_untitled == FALSE &&
      toplevel->page_current->page_filename != NULL) {
    if (g_file_test (toplevel->page_current->page_filename,
                     G_FILE_TEST_EXISTS))
      gtk_file_chooser_set_filename (GTK_FILE_CHOOSER (dialog),
                                     toplevel->page_current->page_filename);
    else {
      gchar *str;
      str = g_path_get_dirname (toplevel->page_current->page_filename);
      gtk_file_chooser_set_current_folder (GTK_FILE_CHOOSER (dialog), str);
      g_free (str);
      str = g_path_get_basename (toplevel->page_current->page_filename);
      gtk_file_chooser_set_current_name (GTK_FILE_CHOOSER (dialog), str);
      g_free (str);
    }
  } else {
    gchar *cwd = g_get_current_dir ();
    /* force save in current working dir */
    gtk_file_chooser_set_current_folder (GTK_FILE_CHOOSER (dialog), cwd);
    g_free (cwd);
    gtk_file_chooser_set_current_name (GTK_FILE_CHOOSER (dialog),
                                       "untitled.sch");
  }

  /* use built-in overwrite confirmation dialog */
  gtk_file_chooser_set_do_overwrite_confirmation (GTK_FILE_CHOOSER (dialog),
                                                  TRUE);

  /* ask for confirmation if the user chooses an unusual filename */
  g_signal_connect (dialog, "response", G_CALLBACK (check_extension), NULL);

  gtk_widget_show (dialog);
  if (gtk_dialog_run ((GtkDialog*)dialog) == GTK_RESPONSE_ACCEPT) {
    gchar *filename =
      gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (dialog));

    /* try saving current page of toplevel to file filename */
    success = x_lowlevel_save_page (w_current,
                                    w_current->toplevel->page_current,
                                    filename);

    g_free (filename);
  }
  gtk_widget_destroy (dialog);
  return success;
}

/*! \brief Load/Backup selection dialog.
 *  \par Function Description
 *  This function opens a message dialog and wait for the user to choose
 *  if load the backup or the original file.
 *
 *  \todo Make this a registered callback function with user data,
 *        as we'd rather be passed a GschemToplevel than a TOPLEVEL.
 *
 *  \param [in] user_data The TOPLEVEL object.
 *  \param [in] message   Message to display to user.
 *  \return TRUE if the user wants to load the backup file, FALSE otherwise.
 */
int x_fileselect_load_backup(void *user_data, GString *message)
{
  GtkWidget *dialog;
  GschemToplevel *w_current = (GschemToplevel *) user_data;

  g_string_append(message, _(
"\n"
"If you load the original file, the backup file "
"will be overwritten in the next autosave timeout and it will be lost."
"\n\n"
"Do you want to load the backup file?\n"));

  dialog = gtk_message_dialog_new (GTK_WINDOW(w_current->main_window),
                                   GTK_DIALOG_MODAL,
                                   GTK_MESSAGE_QUESTION,
                                   GTK_BUTTONS_YES_NO,
                                   "%s", message->str);

  /* Set the alternative button order (ok, cancel, help) for other systems */
  gtk_dialog_set_alternative_button_order(GTK_DIALOG(dialog),
					  GTK_RESPONSE_YES,
					  GTK_RESPONSE_NO,
					  -1);

  gtk_widget_show (dialog);
  if (gtk_dialog_run ((GtkDialog*)dialog) == GTK_RESPONSE_YES) {
    gtk_widget_destroy(dialog);
    return TRUE;
  }
  else {
    gtk_widget_destroy(dialog);
    return FALSE;
  }
}
