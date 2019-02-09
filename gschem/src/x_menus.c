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

struct PopupEntry {
  const gchar const *name;
  GschemAction **action;
  const gchar const *stock_id;
};

static struct PopupEntry popup_items[] = {
  { N_("Add Net"), &action_add_net, "insert-net" },
  { N_("Add Attribute"), &action_add_attribute, "insert-attribute" },
  { N_("Add Component"), &action_add_component, "insert-symbol" },
  { N_("Add Bus"), &action_add_bus, "insert-bus" },
  { N_("Add Text"), &action_add_text, "insert-text" },
  { "SEPARATOR", NULL, NULL },
  { N_("Zoom In"), &action_view_zoom_in, "gtk-zoom-in" },
  { N_("Zoom Out"), &action_view_zoom_out, "gtk-zoom-out" },
  { N_("Zoom Box"), &action_view_zoom_box, NULL },
  { N_("Zoom Extents"), &action_view_zoom_extents, "gtk-zoom-fit" },
  { "SEPARATOR", NULL, NULL },
  { N_("Select"), &action_edit_select, "select" },
  { N_("Edit..."), &action_edit_edit, NULL },
  { N_("Edit Pin Type..."), &action_edit_pin_type, NULL },
  { N_("Copy"), &action_edit_copy, "clone" },
  { N_("Move"), &action_edit_move, NULL },
  { N_("Delete"), &action_edit_delete, "gtk-delete" },
  { "SEPARATOR", NULL, NULL },
  { N_("Down Schematic"), &action_hierarchy_down_schematic, "gtk-go-down" },
  { N_("Down Symbol"), &action_hierarchy_down_symbol, "gtk-go-bottom" },
  { N_("Up"), &action_hierarchy_up, "gtk-go-up" },

  { NULL, NULL, NULL }, /* Guard */
};


/*! \todo Finish function documentation!!!
 *  \brief
 *  \par Function Description
 *
 */
static void
g_menu_execute (GtkMenuItem *menu_item, gpointer user_data)
{
  GschemAction *action = g_object_get_data (G_OBJECT (menu_item), "action");
  GschemToplevel *w_current = (GschemToplevel *) user_data;
  gschem_action_activate (action, w_current);
}

/*! \todo Finish function documentation!!!
 *  \brief
 *  \par Function Description
 *
 */
GtkWidget *
get_main_menu(GschemToplevel *w_current)
{
  char *buf;
  GtkWidget *menu_item;
  GtkWidget *root_menu;
  GtkWidget *menu_bar;
  GtkWidget *menu;
  int scm_items_len;
  SCM scm_items;
  SCM scm_item;
  SCM scm_item_name;
  SCM scm_item_func;
  SCM scm_item_stock;
  SCM scm_index;
  SCM scm_keys;
  char *menu_name;
  char **raw_menu_name = g_malloc (sizeof(char *));
  char *menu_item_name;
  char *raw_menu_item_name;
  char *menu_item_stock;
  char *menu_item_keys;
  int i, j;

  menu_bar = gtk_menu_bar_new ();

  scm_dynwind_begin (0);
  g_dynwind_window (w_current);
  /*! \bug This function may leak memory if there is a non-local exit
   * in Guile code. At some point, unwind handlers need to be added to
   * clean up heap-allocated strings. */

  for (i = 0 ; i < s_menu_return_num(); i++) {
    
    scm_items = s_menu_return_entry(i, raw_menu_name);   
    if (*raw_menu_name == NULL) {
      fprintf(stderr, "Oops.. got a NULL menu name in get_main_menu()\n");
      exit(-1);
    }

    menu = gtk_menu_new();

    menu_item = gtk_tearoff_menu_item_new ();
    gtk_menu_shell_append (GTK_MENU_SHELL (menu), menu_item);
    gtk_widget_show(menu_item);

    scm_items_len = (int) scm_ilength (scm_items);
    for (j = 0 ; j < scm_items_len; j++) {

      scm_index = scm_from_int (j);
      scm_item = scm_list_ref (scm_items, scm_index);
      scm_item_name = SCM_CAR (scm_item);
      scm_item_func = SCM_CADR (scm_item);
      scm_item_stock = scm_is_pair (SCM_CDDR (scm_item)) ?
                         SCM_CADDR (scm_item) : SCM_BOOL_F;
      SCM_ASSERT(scm_is_string(scm_item_name), scm_item_name, SCM_ARGn, "get_main_menu item_name");
      SCM_ASSERT(scm_is_symbol (scm_item_func) ||
                    scm_is_false (scm_item_func),
                 scm_item_func, SCM_ARGn, "get_main_menu item_func");
      SCM_ASSERT (scm_is_string (scm_item_stock) ||
                    scm_is_false (scm_item_stock),
                  scm_item_stock, SCM_ARGn, "get_main_menu stock");

      raw_menu_item_name = scm_to_utf8_string(scm_item_name);
      scm_dynwind_begin(0);
      scm_dynwind_free(raw_menu_item_name);

      menu_item_name = (char *) gettext(raw_menu_item_name);

      if (strcmp(menu_item_name, "SEPARATOR") == 0) {
        menu_item = gtk_menu_item_new();
        gtk_menu_shell_append (GTK_MENU_SHELL (menu), menu_item);
      } else {

          GtkStockItem stock_info;

          SCM s_action = scm_primitive_eval (scm_item_func);

          /* make sure the action won't ever be garbage collected
             since we're going to point to it from C data structures */
          scm_permanent_object (s_action);

          GschemAction *action = scm_to_action (s_action);

          /* Look up key binding in global keymap */
          SCM s_expr =
            scm_list_2 (scm_from_utf8_symbol ("find-key"),
                        s_action);

          scm_keys = g_scm_eval_protected (s_expr, scm_interaction_environment ());

          if (scm_is_false (scm_keys)) {
            menu_item_keys = "";
          } else {
            menu_item_keys = scm_to_utf8_string (scm_keys);
            scm_dynwind_free(menu_item_keys);
          }

          if (scm_is_false (scm_item_stock))
            menu_item_stock = NULL;
          else
            menu_item_stock = scm_to_utf8_string (scm_item_stock);

          menu_item = g_object_new (GTK_TYPE_IMAGE_MENU_ITEM, NULL);

          /* use custom label widget */
          GtkWidget *label = g_object_new (GSCHEM_TYPE_ACCEL_LABEL, NULL);
          gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
          gtk_label_set_use_underline (GTK_LABEL (label), TRUE);
          gtk_label_set_label (GTK_LABEL (label), menu_item_name);
          gschem_accel_label_set_accel_string (GSCHEM_ACCEL_LABEL (label),
                                               menu_item_keys);
          gtk_container_add (GTK_CONTAINER (menu_item), label);
          gtk_widget_show (label);

          /* set icon */
          if (menu_item_stock) {
            GtkWidget *image = gtk_image_new ();
            gtk_widget_show (image);
            gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM (menu_item),
                                           image);

            /* If stock name corresponds to a GTK stock item, then use it.
               Otherwise, look it up in the icon theme. */
            if (gtk_stock_lookup (menu_item_stock, &stock_info))
              gtk_image_set_from_stock (GTK_IMAGE (image), menu_item_stock,
                                        GTK_ICON_SIZE_MENU);
            else
              gtk_image_set_from_icon_name (GTK_IMAGE (image), menu_item_stock,
                                            GTK_ICON_SIZE_MENU);
          }
          free(menu_item_stock);

          if (action == action_file_open_recent)
            gtk_menu_item_set_submenu (GTK_MENU_ITEM (menu_item),
                                       w_current->recent_chooser_menu);
          else if (action == action_docking_area_left)
            gtk_menu_item_set_submenu (GTK_MENU_ITEM (menu_item),
                                       w_current->left_docking_area_menu);
          else if (action == action_docking_area_bottom)
            gtk_menu_item_set_submenu (GTK_MENU_ITEM (menu_item),
                                       w_current->bottom_docking_area_menu);
          else if (action == action_docking_area_right)
            gtk_menu_item_set_submenu (GTK_MENU_ITEM (menu_item),
                                       w_current->right_docking_area_menu);

          g_object_set_data (G_OBJECT (menu_item), "action", action);
          g_signal_connect (G_OBJECT (menu_item), "activate",
                            G_CALLBACK (g_menu_execute), w_current);

        gtk_menu_shell_append (GTK_MENU_SHELL (menu), menu_item);
      }

      gtk_widget_show (menu_item);

      /* add a handle to the menu_bar object to get access to widget objects */
      /* This string should NOT be internationalized */
      buf = g_strdup_printf("%s/%s", *raw_menu_name, raw_menu_item_name);
      g_object_set_data (G_OBJECT (menu_bar), buf, menu_item);
      g_free(buf);

      scm_dynwind_end();
    }
    
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
  GtkAction *action;
  GtkWidget *menu_item;
  GtkWidget *menu;
  GtkStockItem stock_info;
  int i;

  menu = gtk_menu_new ();

  for (i = 0; popup_items[i].name != NULL; i++) {
    struct PopupEntry e = popup_items[i];

    /* No action --> add a separator */
    if (e.action == NULL) {
      menu_item = gtk_menu_item_new();
      gtk_widget_show (menu_item);
      gtk_menu_shell_append (GTK_MENU_SHELL (menu), menu_item);
      continue;
    }

    /* Don't bother showing keybindings in the popup menu */
    action = g_object_new (GTK_TYPE_ACTION,
                           "label", gettext (e.name),
                           "tooltip", gettext (e.name),
                           NULL);
    /* If there's a matching stock item, use it. Otherwise lookup the
       name in the icon theme. */
    if (e.stock_id != NULL && gtk_stock_lookup (e.stock_id, &stock_info)) {
      gtk_action_set_stock_id (GTK_ACTION (action), e.stock_id);
    } else {
      gtk_action_set_icon_name (GTK_ACTION (action), e.stock_id);
    }

    menu_item = gtk_action_create_menu_item (GTK_ACTION (action));
    gtk_menu_shell_append (GTK_MENU_SHELL (menu), menu_item);

    /* Connect things up so that the actions get run */
    g_object_set_data (G_OBJECT (menu_item), "action", *e.action);
    g_signal_connect (G_OBJECT (menu_item), "activate",
                      G_CALLBACK (g_menu_execute), w_current);

    /* Add a handle to the menu object to get access to widget
       objects. Horrible horrible hack, but it's the same approach as
       taken for the main menu bar. :-( */
    g_object_set_data (G_OBJECT (menu), e.name, menu_item);
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

/*! \todo Finish function documentation!!!
 *  \brief
 *  \par Function Description
 *
 */
void x_menus_sensitivity (GschemToplevel *w_current, const char *buf, int flag)
{
  GtkWidget* item=NULL;
  
  if (!buf) {
    return;
  }

  if (!w_current->menubar) {
    return;
  }
  
  item = (GtkWidget *) g_object_get_data (G_OBJECT (w_current->menubar), buf);

  if (item) {
    gtk_widget_set_sensitive(GTK_WIDGET(item), flag);
    /* free(item); */ /* Why doesn't this need to be freed?  */
  } else {
    s_log_message(_("Tried to set the sensitivity on non-existent menu item '%s'\n"), buf); 
  }
 
}

/*! \todo Finish function documentation!!!
 *  \brief
 *  \par Function Description
 *  This function sets the sensitivity of the items in the right button
 *  popup.
 */
void x_menus_popup_sensitivity (GschemToplevel *w_current, const char *buf, int flag)
{
  GtkWidget *item;

  g_assert (w_current);
  g_assert (buf);
  g_assert (w_current->popup_menu);

  item = GTK_WIDGET (g_object_get_data (G_OBJECT (w_current->popup_menu), buf));

  if (item) {
    gtk_widget_set_sensitive (item, flag);
  } else {
    g_critical (_("Tried to set the sensitivity on non-existent menu item '%s'\n"),
                buf);
  }
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
