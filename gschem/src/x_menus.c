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
#include "config.h"

#ifdef HAVE_STRING_H
#include <string.h>
#endif


#include "gschem.h"

#include <glib/gstdio.h>


static GtkWidget *
build_menu (SCM s_menu, gboolean is_main_menu, GschemToplevel *w_current)
{
  GtkWidget *menu = gtk_menu_new ();

  if (is_main_menu) {
    GtkWidget *tearoff_menu_item = gtk_tearoff_menu_item_new ();
    gtk_menu_shell_append (GTK_MENU_SHELL (menu), tearoff_menu_item);
    gtk_widget_show (tearoff_menu_item);
  }

  for (SCM l0 = s_menu; scm_is_pair (l0); l0 = scm_cdr (l0)) {
    SCM s_section = scm_car (l0);
    SCM_ASSERT (scm_is_true (scm_list_p (s_section)),
                s_section, SCM_ARGn, "build_menu section");

    for (SCM l1 = s_section; scm_is_pair (l1); l1 = scm_cdr (l1)) {
      SCM s_item = scm_car (l1);
      SCM_ASSERT (scm_is_action (s_item),
                  s_item, SCM_ARGn, "build_menu item");

      /* make sure the action won't ever be garbage collected
         since we're going to point to it from C data structures */
      scm_permanent_object (s_item);

      GschemAction *action = scm_to_action (s_item);
      GtkWidget *menu_item =
        gschem_action_create_menu_item (action, is_main_menu, w_current);
      gtk_menu_shell_append (GTK_MENU_SHELL (menu), menu_item);
      gtk_widget_show (menu_item);
    }

    if (scm_is_pair (scm_cdr (l0))) {
      /* add separator */
      GtkWidget *menu_item = gtk_menu_item_new ();
      gtk_menu_shell_append (GTK_MENU_SHELL (menu), menu_item);
      gtk_widget_show (menu_item);
    }
  }

  return menu;
}

/*! \todo Finish function documentation!!!
 *  \brief
 *  \par Function Description
 *
 */
void
x_menus_create_main_menu (GschemToplevel *w_current)
{
  SCM s_var = scm_module_variable (scm_current_module (),
                                   scm_from_utf8_symbol ("menubar"));
  if (!scm_is_true (s_var)) {
    g_critical (_("No menubar definition found\n"));
    return;
  }

  SCM s_menubar = scm_variable_ref (s_var);
  if (scm_is_null (s_menubar) || !scm_is_true (scm_list_p (s_menubar))) {
    g_critical (_("Empty or malformed menubar definition\n"));
    return;
  }

  w_current->menubar = gtk_menu_bar_new ();

  for (SCM l = s_menubar; scm_is_pair (l); l = scm_cdr (l)) {
    SCM s_menu = scm_car (l);
    SCM_ASSERT (scm_is_pair (s_menu),
                s_menu, SCM_ARGn, "get_main_menu menu");

    SCM s_title = scm_car (s_menu);
    SCM_ASSERT (scm_is_string (s_title),
                s_title, SCM_ARGn, "get_main_menu menu title");

    GtkWidget *menu = build_menu (scm_cdr (s_menu), TRUE, w_current);

    scm_dynwind_begin (0);
    {
      char *title = scm_to_utf8_string (s_title);
      scm_dynwind_free (title);

      /* store lower-case title without underscores as settings name
         for saving/restoring torn-off menus */
      gchar *settings_name = g_ascii_strdown (title, -1);
      gchar *i = settings_name, *j = settings_name;
      do {
        if (*i != '_')
          *j++ = *i;
      } while (*i++ != '\0');
      g_object_set_data (G_OBJECT (menu), "settings-name", settings_name);

      GtkWidget *root_menu = gtk_menu_item_new_with_mnemonic (title);
      gtk_widget_show (root_menu);
      gtk_menu_item_set_submenu (GTK_MENU_ITEM (root_menu), menu);
      /* no longer right justify the help menu since that has gone out
         of style */
      gtk_menu_shell_append (GTK_MENU_SHELL (w_current->menubar), root_menu);
    }
    scm_dynwind_end ();
  }
}

void
x_menus_create_main_popup (GschemToplevel *w_current)
{
  SCM s_var = scm_module_variable (scm_current_module (),
                                   scm_from_utf8_symbol ("context-menu"));
  if (!scm_is_true (s_var)) {
    g_warning (_("No context menu definition found\n"));
    return;
  }

  SCM s_menu = scm_variable_ref (s_var);
  if (scm_is_null (s_menu) || !scm_is_true (scm_list_p (s_menu))) {
    g_warning (_("Empty or malformed context menu definition\n"));
    return;
  }

  w_current->popup_menu = build_menu (s_menu, FALSE, w_current);
}

#define MAX_RECENT_FILES 10
/*! \brief Callback for recent-chooser.
 *
 * Will be called if element of recent-file-list is activated
 */
void
recent_chooser_item_activated (GtkRecentChooser *chooser, GschemToplevel *w_current)
{
  gchar *uri;
  gchar *filename;

  uri = gtk_recent_chooser_get_current_uri (chooser);
  filename = g_filename_from_uri(uri, NULL, NULL);
  x_highlevel_open_page (w_current, (char *) filename);

  g_free(uri);
  g_free(filename);
}

/*! \brief Create a submenu with filenames for the 'Open Recent'
 *         menu item.
 */
static GtkWidget *
create_recent_chooser_menu (GschemToplevel *w_current)
{
  GtkRecentFilter *recent_filter;
  GtkWidget *menuitem_file_recent_items;
  recent_manager = gtk_recent_manager_get_default();

  menuitem_file_recent_items = gtk_recent_chooser_menu_new_for_manager(recent_manager);

  /* Show only schematic- and symbol-files (*.sch and *.sym) in list */
  recent_filter = gtk_recent_filter_new();
  gtk_recent_filter_add_mime_type(recent_filter, "application/x-geda-schematic");
  gtk_recent_filter_add_mime_type(recent_filter, "application/x-geda-symbol");
  gtk_recent_filter_add_pattern(recent_filter, "*.sch");
  gtk_recent_filter_add_pattern(recent_filter, "*.sym");
  gtk_recent_chooser_add_filter(GTK_RECENT_CHOOSER(menuitem_file_recent_items), recent_filter);

  gtk_recent_chooser_set_show_tips(GTK_RECENT_CHOOSER(menuitem_file_recent_items), TRUE);
  gtk_recent_chooser_set_sort_type(GTK_RECENT_CHOOSER(menuitem_file_recent_items),
                                   GTK_RECENT_SORT_MRU);
  gtk_recent_chooser_set_limit(GTK_RECENT_CHOOSER(menuitem_file_recent_items), MAX_RECENT_FILES);
  gtk_recent_chooser_set_local_only(GTK_RECENT_CHOOSER(menuitem_file_recent_items), FALSE);
  gtk_recent_chooser_menu_set_show_numbers(GTK_RECENT_CHOOSER_MENU(menuitem_file_recent_items), TRUE);
  g_signal_connect(GTK_OBJECT(menuitem_file_recent_items), "item-activated",
                   G_CALLBACK(recent_chooser_item_activated), w_current);

  return menuitem_file_recent_items;
}

/*! \brief Create submenus for various menu items.
 *
 *  Called from x_window_setup().
 */
void
x_menus_create_submenus (GschemToplevel *w_current)
{
  /* create recent files menu */
  w_current->recent_chooser_menu = create_recent_chooser_menu (w_current);
  g_object_set_data (G_OBJECT (w_current->recent_chooser_menu),
                     "settings-name", "recent-files");

  /* create left docking area menu */
  w_current->left_docking_area_menu = gtk_menu_new ();
  g_object_set_data (G_OBJECT (w_current->left_docking_area_menu),
                     "settings-name", "left-docking-area");
  gtk_menu_shell_append (GTK_MENU_SHELL (w_current->left_docking_area_menu),
                         gtk_tearoff_menu_item_new ());
  gtk_widget_show_all (w_current->left_docking_area_menu);

  /* create bottom docking area menu */
  w_current->bottom_docking_area_menu = gtk_menu_new ();
  g_object_set_data (G_OBJECT (w_current->bottom_docking_area_menu),
                     "settings-name", "bottom-docking-area");
  gtk_menu_shell_append (GTK_MENU_SHELL (w_current->bottom_docking_area_menu),
                         gtk_tearoff_menu_item_new ());
  gtk_widget_show_all (w_current->bottom_docking_area_menu);

  /* create right docking area menu */
  w_current->right_docking_area_menu = gtk_menu_new ();
  g_object_set_data (G_OBJECT (w_current->right_docking_area_menu),
                     "settings-name", "right-docking-area");
  gtk_menu_shell_append (GTK_MENU_SHELL (w_current->right_docking_area_menu),
                         gtk_tearoff_menu_item_new ());
  gtk_widget_show_all (w_current->right_docking_area_menu);
}

void
x_menus_create_toolbar (GschemToplevel *w_current)
{
  SCM s_var = scm_module_variable (scm_current_module (),
                                   scm_from_utf8_symbol ("toolbar"));
  if (!scm_is_true (s_var)) {
    g_warning (_("No toolbar definition found\n"));
    return;
  }

  SCM s_toolbar = scm_variable_ref (s_var);
  if (scm_is_null (s_toolbar) || !scm_is_true (scm_list_p (s_toolbar))) {
    g_warning (_("Empty or malformed toolbar definition\n"));
    return;
  }

  w_current->toolbar = gtk_toolbar_new ();
  gtk_orientable_set_orientation (GTK_ORIENTABLE (w_current->toolbar),
                                  GTK_ORIENTATION_HORIZONTAL);
  gtk_toolbar_set_style (GTK_TOOLBAR (w_current->toolbar), GTK_TOOLBAR_ICONS);

  for (SCM l0 = s_toolbar; scm_is_pair (l0); l0 = scm_cdr (l0)) {
    SCM s_section = scm_car (l0);
    SCM_ASSERT (scm_is_true (scm_list_p (s_section)),
                s_section, SCM_ARGn, "x_menus_create_toolbar section");

    for (SCM l1 = s_section; scm_is_pair (l1); l1 = scm_cdr (l1)) {
      SCM s_item = scm_car (l1);
      SCM_ASSERT (scm_is_action (s_item),
                  s_item, SCM_ARGn, "x_menus_create_toolbar item");

      /* make sure the action won't ever be garbage collected
         since we're going to point to it from C data structures */
      scm_permanent_object (s_item);

      GschemAction *action = scm_to_action (s_item);
      GtkToolItem *button =
        gschem_action_create_tool_button (action, w_current);
      gtk_toolbar_insert (GTK_TOOLBAR (w_current->toolbar), button, -1);
    }

    if (scm_is_pair (scm_cdr (l0)))
      gtk_toolbar_insert (GTK_TOOLBAR (w_current->toolbar),
                          gtk_separator_tool_item_new (), -1);
  }

  gtk_widget_show_all (w_current->toolbar);

  /* activate 'select' button at start-up */
  i_update_toolbar (w_current);
}
