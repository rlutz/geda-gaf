/* gEDA - GPL Electronic Design Automation
 * gschem - gEDA Schematic Capture
 * Copyright (C) 1998-2010 Ales Hvezda
 * Copyright (C) 1998-2013 gEDA Contributors (see ChangeLog for details)
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
#include "actions.decl.x"

#include <glib/gstdio.h>

static GschemAction **popup_items[] = {
  &action_add_net,
  &action_add_attribute,
  &action_add_component,
  &action_add_bus,
  &action_add_text,
  NULL,
  &action_view_zoom_in,
  &action_view_zoom_out,
  &action_view_zoom_box,
  &action_view_zoom_extents,
  NULL,
  &action_edit_select,
  &action_edit_edit,
  &action_edit_pin_type,
  &action_edit_copy,
  &action_edit_move,
  &action_edit_delete,
  NULL,
  &action_hierarchy_down_schematic,
  &action_hierarchy_down_symbol,
  &action_hierarchy_up,

  NULL, NULL /* Guard */
};


static GtkWidget *
build_menu (SCM s_menu, GschemToplevel *w_current)
{
  GtkWidget *menu = gtk_menu_new ();

  GtkWidget *tearoff_menu_item = gtk_tearoff_menu_item_new ();
  gtk_menu_shell_append (GTK_MENU_SHELL (menu), tearoff_menu_item);
  gtk_widget_show (tearoff_menu_item);

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
        gschem_action_create_menu_item (action, TRUE, w_current);
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
GtkWidget *
get_main_menu(GschemToplevel *w_current)
{
  GtkWidget *root_menu;
  GtkWidget *menu_bar;
  GtkWidget *menu;
  char *menu_name;
  char **raw_menu_name = g_malloc (sizeof(char *));
  int i;

  menu_bar = gtk_menu_bar_new ();

  scm_dynwind_begin (0);
  g_dynwind_window (w_current);
  /*! \bug This function may leak memory if there is a non-local exit
   * in Guile code. At some point, unwind handlers need to be added to
   * clean up heap-allocated strings. */

  for (i = 0 ; i < s_menu_return_num(); i++) {
    
    if (*raw_menu_name == NULL) {
      fprintf(stderr, "Oops.. got a NULL menu name in get_main_menu()\n");
      exit(-1);
    }

    menu = build_menu (s_menu_return_entry (i, raw_menu_name), w_current);

    menu_name = (char *) gettext(*raw_menu_name);
    root_menu = gtk_menu_item_new_with_mnemonic (menu_name);
    /* do not free *raw_menu_name */

    /* no longer right justify the help menu since that has gone out of style */

    gtk_widget_show (root_menu);
    gtk_menu_item_set_submenu (GTK_MENU_ITEM (root_menu), menu);
    gtk_menu_shell_append (GTK_MENU_SHELL (menu_bar), root_menu);

    /* store lower-case raw name without underscores as settings name
       for saving/restoring torn-off menus */
    gchar *settings_name = g_ascii_strdown (*raw_menu_name, -1);
    gchar *i = settings_name, *j = settings_name;
    do {
      if (*i != '_')
        *j++ = *i;
    } while (*i++ != '\0');
    g_object_set_data (G_OBJECT (menu), "settings-name", settings_name);
  }
  scm_dynwind_end ();

  g_free(raw_menu_name);
  return menu_bar;
}

GtkWidget *
get_main_popup (GschemToplevel *w_current)
{
  GtkWidget *menu_item;
  GtkWidget *menu;
  int i;

  menu = gtk_menu_new ();

  for (i = 0; popup_items[i] != NULL || popup_items[i + 1] != NULL; i++) {
    GschemAction *action;

    /* No action --> add a separator */
    if (popup_items[i] == NULL) {
      menu_item = gtk_menu_item_new();
      gtk_widget_show (menu_item);
      gtk_menu_shell_append (GTK_MENU_SHELL (menu), menu_item);
      continue;
    }
    action = *popup_items[i];

    menu_item = gschem_action_create_menu_item (action, FALSE, w_current);
    gtk_menu_shell_append (GTK_MENU_SHELL (menu), menu_item);
    gtk_widget_show (menu_item);
  }

  return menu;
}

/*! \todo Finish function documentation!!!
 *  \brief
 *  \par Function Description
 *
 *  \note
 *  need to look at this... here and the setup
 */
gint do_popup (GschemToplevel *w_current, GdkEventButton *event)
{
  GtkWidget *menu = (GtkWidget *) w_current->popup_menu;
  g_return_val_if_fail (menu != NULL, FALSE);

  gtk_menu_popup (GTK_MENU (menu), NULL, NULL, NULL, NULL,
                  event->button, event->time);

  return FALSE;
}

#define MAX_RECENT_FILES 10
/*! \brief Callback for recent-chooser.
 *
 * Will be called if element of recent-file-list is activated
 */
void
recent_chooser_item_activated (GtkRecentChooser *chooser, GschemToplevel *w_current)
{
  PAGE *page;
  gchar *uri;
  gchar *filename;

  uri = gtk_recent_chooser_get_current_uri (chooser);
  filename = g_filename_from_uri(uri, NULL, NULL);
  gtk_recent_manager_add_item(recent_manager, uri);
  page = x_window_open_page(w_current, (char *)filename);
  x_window_set_current_page(w_current, page);

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
