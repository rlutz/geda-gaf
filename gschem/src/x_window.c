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
#include <unistd.h>
#include <sys/stat.h>

#include "gdk/gdk.h"
#ifdef GDK_WINDOWING_X11
#include "gdk/gdkx.h"
#endif

#include "gschem.h"
#include "actions.decl.x"

#include "gschem_compselect_dockable.h"
#include "gschem_object_properties_dockable.h"
#include "gschem_text_properties_dockable.h"
#include "gschem_multiattrib_dockable.h"
#include "gschem_options_dockable.h"
#include "gschem_log_dockable.h"
#include "gschem_messages_dockable.h"
#include "gschem_find_text_dockable.h"
#include "gschem_patch_dockable.h"
#include "gschem_pagesel_dockable.h"

#define GSCHEM_THEME_ICON_NAME "geda-gschem"

/*! \todo Finish function documentation!!!
 *  \brief
 *  \par Function Description
 *
 */
void x_window_setup (GschemToplevel *w_current)
{
  TOPLEVEL *toplevel = gschem_toplevel_get_toplevel (w_current);

  /* immediately setup user params */
  i_vars_set(w_current);

  /* Initialize the autosave callback */
  s_page_autosave_init(toplevel);

  /* Initialize the clipboard callback */
  x_clipboard_init (w_current);

  /* Add to the list of windows */
  global_window_list = g_list_append (global_window_list, w_current);

  /* X related stuff */
  x_menus_create_submenus (w_current);
  x_window_create_main (w_current);

  /* update sensitivity of paste action */
  x_clipboard_update_menus (w_current);

  gschem_action_set_sensitive (action_add_last_component, FALSE, w_current);

  /* disable terminal REPL action if stdin is not a terminal */
  gschem_action_set_sensitive (action_file_repl, isatty (STDIN_FILENO),
                               w_current);
}

/*! \todo Finish function documentation!!!
 *  \brief
 *  \par Function Description
 *
 */
void x_window_create_drawing(GtkWidget *scrolled, GschemToplevel *w_current)
{
  /* drawing next */
  w_current->drawing_area = GTK_WIDGET (gschem_page_view_new_with_page (w_current->toplevel->page_current));
  /* Set the size here.  Be sure that it has an aspect ratio of 1.333
   * We could calculate this based on root window size, but for now
   * lets just set it to:
   * Width = root_width*3/4   Height = Width/1.3333333333
   * 1.3333333 is the desired aspect ratio!
   */

  gtk_container_add(GTK_CONTAINER(scrolled), w_current->drawing_area);

  GTK_WIDGET_SET_FLAGS (w_current->drawing_area, GTK_CAN_FOCUS );
  gtk_widget_grab_focus (w_current->drawing_area);
  gtk_widget_show (w_current->drawing_area);
}

/*! \brief Set up callbacks for window events that affect drawing.
 *  \par Function Description
 *
 * Installs GTK+ callback handlers for signals that are emitted by
 * the drawing area, and some for the main window that affect the drawing
 * area.
 *
 * \param [in] w_current The toplevel environment.
 */
void x_window_setup_draw_events(GschemToplevel *w_current)
{
  struct event_reg_t {
    gchar *detailed_signal;
    GCallback c_handler;
  };

  struct event_reg_t drawing_area_events[] = {
    { "expose_event",         G_CALLBACK(x_event_expose)                       },
    { "expose_event",         G_CALLBACK(x_event_raise_dialog_boxes)           },
    { "button_press_event",   G_CALLBACK(x_event_button_pressed)               },
    { "button_release_event", G_CALLBACK(x_event_button_released)              },
    { "motion_notify_event",  G_CALLBACK(x_event_motion)                       },
    { "configure_event",      G_CALLBACK(x_event_configure)                    },
    { "key_press_event",      G_CALLBACK(x_event_key)                          },
    { "key_release_event",    G_CALLBACK(x_event_key)                          },
    { "scroll_event",         G_CALLBACK(x_event_scroll)                       },
    { "update-grid-info",     G_CALLBACK(i_update_grid_info_callback)          },
    { "notify::page",         G_CALLBACK(gschem_toplevel_notify_page_callback) },
    { NULL,                   NULL                                             } };
  struct event_reg_t main_window_events[] = {
    { "enter_notify_event",   G_CALLBACK(x_event_enter)              },
    { NULL,                   NULL                                   } };
  struct event_reg_t *tmp;

  /* is the configure event type missing here? hack */
  gtk_widget_set_events (w_current->drawing_area,
                         GDK_EXPOSURE_MASK |
                         GDK_POINTER_MOTION_MASK |
                         GDK_BUTTON_PRESS_MASK   |
                         GDK_ENTER_NOTIFY_MASK |
                         GDK_KEY_PRESS_MASK |
                         GDK_BUTTON_RELEASE_MASK);

  for (tmp = drawing_area_events; tmp->detailed_signal != NULL; tmp++) {
    g_signal_connect (w_current->drawing_area,
                      tmp->detailed_signal,
                      tmp->c_handler,
                      w_current);
  }

  for (tmp = main_window_events; tmp->detailed_signal != NULL; tmp++) {
    g_signal_connect (w_current->main_window,
                      tmp->detailed_signal,
                      tmp->c_handler,
                      w_current);
  }
}


static void
x_window_find_text (GtkWidget *widget, gint response, GschemToplevel *w_current)
{
  gint close = FALSE;
  int count;

  g_return_if_fail (w_current != NULL);
  g_return_if_fail (w_current->toplevel != NULL);

  switch (response) {
  case GTK_RESPONSE_OK:
    count = gschem_find_text_dockable_find (
        GSCHEM_FIND_TEXT_DOCKABLE (w_current->find_text_dockable),
        geda_list_get_glist (w_current->toplevel->pages),
        gschem_find_text_widget_get_find_type (GSCHEM_FIND_TEXT_WIDGET (w_current->find_text_widget)),
        gschem_find_text_widget_get_find_text_string (GSCHEM_FIND_TEXT_WIDGET (w_current->find_text_widget)),
        gschem_find_text_widget_get_descend (GSCHEM_FIND_TEXT_WIDGET (w_current->find_text_widget)));
    if (count > 0) {
      gschem_dockable_present (w_current->find_text_dockable);
      close = TRUE;
    };
    break;

  case GTK_RESPONSE_CANCEL:
  case GTK_RESPONSE_DELETE_EVENT:
    close = TRUE;
    break;

  default:
    printf("x_window_find_text(): strange signal %d\n", response);
  }

  if (close) {
    gtk_widget_grab_focus (w_current->drawing_area);
    gtk_widget_hide (GTK_WIDGET (widget));
  }
}



static void
x_window_hide_text (GtkWidget *widget, gint response, GschemToplevel *w_current)
{
  g_return_if_fail (w_current != NULL);
  g_return_if_fail (w_current->toplevel != NULL);

  if (response == GTK_RESPONSE_OK) {
    o_edit_hide_specific_text (w_current,
                               s_page_objects (w_current->toplevel->page_current),
                               gschem_show_hide_text_widget_get_text_string (GSCHEM_SHOW_HIDE_TEXT_WIDGET (widget)));
  }

  gtk_widget_grab_focus (w_current->drawing_area);
  gtk_widget_hide (GTK_WIDGET (widget));
}


static void
x_window_show_text (GtkWidget *widget, gint response, GschemToplevel *w_current)
{
  g_return_if_fail (w_current != NULL);
  g_return_if_fail (w_current->toplevel != NULL);

  if (response == GTK_RESPONSE_OK) {
    o_edit_show_specific_text (w_current,
                               s_page_objects (w_current->toplevel->page_current),
                               gschem_show_hide_text_widget_get_text_string (GSCHEM_SHOW_HIDE_TEXT_WIDGET (widget)));
  }

  gtk_widget_grab_focus (w_current->drawing_area);
  gtk_widget_hide (GTK_WIDGET (widget));
}


static void
x_window_invoke_macro (GschemMacroWidget *widget, int response, GschemToplevel *w_current)
{
  if (response == GTK_RESPONSE_OK) {
    const char *macro = gschem_macro_widget_get_macro_string (widget);

    SCM interpreter = scm_list_2(scm_from_utf8_symbol("invoke-macro"),
                                 scm_from_utf8_string(macro));

    scm_dynwind_begin (0);
    g_dynwind_window (w_current);
    g_scm_eval_protected(interpreter, SCM_UNDEFINED);
    scm_dynwind_end ();
  }

  gtk_widget_grab_focus (w_current->drawing_area);
  gtk_widget_hide (GTK_WIDGET (widget));
}

static void
x_window_select_text (GschemFindTextDockable *dockable, OBJECT *object, GschemToplevel *w_current)
{
  GschemPageView *view = gschem_toplevel_get_current_page_view (w_current);
  g_return_if_fail (view != NULL);

  OBJECT *page_obj;

  g_return_if_fail (object != NULL);
  page_obj = gschem_page_get_page_object(object);
  g_return_if_fail (page_obj != NULL);

  x_window_set_current_page (w_current, page_obj->page);

  gschem_page_view_zoom_text (view, object, TRUE);
}

static gboolean
x_window_state_event (GtkWidget *widget,
                      GdkEventWindowState *event,
                      gpointer user_data)
{
  eda_config_set_string (
    eda_config_get_user_context (),
    "gschem.window-geometry", "state",
    (event->new_window_state &
       GDK_WINDOW_STATE_FULLSCREEN) ? "fullscreen" :
    (event->new_window_state &
       GDK_WINDOW_STATE_MAXIMIZED) ? "maximized" : "normal");

  return FALSE;  /* propagate the event further */
}

static void
x_window_save_menu_geometry (GtkMenuShell *menu_shell,
                             GschemToplevel *w_current)
{
  for (GList *l = gtk_container_get_children (GTK_CONTAINER (menu_shell));
       l != NULL; l = l->next) {
    GtkMenuItem *menu_item = GTK_MENU_ITEM (l->data);

    GtkWidget *menu = menu_item->submenu;
    if (menu == NULL)
      /* not a submenu */
      continue;

    char *settings_name = g_object_get_data (G_OBJECT (menu), "settings-name");
    if (settings_name == NULL)
      /* menu doesn't have a settings name set */
      continue;

    gint coords[4];
    gsize length = 0;

    if (GTK_MENU (menu)->torn_off) {
      GtkWidget *window = GTK_MENU (menu)->tearoff_window;
      g_return_if_fail (window != NULL);

      gtk_window_get_position (GTK_WINDOW (window), &coords[0], &coords[1]);
      gtk_window_get_size (GTK_WINDOW (window), &coords[2], &coords[3]);
      length = 4;
    }

    eda_config_set_int_list (
      eda_config_get_user_context (),
      "gschem.menu-geometry", settings_name, coords, length);

    x_window_save_menu_geometry (GTK_MENU_SHELL (menu), w_current);
  }
}

static void
x_window_save_geometry (GschemToplevel *w_current)
{
  gchar *window_state;
  GtkAllocation allocation;

  /* save window geometry */
  window_state = eda_config_get_string (eda_config_get_user_context (),
                                        "gschem.window-geometry",
                                        "state", NULL);
  if (window_state != NULL && strcmp (window_state, "normal") == 0) {
    gint width = -1, height = -1;
    gtk_window_get_size (GTK_WINDOW (w_current->main_window), &width, &height);
    if (width > 0 && height > 0) {
      eda_config_set_int (eda_config_get_user_context (),
                          "gschem.window-geometry", "width", width);
      eda_config_set_int (eda_config_get_user_context (),
                          "gschem.window-geometry", "height", height);
    }
  }
  g_free (window_state);

  /* save torn-off menus */
  if (w_current->menubar != NULL)
    x_window_save_menu_geometry (
      GTK_MENU_SHELL (w_current->menubar), w_current);

  /* save dock area geometry */
  gtk_widget_get_allocation (w_current->left_notebook, &allocation);
  if (allocation.width > 0)
    eda_config_set_int (eda_config_get_user_context (),
                        "gschem.dock-geometry.left",
                        "size", allocation.width);

  gtk_widget_get_allocation (w_current->bottom_notebook, &allocation);
  if (allocation.height > 0)
    eda_config_set_int (eda_config_get_user_context (),
                        "gschem.dock-geometry.bottom",
                        "size", allocation.height);

  gtk_widget_get_allocation (w_current->right_notebook, &allocation);
  if (allocation.width > 0)
    eda_config_set_int (eda_config_get_user_context (),
                        "gschem.dock-geometry.right",
                        "size", allocation.width);
}

static void
x_window_restore_menu_geometry (GtkMenuShell *menu_shell,
                                GschemToplevel *w_current)
{
  for (GList *l = gtk_container_get_children (GTK_CONTAINER (menu_shell));
       l != NULL; l = l->next) {
    GtkMenuItem *menu_item = GTK_MENU_ITEM (l->data);

    GtkWidget *menu = menu_item->submenu;
    if (menu == NULL)
      /* not a submenu */
      continue;

    char *settings_name = g_object_get_data (G_OBJECT (menu), "settings-name");
    if (settings_name == NULL)
      /* menu doesn't have a settings name set */
      continue;

    gsize length = 0;
    gint *coords = eda_config_get_int_list (
      eda_config_get_user_context (),
      "gschem.menu-geometry", settings_name, &length, NULL);

    if (coords != NULL && length == 4) {
      gtk_menu_set_tearoff_state (GTK_MENU (menu), TRUE);

      GtkWidget *window = GTK_MENU (menu)->tearoff_window;
      g_return_if_fail (window != NULL);

      gtk_window_move (GTK_WINDOW (window), coords[0], coords[1]);
      gtk_window_resize (GTK_WINDOW (window), coords[2], coords[3]);
    }
    g_free(coords);

    x_window_restore_menu_geometry (GTK_MENU_SHELL (menu), w_current);
  }
}

static gboolean
x_window_restore_all_menu_geometry (GschemToplevel *w_current)
{
  g_signal_handlers_disconnect_by_func(
    G_OBJECT (w_current->main_window),
    G_CALLBACK (x_window_restore_all_menu_geometry), w_current);

  if (w_current->menubar != NULL)
    x_window_restore_menu_geometry (
      GTK_MENU_SHELL (w_current->menubar), w_current);

  return FALSE;
}

static void
x_window_restore_geometry (GschemToplevel *w_current)
{
  gint width, height, dock_size;
  gchar *window_state;

  /* restore main window size */
  width = eda_config_get_int (eda_config_get_user_context (),
                              "gschem.window-geometry", "width", NULL);
  height = eda_config_get_int (eda_config_get_user_context (),
                               "gschem.window-geometry", "height", NULL);
  if (width <= 0 || height <= 0) {
    width = 1320;
    height = 990;
  }
  g_object_set (w_current->main_window,
                "default-width", width,
                "default-height", height,
                NULL);

  /* restore main window state */
  window_state = eda_config_get_string (eda_config_get_user_context (),
                                        "gschem.window-geometry",
                                        "state", NULL);
  if (window_state != NULL && strcmp (window_state, "fullscreen") == 0)
    gtk_window_fullscreen (GTK_WINDOW (w_current->main_window));
  else if (window_state != NULL && strcmp (window_state, "maximized") == 0)
    gtk_window_maximize (GTK_WINDOW (w_current->main_window));
  g_free (window_state);

  /* defer restoring torn-off menus until main window is shown */
  g_signal_connect_swapped (
    G_OBJECT (w_current->main_window), "focus-in-event",
    G_CALLBACK (x_window_restore_all_menu_geometry), w_current);

  /* restore docking area dimensions */
  dock_size = eda_config_get_int (eda_config_get_user_context (),
                                  "gschem.dock-geometry.left", "size", NULL);
  if (dock_size <= 0)
    dock_size = 330;
  gtk_widget_set_size_request (w_current->left_notebook, dock_size, 0);

  dock_size = eda_config_get_int (eda_config_get_user_context (),
                                  "gschem.dock-geometry.bottom", "size", NULL);
  if (dock_size <= 0)
    dock_size = 165;
  gtk_widget_set_size_request (w_current->bottom_notebook, 0, dock_size);

  dock_size = eda_config_get_int (eda_config_get_user_context (),
                                  "gschem.dock-geometry.right", "size", NULL);
  if (dock_size <= 0)
    dock_size = 330;
  gtk_widget_set_size_request (w_current->right_notebook, dock_size, 0);
}


/*! \todo Finish function documentation!!!
 *  \brief
 *  \par Function Description
 *
 *  \note
 *  When invoked (via signal delete_event), closes the current window
 *  if this is the last window, quit gschem
 *  used when you click the close button on the window which sends a DELETE
 *  signal to the app
 */
static gboolean
x_window_close_wm (GtkWidget *widget, GdkEvent *event, gpointer data)
{
  GschemToplevel *w_current = GSCHEM_TOPLEVEL (data);
  g_return_val_if_fail ((w_current != NULL), TRUE);

  x_window_close(w_current);

  /* stop further propagation of the delete_event signal for window: */
  /*   - if user has cancelled the close the window should obvioulsy */
  /*   not be destroyed */
  /*   - otherwise window has already been destroyed, nothing more to */
  /*   do */
  return TRUE;
}


void
x_window_update_file_change_notification (GschemToplevel *w_current,
                                          PAGE *page)
{
  if (page->is_untitled) {
    g_object_set (w_current->file_change_notification,
                  "gschem-page", page,
                  "path",        NULL,
                  NULL);
    return;
  }

  gchar *basename = g_path_get_basename (page->page_filename);
  gchar *markup = page->exists_on_disk
    ? g_markup_printf_escaped (
        _("<b>The file \"%s\" has changed on disk.</b>\n\n%s"),
        basename,
        page->CHANGED
          ? _("Do you want to drop your changes and reload the file?")
          : _("Do you want to reload it?"))
    : g_markup_printf_escaped (
        _("<b>The file \"%s\" has been created on disk.</b>\n\n%s"),
        basename,
        page->CHANGED
          ? _("Do you want to drop your changes and load the file?")
          : _("Do you want to open it?"));
  g_object_set (w_current->file_change_notification,
                "gschem-page",     page,
                "path",            page->page_filename,
                "has-known-mtime", page->exists_on_disk,
                "known-mtime",     &page->last_modified,
                "button-stock-id", page->CHANGED
                                     ? GTK_STOCK_REVERT_TO_SAVED
                                     : page->exists_on_disk ? GTK_STOCK_REFRESH
                                                            : GTK_STOCK_OPEN,
                "button-label",    page->CHANGED
                                     ? _("_Revert")
                                     : page->exists_on_disk ? _("_Reload")
                                                            : _("_Open"),
                "markup",          markup,
                NULL);
  g_free (markup);
  g_free (basename);
}

static void
x_window_file_change_response (GschemChangeNotification *chnot,
                               gint response_id, gpointer user_data)
{
  if (response_id == GTK_RESPONSE_ACCEPT)
    x_lowlevel_revert_page (chnot->w_current, chnot->page);
  else {
    chnot->page->exists_on_disk = chnot->has_current_mtime;
    chnot->page->last_modified = chnot->current_mtime;
    x_window_update_file_change_notification (chnot->w_current, chnot->page);
  }
}


void
x_window_update_patch_change_notification (GschemToplevel *w_current,
                                           PAGE *page)
{
  gchar *patch_filename = x_patch_guess_filename (page);
  gchar *basename, *markup;

  if (patch_filename == NULL) {
    basename = NULL;
    markup = NULL;
  } else {
    struct stat buf;
    basename = g_path_get_basename (patch_filename);
    if (page->patch_filename != NULL)
      markup = g_markup_printf_escaped (
        _("<b>The back-annotation patch \"%s\" has been updated.</b>\n\n"
          "Do you want to re-import it?"),
        basename);
    else if (page->patch_seen_on_disk)
      markup = g_markup_printf_escaped (
        _("<b>The back-annotation patch \"%s\" has been updated.</b>\n\n"
          "Do you want to import it?"),
        basename);
    else if (stat (patch_filename, &buf) != -1)
      markup = g_markup_printf_escaped (
        _("<b>This file appears to have a back-annotation patch \"%s\" "
          "associated with it.</b>\n\nDo you want to import it?"),
        basename);
    else
      markup = g_markup_printf_escaped (
        _("<b>A back-annotation patch \"%s\" has been created.</b>\n\n"
          "Do you want to import it?"),
        basename);
  }

  g_object_set (w_current->patch_change_notification,
                "gschem-page",     page,
                "path",            patch_filename,
                "has-known-mtime", page->patch_seen_on_disk,
                "known-mtime",     &page->patch_mtime,
                "button-label",    _("_Import"),
                "markup",          markup,
                NULL);
  g_free (markup);
  g_free (basename);
  g_free (patch_filename);
}

static void
x_window_patch_change_response (GschemChangeNotification *chnot,
                                gint response_id, gpointer user_data)
{
  if (response_id == GTK_RESPONSE_ACCEPT) {
    if (chnot->page->patch_filename == NULL)
      chnot->page->patch_filename = g_strdup (chnot->path);
    x_patch_do_import (chnot->w_current, chnot->page);
  } else {
    chnot->page->patch_seen_on_disk = chnot->has_current_mtime;
    chnot->page->patch_mtime = chnot->current_mtime;
    x_window_update_patch_change_notification (chnot->w_current, chnot->page);
  }
}


/*! \todo Finish function documentation!!!
 *  \brief
 *  \par Function Description
 *
 */
void x_window_create_main(GschemToplevel *w_current)
{
  TOPLEVEL *toplevel = gschem_toplevel_get_toplevel (w_current);

  GtkPolicyType policy;
  GtkWidget *main_box=NULL;
  GtkWidget *handlebox=NULL;
  GtkWidget *scrolled;
  GtkAdjustment *hadjustment;
  GtkAdjustment *vadjustment;
  char *right_button_text;
  GtkWidget *left_hpaned, *right_hpaned;
  GtkWidget *vpaned;
  GtkWidget *work_box;

  w_current->main_window = GTK_WIDGET (gschem_main_window_new ());

  gtk_widget_set_name (w_current->main_window, "gschem");
  gtk_window_set_policy (GTK_WINDOW (w_current->main_window), TRUE, TRUE, TRUE);

  /* We want the widgets to flow around the drawing area, so we don't
   * set a size of the main window.  The drawing area's size is fixed,
   * see below
   */

   /*
    * normally we let the window manager handle locating and sizing
    * the window.  However, for some batch processing of schematics
    * (generating a pdf of all schematics for example) we want to
    * override this.  Hence "auto_place_mode".
    */
   if( auto_place_mode )
   	gtk_widget_set_uposition (w_current->main_window, 10, 10);

  /* this should work fine */
  g_signal_connect (G_OBJECT (w_current->main_window), "delete_event",
                    G_CALLBACK (x_window_close_wm), w_current);

  /* Containers first */
  main_box = gtk_vbox_new(FALSE, 1);
  gtk_container_set_border_width (GTK_CONTAINER (main_box), 0);
  gtk_container_add(GTK_CONTAINER(w_current->main_window), main_box);

  x_menus_create_main_menu (w_current);
  if (w_current->menubar != NULL) {
    if (w_current->handleboxes) {
      handlebox = gtk_handle_box_new ();
      gtk_box_pack_start (GTK_BOX (main_box), handlebox, FALSE, FALSE, 0);
      gtk_container_add (GTK_CONTAINER (handlebox), w_current->menubar);
    } else {
      gtk_box_pack_start (GTK_BOX (main_box), w_current->menubar,
                          FALSE, FALSE, 0);
    }
  }
  gschem_action_set_sensitive (action_view_menubar, w_current->menubar != NULL,
                               w_current);
  gschem_action_set_active (action_view_menubar, w_current->menubar != NULL,
                            w_current);

  gtk_widget_realize (w_current->main_window);

  x_menus_create_toolbar (w_current);
  if (w_current->toolbar != NULL) {
    if (w_current->handleboxes) {
      handlebox = gtk_handle_box_new ();
      gtk_box_pack_start (GTK_BOX (main_box), handlebox, FALSE, FALSE, 0);
      gtk_container_add (GTK_CONTAINER (handlebox), w_current->toolbar);
      gtk_widget_set_visible (handlebox, w_current->toolbars);
      gtk_widget_set_no_show_all (handlebox, TRUE);
    } else {
      gtk_box_pack_start (GTK_BOX (main_box), w_current->toolbar,
                          FALSE, FALSE, 0);
      gtk_widget_set_visible (w_current->toolbar, w_current->toolbars);
      gtk_widget_set_no_show_all (w_current->toolbar, TRUE);
    }
  }
  gschem_action_set_sensitive (action_view_toolbar, w_current->toolbar != NULL,
                               w_current);
  gschem_action_set_active (action_view_toolbar,
                            w_current->toolbars && w_current->toolbar != NULL,
                            w_current);

  left_hpaned = gtk_hpaned_new ();
  gtk_container_add (GTK_CONTAINER(main_box), left_hpaned);

  w_current->left_notebook = gtk_notebook_new ();
  gtk_paned_pack1 (GTK_PANED (left_hpaned),
                   w_current->left_notebook,
                   FALSE,
                   TRUE);
  gtk_notebook_set_group_name (GTK_NOTEBOOK (w_current->left_notebook),
                               "gschem-dock");

  right_hpaned = gtk_hpaned_new ();
  gtk_paned_pack2 (GTK_PANED (left_hpaned),
                   right_hpaned,
                   TRUE,
                   TRUE);

  w_current->right_notebook = gtk_notebook_new ();
  gtk_paned_pack2 (GTK_PANED (right_hpaned),
                   w_current->right_notebook,
                   FALSE,
                   TRUE);
  gtk_notebook_set_group_name (GTK_NOTEBOOK (w_current->right_notebook),
                               "gschem-dock");

  vpaned = gtk_vpaned_new ();
  gtk_paned_pack1 (GTK_PANED (right_hpaned),
                   vpaned,
                   TRUE,
                   TRUE);

  w_current->bottom_notebook = gtk_notebook_new ();
  gtk_paned_pack2 (GTK_PANED (vpaned),
                   w_current->bottom_notebook,
                   FALSE,
                   TRUE);
  gtk_notebook_set_group_name (GTK_NOTEBOOK (w_current->bottom_notebook),
                               "gschem-dock");

  work_box = gtk_vbox_new (FALSE, 0);
  gtk_paned_pack1 (GTK_PANED (vpaned),
                   work_box,
                   TRUE,
                   TRUE);

  w_current->file_change_notification =
    g_object_new (GSCHEM_TYPE_CHANGE_NOTIFICATION,
                  "gschem-toplevel", w_current,
                  "message-type", GTK_MESSAGE_QUESTION,
                  NULL);
  g_signal_connect (w_current->file_change_notification, "response",
                    G_CALLBACK (x_window_file_change_response), NULL);
  gtk_box_pack_start (GTK_BOX (work_box),
                      w_current->file_change_notification->info_bar,
                      FALSE, FALSE, 0);

  w_current->patch_change_notification =
    g_object_new (GSCHEM_TYPE_CHANGE_NOTIFICATION,
                  "gschem-toplevel", w_current,
                  "message-type", GTK_MESSAGE_INFO,
                  NULL);
  g_signal_connect (w_current->patch_change_notification, "response",
                    G_CALLBACK (x_window_patch_change_response), NULL);
  gtk_box_pack_start (GTK_BOX (work_box),
                      w_current->patch_change_notification->info_bar,
                      FALSE, FALSE, 0);

  /*  Try to create popup menu (appears in right mouse button  */
  x_menus_create_main_popup (w_current);


  /* Setup a GtkScrolledWindow for the drawing area */
  hadjustment = GTK_ADJUSTMENT (gtk_adjustment_new (0.0,
                                                    toplevel->init_left,
                                                    toplevel->init_right,
                                                    100.0,
                                                    100.0,
                                                    10.0));

  vadjustment = GTK_ADJUSTMENT (gtk_adjustment_new (toplevel->init_bottom,
                                                    0.0,
                                                    toplevel->init_bottom - toplevel->init_top,
                                                    100.0,
                                                    100.0,
                                                    10.0));

  scrolled = gtk_scrolled_window_new (hadjustment, vadjustment);
  gtk_container_add (GTK_CONTAINER (work_box), scrolled);
  x_window_create_drawing(scrolled, w_current);
  x_window_setup_draw_events(w_current);

  policy = (w_current->scrollbars_flag) ? GTK_POLICY_ALWAYS : GTK_POLICY_NEVER;
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled), policy, policy);
  gschem_action_set_active (action_view_scrollbars, w_current->scrollbars_flag,
                            w_current);

  /* find text box */
  w_current->find_text_widget = GTK_WIDGET (g_object_new (GSCHEM_TYPE_FIND_TEXT_WIDGET, NULL));

  gtk_box_pack_start (GTK_BOX (work_box),
                      w_current->find_text_widget,
                      FALSE,
                      FALSE,
                      0);

  g_signal_connect (w_current->find_text_widget,
                    "response",
                    G_CALLBACK (&x_window_find_text),
                    w_current);

  /* hide text box */
  w_current->hide_text_widget = GTK_WIDGET (g_object_new (GSCHEM_TYPE_SHOW_HIDE_TEXT_WIDGET,
                                                          "button-text", pgettext ("actuate", "Hide"),
                                                          "label-text",  _("Hide text starting with:"),
                                                          NULL));

  gtk_box_pack_start (GTK_BOX (work_box),
                      w_current->hide_text_widget,
                      FALSE,
                      FALSE,
                      0);

  g_signal_connect (w_current->hide_text_widget,
                    "response",
                    G_CALLBACK (&x_window_hide_text),
                    w_current);

  /* show text box */
  w_current->show_text_widget = GTK_WIDGET (g_object_new (GSCHEM_TYPE_SHOW_HIDE_TEXT_WIDGET,
                                                          "button-text", pgettext ("actuate", "Show"),
                                                          "label-text",  _("Show text starting with:"),
                                                          NULL));

  gtk_box_pack_start (GTK_BOX (work_box),
                      w_current->show_text_widget,
                      FALSE,
                      FALSE,
                      0);

  g_signal_connect (w_current->show_text_widget,
                    "response",
                    G_CALLBACK (&x_window_show_text),
                    w_current);

  /* macro box */
  w_current->macro_widget = GTK_WIDGET (g_object_new (GSCHEM_TYPE_MACRO_WIDGET, NULL));

  gtk_box_pack_start (GTK_BOX (work_box),
                      w_current->macro_widget,
                      FALSE,
                      FALSE,
                      0);

  g_signal_connect (w_current->macro_widget,
                    "response",
                    G_CALLBACK (&x_window_invoke_macro),
                    w_current);


  w_current->compselect_dockable = g_object_new (
    GSCHEM_TYPE_COMPSELECT_DOCKABLE,
    "title", _("Library"),
    "settings-name", "compselect",
    "cancellable", TRUE,
    "initial-state", GSCHEM_DOCKABLE_STATE_DOCKED_RIGHT,
    "initial-width", 500,
    "initial-height", 600,
    "gschem-toplevel", w_current,
    NULL);

  w_current->object_properties_dockable = g_object_new (
    GSCHEM_TYPE_OBJECT_PROPERTIES_DOCKABLE,
    "title", _("Object"),
    "settings-name", "object-properties",  /* line-type */
    "initial-state", GSCHEM_DOCKABLE_STATE_DOCKED_RIGHT,
    "initial-width", 400,
    "initial-height", 600,
    "gschem-toplevel", w_current,
    NULL);

  w_current->text_properties_dockable = g_object_new (
    GSCHEM_TYPE_TEXT_PROPERTIES_DOCKABLE,
    "title", _("Text"),
    "settings-name", "text-edit",
    "initial-state", GSCHEM_DOCKABLE_STATE_DOCKED_RIGHT,
    "initial-width", 400,
    "initial-height", 450,
    "gschem-toplevel", w_current,
    NULL);

  w_current->multiattrib_dockable = g_object_new (
    GSCHEM_TYPE_MULTIATTRIB_DOCKABLE,
    "title", _("Attributes"),
    "settings-name", "multiattrib",
    "initial-state", GSCHEM_DOCKABLE_STATE_DOCKED_RIGHT,
    "initial-width", 450,
    "initial-height", 450,
    "gschem-toplevel", w_current,
    NULL);

  w_current->options_dockable = g_object_new (
    GSCHEM_TYPE_OPTIONS_DOCKABLE,
    "title", _("Options"),
    "settings-name", "options",  /* snap-size */
    "initial-state", GSCHEM_DOCKABLE_STATE_HIDDEN,
    "initial-width", 320,
    "initial-height", 350,
    "gschem-toplevel", w_current,
    NULL);

  w_current->log_dockable = g_object_new (
    GSCHEM_TYPE_LOG_DOCKABLE,
    "title", _("Status"),
    "settings-name", "log",
    "initial-state", GSCHEM_DOCKABLE_STATE_DOCKED_BOTTOM,
    "initial-width", 640,
    "initial-height", 480,
    "gschem-toplevel", w_current,
    NULL);

  w_current->messages_dockable = g_object_new (
    GSCHEM_TYPE_MESSAGES_DOCKABLE,
    "title", _("Messages"),
    "settings-name", "messages",
    "initial-state", GSCHEM_DOCKABLE_STATE_DOCKED_BOTTOM,
    "initial-width", 800,
    "initial-height", 320,
    "gschem-toplevel", w_current,
    NULL);

  w_current->find_text_dockable = g_object_new (
    GSCHEM_TYPE_FIND_TEXT_DOCKABLE,
    "title", _("Search results"),
    "settings-name", "find-text",
    "initial-state", GSCHEM_DOCKABLE_STATE_DOCKED_BOTTOM,
    "initial-width", 500,
    "initial-height", 300,
    "gschem-toplevel", w_current,
    NULL);
  g_signal_connect (w_current->find_text_dockable,
                    "select-object",
                    G_CALLBACK (&x_window_select_text),
                    w_current);

  w_current->patch_dockable = g_object_new (
    GSCHEM_TYPE_PATCH_DOCKABLE,
    "title", _("Patch"),
    "settings-name", "patch",
    "initial-state", GSCHEM_DOCKABLE_STATE_HIDDEN,
    "initial-width", 500,
    "initial-height", 300,
    "gschem-toplevel", w_current,
    NULL);

  w_current->pagesel_dockable = g_object_new (
    GSCHEM_TYPE_PAGESEL_DOCKABLE,
    "title", _("Pages"),
    "settings-name", "pagesel",
    "initial-state", GSCHEM_DOCKABLE_STATE_HIDDEN,
    "initial-width", 515,
    "initial-height", 180,
    "gschem-toplevel", w_current,
    NULL);

  gschem_dockable_initialize_toplevel (w_current);


  /* bottom box */
  if (default_third_button == POPUP_ENABLED) {
    right_button_text = _("Menu/Cancel");
  } else {
    right_button_text = _("Pan/Cancel");
  }

  w_current->bottom_widget = GTK_WIDGET (g_object_new (GSCHEM_TYPE_BOTTOM_WIDGET,
      "grid-mode",          gschem_options_get_grid_mode (w_current->options),
      "grid-size",          gschem_options_get_snap_size (w_current->options), /* x_grid_query_drawn_spacing (w_current), -- occurs before the page is set */
      "left-button-text",   _("Pick"),
      "middle-button-text", _("none"),
      "right-button-text",  right_button_text,
      "snap-mode",          gschem_options_get_snap_mode (w_current->options),
      "snap-size",          gschem_options_get_snap_size (w_current->options),
      "status-text",        _("Select Mode"),
      NULL));

  i_update_middle_button (w_current, NULL, NULL);

  gtk_box_pack_start (GTK_BOX (main_box), w_current->bottom_widget, FALSE, FALSE, 0);

  x_window_restore_geometry (w_current);
  g_signal_connect (G_OBJECT (w_current->main_window), "window-state-event",
                    G_CALLBACK (x_window_state_event), w_current);

  gtk_widget_show_all (w_current->main_window);

  /* hide unused notebooks */
  if (gtk_notebook_get_n_pages (GTK_NOTEBOOK (w_current->left_notebook)) == 0)
    gtk_widget_hide (w_current->left_notebook);
  if (gtk_notebook_get_n_pages (GTK_NOTEBOOK (w_current->bottom_notebook)) == 0)
    gtk_widget_hide (w_current->bottom_notebook);
  if (gtk_notebook_get_n_pages (GTK_NOTEBOOK (w_current->right_notebook)) == 0)
    gtk_widget_hide (w_current->right_notebook);
}

/*! \todo Finish function documentation!!!
 *  \brief
 *  \par Function Description
 *
 */
void x_window_close(GschemToplevel *w_current)
{
  gboolean last_window = FALSE;

  /* If we're closing whilst inside an action, re-wind the
   * page contents back to their state before we started */
  if (w_current->inside_action) {
    i_cancel (w_current);
  }

  /* last chance to save possible unsaved pages */
  if (!x_dialog_close_window (w_current)) {
    /* user somehow cancelled the close */
    return;
  }

  x_clipboard_finish (w_current);

#if DEBUG
  o_conn_print_hash(w_current->page_current->conn_table);
#endif

  w_current->dont_invalidate = TRUE;

  /* save window geometry */
  x_window_save_geometry (w_current);

  /* close all the dialog boxes */
  if (w_current->sowindow)
  gtk_widget_destroy(w_current->sowindow);

  if (w_current->tiwindow)
  gtk_widget_destroy(w_current->tiwindow);

  if (w_current->aawindow)
  gtk_widget_destroy(w_current->aawindow);

  if (w_current->aewindow)
  gtk_widget_destroy(w_current->aewindow);

  if (w_current->hkwindow)
  gtk_widget_destroy(w_current->hkwindow);

  if (w_current->sewindow)
  gtk_widget_destroy(w_current->sewindow);

  /* save dock window geometry, close dock windows, disconnect signals */
  gschem_dockable_cleanup_toplevel (w_current);

  g_clear_object (&w_current->compselect_dockable);
  g_clear_object (&w_current->object_properties_dockable);
  g_clear_object (&w_current->text_properties_dockable);
  g_clear_object (&w_current->multiattrib_dockable);
  g_clear_object (&w_current->options_dockable);
  g_clear_object (&w_current->log_dockable);
  g_clear_object (&w_current->messages_dockable);
  g_clear_object (&w_current->find_text_dockable);
  g_clear_object (&w_current->patch_dockable);
  g_clear_object (&w_current->pagesel_dockable);

  if (g_list_length (global_window_list) == 1) {
    /* no more window after this one, remember to quit */
    last_window = TRUE;
  }

  /* stuff that has to be done before we free w_current */
  if (last_window) {
    /* close the log file */
    s_log_close ();
    /* free the buffers */
    o_buffer_free (w_current);
  }

  /* Allow Scheme value for this window to be garbage-collected */
  if (!SCM_UNBNDP (w_current->smob)) {
    SCM_SET_SMOB_DATA (w_current->smob, NULL);
    scm_gc_unprotect_object (w_current->smob);
    w_current->smob = SCM_UNDEFINED;
  }

  /* finally close the main window */
  gtk_widget_destroy(w_current->main_window);

  global_window_list = g_list_remove (global_window_list, w_current);
  gschem_toplevel_free (w_current);

  /* just closed last window, so quit */
  if (last_window) {
    gtk_main_quit();
  }
}

/*! \todo Finish function documentation!!!
 *  \brief
 *  \par Function Description
 *
 */
void x_window_close_all(GschemToplevel *w_current)
{
  GschemToplevel *current;
  GList *list_copy, *iter;

  iter = list_copy = g_list_copy (global_window_list);
  while (iter != NULL ) {
    current = (GschemToplevel *)iter->data;
    iter = g_list_next (iter);
    x_window_close (current);
  }
  g_list_free (list_copy);
}

/*! \brief Changes the current page.
 *  \par Function Description
 *  This function displays the specified page <B>page</B> in the
 *  window attached to <B>toplevel</B>.
 *
 *  It changes the <B>toplevel</B>'s current page to <B>page</B>,
 *  draws it and updates the user interface.
 *
 *  <B>page</B> has to be in the list of PAGEs attached to <B>toplevel</B>.
 *
 *  \param [in] w_current The toplevel environment.
 *  \param [in] page      The page to become current page.
 */
void
x_window_set_current_page (GschemToplevel *w_current, PAGE *page)
{
  GschemPageView *page_view = gschem_toplevel_get_current_page_view (w_current);
  TOPLEVEL *toplevel = gschem_toplevel_get_toplevel (w_current);
  GList *iter;

  g_return_if_fail (page_view != NULL);
  g_return_if_fail (toplevel != NULL);
  g_return_if_fail (page != NULL);

  g_warn_if_fail (page_view->page == toplevel->page_current ||
                  page_view->page == NULL);

  if (page == toplevel->page_current && page_view->page != NULL)
    /* nothing to do */
    return;

  o_redraw_cleanstates (w_current);

  gschem_page_view_set_page (page_view, page);

  gschem_action_set_sensitive (action_page_revert,
                               page->is_untitled == FALSE &&
                               g_file_test (page->page_filename,
                                            G_FILE_TEST_EXISTS |
                                            G_FILE_TEST_IS_REGULAR),
                               w_current);

  o_undo_update_actions (w_current, page);

  iter = g_list_find (geda_list_get_glist (toplevel->pages), page);
  gschem_action_set_sensitive (action_page_prev,
                               w_current->enforce_hierarchy
                                 ? s_hierarchy_find_prev_page (
                                     toplevel->pages, page) != NULL
                                 : iter != NULL && iter->prev != NULL,
                               w_current);
  gschem_action_set_sensitive (action_page_next,
                               w_current->enforce_hierarchy
                                 ? s_hierarchy_find_next_page(
                                     toplevel->pages, page) != NULL
                                 : iter != NULL && iter->next != NULL,
                               w_current);
  gschem_action_set_sensitive (action_hierarchy_up, page->up >= 0, w_current);

  i_update_menus (w_current);
  /* i_set_filename (w_current, page->page_filename); */

  x_window_update_file_change_notification (w_current, page);
  x_window_update_patch_change_notification (w_current, page);

  x_pagesel_update (w_current);
  x_multiattrib_update (w_current);
  x_messages_page_changed (w_current);
}

/*! \brief Raise the main window to the front.
 *
 * This is mostly equivalent to
 *   gtk_window_present (GTK_WINDOW (w_current->main_window));
 *
 * One of the two actions that \c gtk_window_present performs on an
 * already-visible window (\c gdk_window_show) is triggering a bug
 * with toolbar icon drawing, the other (\c gdk_window_focus) is the
 * one we actually want.  In order to work around that bug, just call
 * \c gdk_window_focus directly.
 *
 * \param [in] w_current  the toplevel environment
 */
void x_window_present (GschemToplevel *w_current)
{
  //gdk_window_show (w_current->main_window->window);  /* the culprit */

#ifdef GDK_WINDOWING_X11
  GdkDisplay *display = gtk_widget_get_display (w_current->main_window);
  guint32 timestamp = gdk_x11_display_get_user_time (display);
#else
  guint32 timestamp = gtk_get_current_event_time ();
#endif

  gdk_window_focus (w_current->main_window->window, timestamp);
}

/*! \brief Setup default icon for GTK windows
 *
 *  \par Function Description
 *  Sets the default window icon by name, to be found in the current icon
 *  theme. The name used is \#defined above as GSCHEM_THEME_ICON_NAME.
 */
void x_window_set_default_icon( void )
{
  gtk_window_set_default_icon_name( GSCHEM_THEME_ICON_NAME );
}

/*! \brief Setup icon search paths.
 * \par Function Description
 * Add the icons installed by gschem to the search path for the
 * default icon theme, so that they can be automatically found by GTK.
 */
void
x_window_init_icons (void)
{
  gchar *icon_path;

  g_return_if_fail (s_path_sys_data () != NULL);

  icon_path = g_build_filename (s_path_sys_data (), "icons", NULL);
  gtk_icon_theme_append_search_path (gtk_icon_theme_get_default (),
                                     icon_path);
  g_free (icon_path);
}

/*! \brief Creates a new X window.
 *
 * \par Function description
 *
 * Creates and initializes new GschemToplevel object and then sets
 * and setups its libgeda \a toplevel.
 *
 * \param toplevel The libgeda TOPLEVEL object.
 * \return Pointer to the new GschemToplevel object.
 */
GschemToplevel* x_window_new (TOPLEVEL *toplevel)
{
  GschemToplevel *w_current;

  w_current = gschem_toplevel_new ();
  gschem_toplevel_set_toplevel (w_current,
                                (toplevel != NULL) ? toplevel : s_toplevel_new ());

  gschem_toplevel_get_toplevel (w_current)->load_newer_backup_func = x_fileselect_load_backup;
  gschem_toplevel_get_toplevel (w_current)->load_newer_backup_data = w_current;

  o_text_set_rendered_bounds_func (gschem_toplevel_get_toplevel (w_current),
                                   o_text_get_rendered_bounds, w_current);

  /* Damage notifications should invalidate the object on screen */
  o_add_change_notify (gschem_toplevel_get_toplevel (w_current),
                       (ChangeNotifyFunc) o_invalidate,
                       (ChangeNotifyFunc) o_invalidate, w_current);

  x_window_setup (w_current);

  return w_current;
}
