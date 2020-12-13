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
#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif
#ifdef HAVE_STRING_H
#include <string.h>
#endif

#include "gschem.h"
#include "../include/gschem_pagesel_dockable.h"
#include "actions.decl.x"


static void pagesel_popup_menu (GschemPageselDockable *pagesel,
                                GdkEventButton *event);
static void pagesel_update (GschemPageselDockable *pagesel);

static void pagesel_class_init (GschemPageselDockableClass *class);
static GtkWidget *pagesel_create_widget (GschemDockable *dockable);



/*! \brief Update the list and status of <B>toplevel</B>'s pages.
 *  \par Function Description
 *  Updates the list and status of <B>toplevel</B>\'s pages if the page
 *  manager dialog is opened.
 *
 *  \param [in] w_current  The GschemToplevel object to update.
 */
void x_pagesel_update (GschemToplevel *w_current)
{
  if (w_current->pagesel_dockable != NULL)
    pagesel_update (GSCHEM_PAGESEL_DOCKABLE (w_current->pagesel_dockable));

  i_update_filename (w_current);
}

enum {
  COLUMN_PAGE,
  COLUMN_BASENAME,
  COLUMN_PATH,
  COLUMN_CHANGED,
  NUM_COLUMNS
};


/*! \todo Finish function documentation!!!
 *  \brief
 *  \par Function Description
 *
 */
static void pagesel_callback_selection_changed (GtkTreeSelection *selection,
						gpointer user_data)
{
  GtkTreeModel *model;
  GtkTreeIter iter;
  GschemPageselDockable *pagesel = GSCHEM_PAGESEL_DOCKABLE (user_data);
  GschemToplevel *w_current;
  PAGE *page;

  if (!gtk_tree_selection_get_selected (selection, &model, &iter)) {
    return;
  }

  w_current = GSCHEM_DOCKABLE (pagesel)->w_current;
  gtk_tree_model_get (model, &iter,
                      COLUMN_PAGE, &page,
                      -1);

  x_window_set_current_page (w_current, page);
}

static void pagesel_callback_row_activated (GtkTreeView *tree_view,
                                            GtkTreePath *path,
                                            GtkTreeViewColumn *column,
                                            gpointer user_data)
{
  GschemDockable *dockable = GSCHEM_DOCKABLE (user_data);
  GschemToplevel *w_current = dockable->w_current;
  GtkTreeModel *model;
  GtkTreeIter iter;
  PAGE *page = NULL;

  model = gtk_tree_view_get_model (tree_view);
  gtk_tree_model_get_iter (model, &iter, path);
  gtk_tree_model_get (model, &iter, COLUMN_PAGE, &page, -1);

  x_window_set_current_page (w_current, page);

  if (gschem_dockable_get_state (dockable) == GSCHEM_DOCKABLE_STATE_DIALOG)
    gschem_dockable_hide (dockable);

  x_window_present (w_current);
  gtk_widget_grab_focus (w_current->drawing_area);
}

/*! \todo Finish function documentation!!!
 *  \brief
 *  \par Function Description
 *
 */
static gboolean pagesel_callback_button_pressed (GtkWidget *widget,
						 GdkEventButton *event,
						 gpointer user_data)
{
  GschemPageselDockable *pagesel = GSCHEM_PAGESEL_DOCKABLE (user_data);
  gboolean ret = FALSE;

  if (event->type == GDK_BUTTON_PRESS  &&  event->button == 3) {
    pagesel_popup_menu (pagesel, event);
    ret = TRUE;
  }

  return ret;
}

/*! \todo Finish function documentation!!!
 *  \brief
 *  \par Function Description
 *
 */
static gboolean pagesel_callback_popup_menu (GtkWidget *widget,
					     gpointer user_data)
{
  GschemPageselDockable *pagesel = GSCHEM_PAGESEL_DOCKABLE (user_data);

  pagesel_popup_menu (pagesel, NULL);
  
  return TRUE;
}

static void pagesel_callback_popup (GtkMenuItem *menuitem,
                                    gpointer user_data)
{
  gschem_action_activate (g_object_get_data (G_OBJECT (menuitem), "action"),
                          GSCHEM_DOCKABLE (user_data)->w_current);
}

/*! \brief Popup context-sensitive menu.
 *  \par Function Description
 *  Pops up a context-sensitive menu.
 *
 *  <B>event</B> can be NULL if the popup is triggered by a key binding
 *  instead of a mouse click.
 *
 *  \param [in] pagesel  The GschemPageselDockable object.
 *  \param [in] event    Mouse click event info.
 */
static void pagesel_popup_menu (GschemPageselDockable *pagesel,
                                GdkEventButton *event)
{
  GtkTreePath *path;
  GtkWidget *menu, *menuitem;
  struct menuitem_t {
    gchar *label;
    GschemAction *action;
  };
  struct menuitem_t menuitems[] = {
    { N_("New Page"),     action_file_new     },
    { N_("Open Page..."), action_file_open    },
    { "-",                NULL                },
    { N_("Save Page"),    action_file_save    },
    { N_("Close Page"),   action_page_close   },
    { NULL,               NULL                } };
  struct menuitem_t *tmp;
  
  if (event != NULL &&
      gtk_tree_view_get_path_at_pos (pagesel->treeview,
                                     (gint)event->x, 
                                     (gint)event->y,
                                     &path, NULL, NULL, NULL)) {
    GtkTreeSelection *selection;
    selection = gtk_tree_view_get_selection (pagesel->treeview);
    gtk_tree_selection_unselect_all (selection);
    gtk_tree_selection_select_path (selection, path);
    gtk_tree_path_free (path);
  }

  /* create the context menu */
  menu = gtk_menu_new();

  menuitem = gtk_image_menu_item_new_with_mnemonic (_("_Refresh List"));
  gtk_image_menu_item_set_image (
    GTK_IMAGE_MENU_ITEM (menuitem),
    gtk_image_new_from_stock (GTK_STOCK_REFRESH, GTK_ICON_SIZE_MENU));
  g_signal_connect_swapped (menuitem, "activate",
                            G_CALLBACK (pagesel_update), pagesel);
  gtk_menu_shell_append (GTK_MENU_SHELL (menu), menuitem);

  menuitem = gtk_separator_menu_item_new ();
  gtk_menu_shell_append (GTK_MENU_SHELL (menu), menuitem);

  for (tmp = menuitems; tmp->label != NULL; tmp++) {
    if (strcmp (tmp->label, "-") == 0) {
      menuitem = gtk_separator_menu_item_new ();
    } else {
      menuitem = gtk_menu_item_new_with_label (_(tmp->label));
      g_object_set_data (G_OBJECT (menuitem), "action", tmp->action);
      g_signal_connect (menuitem,
                        "activate",
                        G_CALLBACK (pagesel_callback_popup),
                        pagesel);
    }
    gtk_menu_shell_append (GTK_MENU_SHELL (menu), menuitem);
  }
  gtk_widget_show_all (menu);
  /* make menu a popup menu */
  gtk_menu_popup (GTK_MENU (menu), NULL, NULL, NULL, NULL,
                  (event != NULL) ? event->button : 0,
                  gdk_event_get_time ((GdkEvent*)event));
  
}


/*! \brief Handler for the notify::gschem-toplevel signal of GschemDialog
 *
 *  \par Function Description
 *
 *  When the gschem-toplevel property is set on the parent GschemDialog,
 *  we should update the pagesel dialog.
 *
 *  \param [in] gobject    the object which received the signal.
 *  \param [in] arg1      the GParamSpec of the property which changed
 *  \param [in] user_data  user data set when the signal handler was connected.
 */
static void notify_gschem_toplevel_cb (GObject    *gobject,
                                       GParamSpec *arg1,
                                       gpointer    user_data)
{
  GschemPageselDockable *pagesel = GSCHEM_PAGESEL_DOCKABLE (gobject);

  pagesel_update( pagesel );
}


/*! \todo Finish function documentation!!!
 *  \brief
 *  \par Function Description
 *
 */
GType gschem_pagesel_dockable_get_type()
{
  static GType pagesel_type = 0;
  
  if (!pagesel_type) {
    static const GTypeInfo pagesel_info = {
      sizeof (GschemPageselDockableClass),
      NULL, /* base_init */
      NULL, /* base_finalize */
      (GClassInitFunc) pagesel_class_init,
      NULL, /* class_finalize */
      NULL, /* class_data */
      sizeof (GschemPageselDockable),
      0,    /* n_preallocs */
      NULL  /* instance_init */
    };
		
    pagesel_type = g_type_register_static (GSCHEM_TYPE_DOCKABLE,
                                           "GschemPageselDockable",
                                           &pagesel_info, 0);
  }
  
  return pagesel_type;
}

/*! \todo Finish function documentation!!!
 *  \brief
 *  \par Function Description
 *
 */
static void pagesel_class_init (GschemPageselDockableClass *class)
{
  GSCHEM_DOCKABLE_CLASS (class)->create_widget = pagesel_create_widget;
}

/*! \todo Finish function documentation!!!
 *  \brief
 *  \par Function Description
 *
 */
static GtkWidget *pagesel_create_widget (GschemDockable *dockable)
{
  GschemPageselDockable *pagesel = GSCHEM_PAGESEL_DOCKABLE (dockable);
  GtkWidget *scrolled_win, *treeview;
  GtkTreeModel *store;
  GtkCellRenderer *renderer;
  GtkTreeViewColumn *column;
  GtkTreeSelection *selection;

  /* create the model for the treeview */
  store = (GtkTreeModel*)gtk_tree_store_new (NUM_COLUMNS,
                                             G_TYPE_POINTER,  /* page */
                                             G_TYPE_STRING,   /* basename */
                                             G_TYPE_STRING,   /* path */
                                             G_TYPE_BOOLEAN); /* changed */

  /* create a scrolled window for the treeview */
  scrolled_win = GTK_WIDGET (
    g_object_new (GTK_TYPE_SCROLLED_WINDOW,
                  /* GtkContainer */
                  "border-width",      0,
                  /* GtkScrolledWindow */
                  "hscrollbar-policy", GTK_POLICY_AUTOMATIC,
                  "vscrollbar-policy", GTK_POLICY_ALWAYS,
                  NULL));
  /* create the treeview */
  treeview = GTK_WIDGET (g_object_new (GTK_TYPE_TREE_VIEW,
                                       /* GtkTreeView */
                                       "model",      store,
                                       "tooltip-column", COLUMN_PATH,
                                       NULL));
  g_signal_connect (treeview,
                    "row-activated",
                    G_CALLBACK (pagesel_callback_row_activated),
                    pagesel);
  g_signal_connect (treeview,
                    "button-press-event",
                    G_CALLBACK (pagesel_callback_button_pressed),
                    pagesel);
  g_signal_connect (treeview,
                    "popup-menu",
                    G_CALLBACK (pagesel_callback_popup_menu),
                    pagesel);
  selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (treeview));
  gtk_tree_selection_set_mode (selection,
                               GTK_SELECTION_SINGLE);
  g_signal_connect (selection,
                    "changed",
                    G_CALLBACK (pagesel_callback_selection_changed),
                    pagesel); 
  /*   - first column: changed */
  renderer = GTK_CELL_RENDERER (
    g_object_new (GTK_TYPE_CELL_RENDERER_TEXT,
                  /* GtkCellRendererText */
                  "editable", FALSE,
                  "text", "*",
                  /* GtkCellRenderer */
                  "xalign", .5,
                  NULL));
  column = GTK_TREE_VIEW_COLUMN (
    g_object_new (GTK_TYPE_TREE_VIEW_COLUMN,
                  /* GtkTreeViewColumn */
                  "title", _("Chg"),
                  NULL));
  gtk_tree_view_column_pack_start (column, renderer, TRUE);
  gtk_tree_view_column_add_attribute (column, renderer, "visible", COLUMN_CHANGED);
  gtk_tree_view_append_column (GTK_TREE_VIEW (treeview), column);
  /*   - second column: page name */
  renderer = GTK_CELL_RENDERER (
    g_object_new (GTK_TYPE_CELL_RENDERER_TEXT,
                  /* GtkCellRendererText */
                  "editable", FALSE,
                  NULL));
  column = GTK_TREE_VIEW_COLUMN (
    g_object_new (GTK_TYPE_TREE_VIEW_COLUMN,
                  /* GtkTreeViewColumn */
                  "title", _("Filename"),
                  NULL));
  gtk_tree_view_column_pack_start (column, renderer, TRUE);
  gtk_tree_view_column_add_attribute (column, renderer, "text", COLUMN_BASENAME);
  gtk_tree_view_append_column (GTK_TREE_VIEW (treeview), column);
  gtk_tree_view_set_expander_column (GTK_TREE_VIEW (treeview), column);
      
  /* add the treeview to the scrolled window */
  gtk_container_add (GTK_CONTAINER (scrolled_win), treeview);
  /* set treeview of pagesel */
  pagesel->treeview = GTK_TREE_VIEW (treeview);

  /* add the scrolled window to the dialog vbox */
  gtk_widget_show_all (scrolled_win);

  g_signal_connect( pagesel, "notify::gschem-toplevel",
                    G_CALLBACK( notify_gschem_toplevel_cb ), NULL );

  pagesel_update (pagesel);
  return scrolled_win;
}


/*! \brief Update tree model of <B>pagesel</B>'s treeview.
 *  \par Function Description
 *  Updates the tree model of <B>pagesel</B>\'s treeview.
 *
 *  Right now, each time it is called, it rebuilds all the model from the
 *  list of pages passed in.
 *  It is a recursive function to populate the tree store
 *
 *  \param [in] model   GtkTreeModel to update.
 *  \param [in] parent  GtkTreeIter pointer to tree root.
 *  \param [in] pages   GedaPageList of pages for this toplevel.
 *  \param [in] page    The PAGE object to update tree model from.
 */
static void add_page (GtkTreeModel *model, GtkTreeIter *parent,
                      GedaPageList *pages, PAGE *page)
{
  GtkTreeIter iter;
  PAGE *p_current;
  GList *p_iter;

  /* add the page to the store */
  gtk_tree_store_append (GTK_TREE_STORE (model),
                         &iter,
                         parent);

  if (page->is_untitled)
    gtk_tree_store_set (GTK_TREE_STORE (model),
                        &iter,
                        COLUMN_PAGE, page,
                        COLUMN_BASENAME, _("(untitled page)"),
                        COLUMN_PATH, NULL,
                        COLUMN_CHANGED, page->CHANGED,
                        -1);
  else {
    /* get basename and abbreviated path */
    gchar *basename = g_path_get_basename (page->page_filename);
    gchar *path = NULL;
    const gchar *homedir = g_get_home_dir ();
    size_t hd_len = strlen (homedir);
    if (hd_len > 1 && strncmp (page->page_filename, homedir, hd_len) == 0 &&
        (homedir[hd_len - 1] == '/' || page->page_filename[hd_len] == '/'))
      path = g_strdup_printf (
        "~/%s", page->page_filename + hd_len + (homedir[hd_len - 1] != '/'));

    gtk_tree_store_set (GTK_TREE_STORE (model),
                        &iter,
                        COLUMN_PAGE, page,
                        COLUMN_BASENAME, basename,
                        COLUMN_PATH, path != NULL ? path : page->page_filename,
                        COLUMN_CHANGED, page->CHANGED,
                        -1);

    g_free (path);
    g_free (basename);
  }

  /* search a page that has a up field == p_current->pid */
  for ( p_iter = geda_list_get_glist( pages );
        p_iter != NULL;
        p_iter = g_list_next( p_iter ) ) {

    p_current = (PAGE *)p_iter->data;
    if (p_current->up == page->pid) {
      add_page (model, &iter, pages, p_current);
    }
  }
}

/*! \todo Finish function documentation!!!
 *  \brief
 *  \par Function Description
 *  Recursive function to select the current page in the treeview
 *
 */
static void select_page(GtkTreeView *treeview,
			GtkTreeIter *parent, PAGE *page)
{
  GtkTreeModel *treemodel = gtk_tree_view_get_model (treeview);
  GtkTreeIter iter;
  PAGE *p_current;

  if (!gtk_tree_model_iter_children (treemodel, &iter, parent)) {
    return;
  }

  do {
    gtk_tree_model_get (treemodel, &iter,
                        COLUMN_PAGE, &p_current,
                        -1);
    if (p_current == page) {
      gtk_tree_view_expand_all (treeview);
      gtk_tree_selection_select_iter (
        gtk_tree_view_get_selection (treeview),
        &iter);
      return;
    }

    select_page (treeview, &iter, page);
    
  } while (gtk_tree_model_iter_next (treemodel, &iter));
  
}

/*! \todo Finish function documentation!!!
 *  \brief
 *  \par Function Description
 *
 */
static void pagesel_update (GschemPageselDockable *pagesel)
{
  GtkTreeModel *model;
  TOPLEVEL *toplevel;
  PAGE *p_current;
  GList *iter;

  g_return_if_fail (GSCHEM_IS_PAGESEL_DOCKABLE (pagesel));

  if (GSCHEM_PAGESEL_DOCKABLE (pagesel)->treeview == NULL)
    return;

  g_return_if_fail (GSCHEM_DOCKABLE (pagesel)->w_current);

  toplevel = gschem_toplevel_get_toplevel (GSCHEM_DOCKABLE (pagesel)->w_current);
  model    = gtk_tree_view_get_model (pagesel->treeview);

  /* wipe out every thing in the store */
  gtk_tree_store_clear (GTK_TREE_STORE (model));
  /* now rebuild */
  for ( iter = geda_list_get_glist( toplevel->pages );
        iter != NULL;
        iter = g_list_next( iter ) ) {

    p_current = (PAGE *)iter->data;
    /* find every page that is not a hierarchy-down of another page */
    if (p_current->up < 0 ||
        s_page_search_by_page_id (toplevel->pages,
                                  p_current->up) == NULL) {
      add_page (model, NULL, toplevel->pages, p_current);
    }
  }

  /* select the current page in the treeview */
  select_page (pagesel->treeview, NULL, toplevel->page_current);  
}

