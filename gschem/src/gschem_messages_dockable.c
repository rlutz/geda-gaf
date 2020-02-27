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

/*! \file gschem_messages_dockable.c
 * \brief Report problems with the current schematic or symbol.
 */

#include <config.h>

#include <stdio.h>
#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif
#ifdef HAVE_STRING_H
#include <string.h>
#endif

#include <math.h>  /* for floor(3) */

#include "gschem.h"
#include "../include/gschem_messages_dockable.h"


typedef enum {
  type_symversion,
  type_symversion_error,
  type_other = -1
} GschemMessageType;

typedef enum {
  severity_error,
  severity_warning,
  severity_message,
  severity_supplemental_text
} GschemMessageSeverity;

enum {
  COLUMN_TYPE,
  COLUMN_SEVERITY,
  COLUMN_FILENAME,
  COLUMN_REFDES,
  COLUMN_MESSAGE,
  COLUMN_TOOLTIP,
  COLUMN_OBJECT,
  COLUMN_REMOVED,
  N_COLUMNS
};


static gpointer parent_class = NULL;

static void class_init (GschemMessagesDockableClass *class);
static void instance_init (GschemMessagesDockable *messages_dockable);
static void dispose (GObject *object);

static GtkWidget *create_widget (GschemDockable *dockable);

static void clear_weak_refs (GschemMessagesDockable *messages_dockable);


GType
gschem_messages_dockable_get_type ()
{
  static GType type = 0;

  if (type == 0) {
    static const GTypeInfo info = {
      sizeof (GschemMessagesDockableClass),
      NULL,                                     /* base_init */
      NULL,                                     /* base_finalize */
      (GClassInitFunc) class_init,
      NULL,                                     /* class_finalize */
      NULL,                                     /* class_data */
      sizeof (GschemMessagesDockable),
      0,                                        /* n_preallocs */
      (GInstanceInitFunc) instance_init,
      NULL                                      /* value_table */
    };

    type = g_type_register_static (GSCHEM_TYPE_DOCKABLE,
                                   "GschemMessagesDockable",
                                   &info, 0);
  }

  return type;
}


static void
class_init (GschemMessagesDockableClass *class)
{
  parent_class = g_type_class_peek_parent (class);

  G_OBJECT_CLASS (class)->dispose = dispose;
  GSCHEM_DOCKABLE_CLASS (class)->create_widget = create_widget;
}


static void
instance_init (GschemMessagesDockable *messages_dockable)
{
  messages_dockable->store = gtk_list_store_new (
    N_COLUMNS,
    G_TYPE_INT,       /* type */
    G_TYPE_INT,       /* severity */
    G_TYPE_STRING,    /* filename */
    G_TYPE_STRING,    /* refdes */
    G_TYPE_STRING,    /* message */
    G_TYPE_STRING,    /* tooltip */
    G_TYPE_POINTER,   /* object */
    G_TYPE_BOOLEAN);  /* removed */
}


static void
dispose (GObject *object)
{
  clear_weak_refs (GSCHEM_MESSAGES_DOCKABLE (object));
  g_clear_object (&GSCHEM_MESSAGES_DOCKABLE (object)->store);

  G_OBJECT_CLASS (parent_class)->dispose (object);
}


static void
weakref_notify_func (void *dead_obj, void *user_data)
{
  GschemMessagesDockable *messages_dockable =
    GSCHEM_MESSAGES_DOCKABLE (user_data);
  GtkTreeModel *model = GTK_TREE_MODEL (messages_dockable->store);
  GtkTreeIter iter;

  g_return_if_fail (dead_obj != NULL);

  /* clear the 'object' field in all matching messages */
  if (gtk_tree_model_get_iter_first (model, &iter))
    do {
      OBJECT *obj;
      gtk_tree_model_get (model, &iter,
                          COLUMN_OBJECT, &obj,
                          -1);
      if (obj == dead_obj)
        gtk_list_store_set (GTK_LIST_STORE (model), &iter,
                            COLUMN_OBJECT, NULL,
                            COLUMN_REMOVED, TRUE,
                            COLUMN_TOOLTIP,
                              _("<i>Press F5 to update messages</i>"),
                            -1);
    } while (gtk_tree_model_iter_next (model, &iter));

  /* don't remove the weak reference because this would mess up
     weakref invocation (and happens automatically, anyway) */
}


static void
add_message (GschemMessagesDockable *messages_dockable,
             gint type, gint severity,
             const gchar *filename, const gchar *refdes, OBJECT *obj,
             const gchar *tooltip,
             const gchar *format, ...)
{
  va_list args;
  gchar *msg;
  GtkTreeIter iter;

  va_start (args, format);
  msg = g_strdup_vprintf (format, args);
  va_end (args);

  gtk_list_store_append (messages_dockable->store, &iter);
  gtk_list_store_set (messages_dockable->store, &iter,
                      COLUMN_TYPE, type,
                      COLUMN_SEVERITY, severity,
                      COLUMN_FILENAME, filename,
                      COLUMN_REFDES, refdes,
                      COLUMN_OBJECT, obj,
                      COLUMN_MESSAGE, msg,
                      COLUMN_TOOLTIP, tooltip,
                      -1);

  g_free (msg);

  if (obj != NULL)
    /* duplicate weakrefs don't hurt, so don't bother checking */
    s_object_weak_ref (obj, weakref_notify_func, messages_dockable);
}


static void
clear_weak_refs (GschemMessagesDockable *messages_dockable)
{
  GtkTreeModel *model = GTK_TREE_MODEL (messages_dockable->store);
  GtkTreeIter iter;

  if (gtk_tree_model_get_iter_first (model, &iter))
    do {
      OBJECT *obj;
      gtk_tree_model_get (model, &iter,
                          COLUMN_OBJECT, &obj,
                          -1);
      if (obj != NULL)
        s_object_weak_unref (obj, weakref_notify_func, messages_dockable);
    } while (gtk_tree_model_iter_next (model, &iter));
}


static void
goto_object (GschemToplevel *w_current, OBJECT *obj)
{
  TOPLEVEL *toplevel = gschem_toplevel_get_toplevel (w_current);
  int x0, y0, x1, y1;
  GschemPageView *page_view;

  g_return_if_fail (obj != NULL);

  g_return_if_fail (world_get_single_object_bounds (
                      toplevel, obj, &x0, &y0, &x1, &y1));

  page_view = gschem_toplevel_get_current_page_view (w_current);
  g_return_if_fail (page_view != NULL);
  gschem_page_view_pan (page_view, (x0 + x1) / 2, (y0 + y1) / 2);
}


static void
select_object (GschemToplevel *w_current, OBJECT *obj)
{
  TOPLEVEL *toplevel = gschem_toplevel_get_toplevel (w_current);
  SELECTION *selection = toplevel->page_current->selection_list;

  g_return_if_fail (obj != NULL);

  o_redraw_cleanstates (w_current);
  o_select_unselect_all (w_current);

  g_run_hook_object (w_current, "%select-objects-hook", obj);
  o_selection_add (toplevel, selection, obj);
  o_attrib_add_selected (w_current, selection, obj);

  i_set_state (w_current, SELECT);
  i_action_stop (w_current);
  i_update_menus (w_current);
}


static void
confirm_object_symversion (GschemToplevel *w_current, OBJECT *object)
{
  TOPLEVEL *toplevel = gschem_toplevel_get_toplevel (w_current);

  OBJECT *inherited;
  gboolean symversion_exists;
  gboolean changed;

  g_return_if_fail (object != NULL);
  g_return_if_fail (object->complex != NULL);
  g_return_if_fail (object->complex->prim_objs != NULL);
  g_return_if_fail (object->page != NULL);

  /* find inherited symversion= attribute */

  inherited = NULL;

  for (const GList *l = object->complex->prim_objs; l != NULL; l = l->next) {
    OBJECT *o_current = l->data;
    gchar *name;
    gboolean is_symversion;

    /* skip non-text objects, attached attributes, and text which doesn't
       constitute a valid attribute (e.g. general text placed on the page) */
    if (o_current->type != OBJ_TEXT ||
        o_current->attached_to != NULL ||
        !o_attrib_get_name_value (o_current, &name, NULL))
      continue;

    is_symversion = strcmp (name, "symversion") == 0;
    g_free (name);

    if (is_symversion) {
      inherited = o_current;
      break;
    }
  }

  if (inherited == NULL) {
    s_log_message (_("Can't find inherited symversion attribute\n"));
    return;
  }

  /* search for and update existing symversion= attribute */

  symversion_exists = FALSE;
  changed = FALSE;

  for (const GList *l = object->attribs; l != NULL; l = l->next) {
    OBJECT *attrib = l->data;
    char *name;

    if (attrib->type != OBJ_TEXT ||
        !o_attrib_get_name_value (attrib, &name, NULL))
      continue;

    if (strcmp (name, "symversion") == 0) {
      if (strcmp (attrib->text->string, inherited->text->string) != 0) {
        o_text_change (w_current, attrib, inherited->text->string,
                       attrib->visibility, attrib->show_name_value);
        changed = TRUE;
      }
      symversion_exists = TRUE;
    }

    g_free (name);
  }

  /* promote symversion= attribute if it doesn't exist yet */

  if (!symversion_exists) {
    OBJECT *o_new = o_object_copy (toplevel, inherited);
    o_set_visibility (toplevel, o_new, VISIBLE);
    s_page_append (toplevel, object->page, o_new);
    o_attrib_attach (toplevel, o_new, object, TRUE);
    g_run_hook_object (w_current, "%add-objects-hook", o_new);

    SELECTION *selection = object->page->selection_list;
    if (g_list_find (geda_list_get_glist (selection), object) != NULL) {
      g_run_hook_object (w_current, "%select-objects-hook", o_new);
      o_selection_add (toplevel, selection, o_new);
      o_attrib_add_selected (w_current, selection, o_new);
    }

    changed = TRUE;
  }

  if (changed) {
    gschem_toplevel_page_content_changed (w_current, object->page);
    o_undo_savestate_old (w_current, UNDO_ALL, _("Confirm Symbol Version"));
  }
}


/******************************************************************************/


static void
icon_cell_data_func (GtkTreeViewColumn *tree_column, GtkCellRenderer *cell,
                     GtkTreeModel *tree_model, GtkTreeIter *iter,
                     gpointer data)
{
  gint severity;
  gtk_tree_model_get (tree_model, iter,
                      COLUMN_SEVERITY, &severity,
                      -1);

  g_object_set (cell,
                "icon-name",
                  severity == severity_error ? GTK_STOCK_DIALOG_ERROR :
                  severity == severity_warning ? GTK_STOCK_DIALOG_WARNING :
                  severity == severity_message ? GTK_STOCK_DIALOG_INFO : "",
                NULL);
}


static void
tree_selection_changed (GtkTreeSelection *selection, gpointer user_data)
{
  GtkTreeModel *model;
  GtkTreeIter iter;
  OBJECT *obj;

  if (!gtk_tree_selection_get_selected (selection, &model, &iter))
    return;

  gtk_tree_model_get (model, &iter,
                      COLUMN_OBJECT, &obj,
                      -1);

  if (obj != NULL)
    goto_object (GSCHEM_DOCKABLE (user_data)->w_current, obj);
}


static void
tree_view_row_activated (GtkTreeView *tree_view, GtkTreePath *path,
                         GtkTreeViewColumn *column, gpointer user_data)
{
  GtkTreeModel *model = gtk_tree_view_get_model (tree_view);
  GtkTreeIter iter;
  OBJECT *obj;

  g_return_if_fail (gtk_tree_model_get_iter (model, &iter, path));

  gtk_tree_model_get (model, &iter,
                      COLUMN_OBJECT, &obj,
                      -1);

  if (obj != NULL)
    select_object (GSCHEM_DOCKABLE (user_data)->w_current, obj);
}


static OBJECT *
get_message_object (GschemMessagesDockable *messages_dockable)
{
  GtkTreeView *tree_view = GTK_TREE_VIEW (messages_dockable->tree_view);
  GtkTreeSelection *selection = gtk_tree_view_get_selection (tree_view);
  GtkTreeModel *model;
  GtkTreeIter iter;
  OBJECT *obj;

  if (!gtk_tree_selection_get_selected (selection, &model, &iter))
    return NULL;

  gtk_tree_model_get (model, &iter,
                      COLUMN_OBJECT, &obj,
                      -1);
  return obj;
}


static void
select_item_activate (GtkMenuItem *menu_item, gpointer user_data)
{
  GschemMessagesDockable *messages_dockable =
    GSCHEM_MESSAGES_DOCKABLE (user_data);

  OBJECT *obj = get_message_object (messages_dockable);

  select_object (GSCHEM_DOCKABLE (user_data)->w_current, obj);
}


static void
confirm_item_activate (GtkMenuItem *menu_item, gpointer user_data)
{
  GschemMessagesDockable *messages_dockable =
    GSCHEM_MESSAGES_DOCKABLE (user_data);

  OBJECT *obj = get_message_object (messages_dockable);

  confirm_object_symversion (GSCHEM_DOCKABLE (user_data)->w_current, obj);
}


static void
create_menu (GschemMessagesDockable *messages_dockable)
{
  GtkWidget *menu, *item;
  g_return_if_fail (messages_dockable->menu == NULL);

  menu = messages_dockable->menu = gtk_menu_new ();

  item = messages_dockable->select_item =
    gtk_menu_item_new_with_mnemonic (_("_Select Object"));
  g_signal_connect (item, "activate",
                    G_CALLBACK (select_item_activate), messages_dockable);
  gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);

  item = messages_dockable->confirm_item =
    gtk_menu_item_new_with_mnemonic (_("_Confirm Symbol Version"));
  g_signal_connect (item, "activate",
                    G_CALLBACK (confirm_item_activate), messages_dockable);
  gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);

  gtk_widget_show_all (menu);

  /* make sure the menu will be destroyed when the widget is destroyed and
     moves between screens correctly if the widgets moves between screens */
  gtk_menu_attach_to_widget (
    GTK_MENU (menu), messages_dockable->tree_view, NULL);
}


static void
menu_position_func (GtkMenu *menu, gint *x, gint *y, gboolean *push_in,
                    gpointer user_data)
{
  gint *pos = user_data;

  *x = pos[0];
  *y = pos[1];
}


static void
popup (GschemMessagesDockable *messages_dockable,
       GtkTreeModel *model, GtkTreeIter *iter,
       GdkEventButton *event)
{
  gint type, severity;
  OBJECT *obj;

  gtk_tree_model_get (model, iter,
                      COLUMN_TYPE, &type,
                      COLUMN_SEVERITY, &severity,
                      COLUMN_OBJECT, &obj,
                      -1);

  if (severity == severity_supplemental_text || obj == NULL)
    /* there's no popup for extra lines and removed items */
    return;

  if (messages_dockable->menu == NULL)
    create_menu (messages_dockable);

  gtk_widget_set_sensitive (messages_dockable->select_item, obj != NULL);
  gtk_widget_set_sensitive (messages_dockable->confirm_item,
                            type == type_symversion);

  if (event != NULL)
    gtk_menu_popup (GTK_MENU (messages_dockable->menu), NULL, NULL,
                    NULL, NULL, event->button, event->time);
  else {
    GtkTreeView *tree_view = GTK_TREE_VIEW (messages_dockable->tree_view);
    GtkTreePath *path = gtk_tree_model_get_path (model, iter);
    GdkRectangle rect;
    gint pos[2];

    gtk_tree_view_get_cell_area (tree_view, path, NULL, &rect);
    gtk_tree_path_free (path);

    (void) gdk_window_get_origin (gtk_tree_view_get_bin_window (tree_view),
                                  &pos[0], &pos[1]);
    pos[0] += rect.x;
    pos[1] += rect.y + rect.height;

    gtk_menu_popup (GTK_MENU (messages_dockable->menu), NULL, NULL,
                    menu_position_func, pos, 0, gtk_get_current_event_time ());
    gtk_menu_shell_select_first (GTK_MENU_SHELL (messages_dockable->menu),
                                 FALSE);
  }
}


static gboolean
tree_view_button_press_event (GtkWidget *widget, GdkEventButton *event,
                              gpointer user_data)
{
  GschemMessagesDockable *messages_dockable =
    GSCHEM_MESSAGES_DOCKABLE (user_data);

  GtkTreeView *tree_view = GTK_TREE_VIEW (messages_dockable->tree_view);
  GtkTreePath *path = NULL;
  GtkTreeModel *model = gtk_tree_view_get_model (tree_view);
  GtkTreeIter iter;
  GtkTreeSelection *selection = gtk_tree_view_get_selection (tree_view);

  /* only show popup if this is a single right click */
  if (event->button != 3 || event->type != GDK_BUTTON_PRESS)
    return FALSE;

  /* coordinates are expected to be relative to bin window */
  if (event->window != gtk_tree_view_get_bin_window (tree_view))
    return FALSE;

  if (!gtk_tree_view_get_path_at_pos (tree_view, event->x, event->y,
                                      &path, NULL, NULL, NULL) ||
      !gtk_tree_model_get_iter (model, &iter, path)) {
    gtk_tree_path_free (path);
    return FALSE;
  }

  gtk_tree_selection_select_path (selection, path);
  gtk_tree_path_free (path);

  popup (messages_dockable, model, &iter, event);
  return TRUE;
}


static gboolean
tree_view_popup_menu (GtkWidget *widget, gpointer user_data)
{
  GschemMessagesDockable *messages_dockable =
    GSCHEM_MESSAGES_DOCKABLE (user_data);

  GtkTreeSelection *selection = gtk_tree_view_get_selection (
    GTK_TREE_VIEW (messages_dockable->tree_view));
  GtkTreeModel *model;
  GtkTreeIter iter;

  if (!gtk_tree_selection_get_selected (selection, &model, &iter))
    return FALSE;

  popup (messages_dockable, model, &iter, NULL);
  return TRUE;
}


static GtkWidget *
create_widget (GschemDockable *dockable)
{
  GschemMessagesDockable *messages_dockable =
    GSCHEM_MESSAGES_DOCKABLE (dockable);

  GtkWidget *tree_view;
  GtkTreeSelection *selection;
  GtkTreeViewColumn *column;
  GtkCellRenderer *renderer;
  GtkWidget *scrolled;

  tree_view = GTK_WIDGET (
    g_object_new (GTK_TYPE_TREE_VIEW,
                  /* GtkTreeView */
                  "headers-visible", FALSE,
                  "model", GTK_TREE_MODEL (messages_dockable->store),
                  "tooltip-column", COLUMN_TOOLTIP,
                  NULL));
  messages_dockable->tree_view = tree_view;

  g_signal_connect (tree_view, "row-activated",
                    G_CALLBACK (tree_view_row_activated), messages_dockable);
  g_signal_connect (tree_view, "button-press-event",
                    G_CALLBACK (tree_view_button_press_event),
                    messages_dockable);
  g_signal_connect (tree_view, "popup-menu",
                    G_CALLBACK (tree_view_popup_menu), messages_dockable);

  selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (tree_view));
  g_signal_connect (selection, "changed",
                    G_CALLBACK (tree_selection_changed), messages_dockable);


  /* filename column */
  column = gtk_tree_view_column_new ();
  gtk_tree_view_column_set_title (column, _("Filename"));
  gtk_tree_view_append_column (GTK_TREE_VIEW (tree_view), column);

  renderer = gtk_cell_renderer_pixbuf_new ();
  gtk_tree_view_column_pack_start (column, renderer, FALSE);
  gtk_tree_view_column_set_cell_data_func (column, renderer,
                                           icon_cell_data_func, NULL, NULL);

  renderer = gtk_cell_renderer_text_new ();
  gtk_tree_view_column_pack_start (column, renderer, TRUE);
  gtk_tree_view_column_add_attribute (column, renderer,
                                      "text", COLUMN_FILENAME);
  g_object_set (renderer, "foreground", "#aaa", NULL);
  gtk_tree_view_column_add_attribute (column, renderer,
                                      "foreground-set", COLUMN_REMOVED);

  /* refdes column */
  column = gtk_tree_view_column_new ();
  gtk_tree_view_column_set_title (column, _("Comp/Pin"));
  gtk_tree_view_append_column (GTK_TREE_VIEW (tree_view), column);

  renderer = gtk_cell_renderer_text_new ();
  gtk_tree_view_column_pack_start (column, renderer, TRUE);
  gtk_tree_view_column_add_attribute (column, renderer,
                                      "text", COLUMN_REFDES);
  g_object_set (renderer, "foreground", "#aaa", NULL);
  gtk_tree_view_column_add_attribute (column, renderer,
                                      "foreground-set", COLUMN_REMOVED);

  /* message column */
  column = gtk_tree_view_column_new ();
  gtk_tree_view_column_set_title (column, _("Message"));
  gtk_tree_view_append_column (GTK_TREE_VIEW (tree_view), column);

  renderer = gtk_cell_renderer_text_new ();
  gtk_tree_view_column_pack_start (column, renderer, TRUE);
  gtk_tree_view_column_add_attribute (column, renderer,
                                      "text", COLUMN_MESSAGE);
  g_object_set (renderer, "foreground", "#aaa", NULL);
  gtk_tree_view_column_add_attribute (column, renderer,
                                      "foreground-set", COLUMN_REMOVED);


  scrolled = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled),
                                  GTK_POLICY_AUTOMATIC,
                                  GTK_POLICY_ALWAYS);
  gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (scrolled),
                                       GTK_SHADOW_NONE);
  gtk_container_add (GTK_CONTAINER (scrolled), tree_view);

  gtk_widget_show_all (scrolled);
  return scrolled;
}


/******************************************************************************/


static void
check_floating_symversion (GschemMessagesDockable *messages_dockable,
                           const gchar *filename)
{
  GschemToplevel *w_current = GSCHEM_DOCKABLE (messages_dockable)->w_current;
  TOPLEVEL *toplevel = gschem_toplevel_get_toplevel (w_current);

  char *symversion = o_attrib_search_floating_attribs_by_name (
    s_page_objects (toplevel->page_current), "symversion", 0);

  if (symversion == NULL)
    return;

  char *endptr = NULL;
  double symversion_value = strtod (symversion, &endptr);

  if (symversion_value == 0 && symversion == endptr)
    add_message (messages_dockable,
                 type_other, severity_warning, filename, "", NULL, _(
"See http://wiki.geda-project.org/geda:master_attributes_list#symversion for "
"the semantics of the symversion= attribute."),
                 _("Invalid symversion= attribute \"%s\""),
                 symversion);

  g_free (symversion);
}


/*! \brief Check symversion of a component.
 *
 * Compares the symversion= attribute attached to a component in the
 * schematic with the symversion= attribute inherited from the symbol.
 */
static void
check_symversion (GschemMessagesDockable *messages_dockable,
                  const gchar *filename, const char *refdes, OBJECT *object)
{
  char *inside = NULL;
  char *outside = NULL;
  double inside_value = -1.0;
  double outside_value = -1.0;
  char *err_check = NULL;
  int inside_present = FALSE;
  int outside_present = FALSE;
  double inside_major, inside_minor;
  double outside_major, outside_minor;

  g_return_if_fail (object != NULL);
  g_return_if_fail ((object->type == OBJ_COMPLEX ||
                     object->type == OBJ_PLACEHOLDER));
  g_return_if_fail (object->complex != NULL);

  /* first look on the inside for the symversion= attribute */
  inside = o_attrib_search_inherited_attribs_by_name (object, "symversion", 0);

  /* now look for the symversion= attached to object */
  outside = o_attrib_search_attached_attribs_by_name (object, "symversion", 0);

  if (inside)
  {
    inside_value = strtod(inside, &err_check);
    if (inside_value == 0 && inside == err_check)
    {
      add_message (messages_dockable,
                   type_symversion_error, severity_warning,
                   filename, refdes, object,
                   _("This is an error in the symbol and should be fixed by "
                     "the library author."),
                   _("Symbol version parse error: could not parse attribute "
                     "\"symversion=%s\" in symbol file \"%s\""),
                   inside, object->complex_basename);
      goto done;
    }
    inside_present = TRUE;
  } else {
    inside_present = FALSE;  /* attribute not inside */
  }

  if (outside)
  {
    outside_value = strtod(outside, &err_check);
    if (outside_value == 0 && outside == err_check)
    {
      add_message (messages_dockable,
                   type_symversion_error, severity_warning,
                   filename, refdes, object, NULL,
                   _("Symbol version parse error: could not parse attribute "
                     "\"symversion=%s\" attached to symbol \"%s\""),
                   outside, object->complex_basename);
      goto done;
    }
    outside_present = TRUE;
  } else {
    outside_present = FALSE;  /* attribute not outside */
  }

#if DEBUG
  printf("%s:\n\tinside: %.1f outside: %.1f\n\n", object->name,
         inside_value, outside_value);
#endif

  /* symversion= is not present anywhere */
  if (!inside_present && !outside_present)
  {
    /* symbol is legacy and versioned okay */
    goto done;
  }

  /* No symversion inside, but a version is outside, this is a weird case */
  if (!inside_present && outside_present)
  {
    add_message (messages_dockable,
                 type_symversion_error, severity_warning,
                 filename, refdes, object, NULL,
                 _("Symbol version oddity: "
                   "symversion=%s attached to instantiated symbol, "
                   "but no symversion= attribute inside symbol file \"%s\""),
                 outside, object->complex_basename);
    goto done;
  }

  /* inside & not outside is a valid case, means symbol in library is newer */
  /* also if inside_value is greater than outside_value, then symbol in */
  /* library is newer */
  if ((inside_present && !outside_present) ||
      ((inside_present && outside_present) && (inside_value > outside_value)))
  {
    /* break up the version values into major.minor numbers */
    inside_major = floor(inside_value);
    inside_minor = inside_value - inside_major;

    if (outside_present)
    {
      outside_major = floor(outside_value);
      outside_minor = outside_value - outside_major;
    } else {
      /* symversion was not attached to the symbol, set all to zero */
      outside_major = 0.0;
      outside_minor = 0.0;
      outside_value = 0.0;
    }

#if DEBUG
    printf("i: %f %f %f\n", inside_value, inside_major, inside_minor);
    printf("o: %f %f %f\n", outside_value, outside_major, outside_minor);
#endif

    const char *tooltip = _(
"The symbol has changed in the library, and its author has updated the "
"symversion= attribute in order to make you aware of this fact.\n\n"
"<i>Major</i> version changes usually indicate that things like pin "
"endpoints have changed which need some manual fixing of the schematic.\n"
"<i>Minor</i> version changes should be reviewed but don't usually "
"necessitate fixing.\n\n"
"Once you have verified that your schematic works correctly with the new "
"version of the symbol, you can update the component's symversion= attribute "
"to match the library version (or right-click and select \"Confirm Symbol "
"Version\") in order to hide this message.");

    if (inside_major > outside_major)
    {
      add_message (messages_dockable,
                   type_symversion, severity_warning,
                   filename, refdes, object, tooltip,
                   _("Major version change: symbol \"%s\" in library (%s) "
                     "is newer than instantiated symbol (%s)"),
                   object->complex_basename,
                   inside != NULL ? inside : _("no version"),
                   outside != NULL ? outside : _("no version"));

      /* don't bother checking minor changes if there are major ones */
      goto done;
    }

    if (inside_minor > outside_minor)
    {
      add_message (messages_dockable,
                   type_symversion, severity_message,
                   filename, refdes, object, tooltip,
                   _("Minor version change: symbol \"%s\" in library (%s) "
                     "is newer than instantiated symbol (%s)"),
                   object->complex_basename,
                   inside != NULL ? inside : _("no version"),
                   outside != NULL ? outside : _("no version"));
    }
    else
    add_message (messages_dockable,
                 type_symversion, severity_warning,
                 filename, refdes, object, tooltip,
                 _("Symbol version mismatch: symbol \"%s\" in library (%s) "
                   "is newer than instantiated symbol (%s)"),
                 object->complex_basename,
                 inside != NULL ? inside : _("no version"),
                 outside != NULL ? outside : _("no version"));


    goto done;
  }

  /* outside value is greater than inside value, this is weird case */
  if ((inside_present && outside_present) && (outside_value > inside_value))
  {
    add_message (messages_dockable,
                 type_symversion, severity_warning, filename, refdes, object,
                 _("This probably means you are trying to load a schematic "
                   "with an older (and probably incompatible) version of the "
                   "library."),
                 _("Symbol version oddity: instantiated symbol (%s) "
                   "is newer than symbol \"%s\" in library (%s)"),
                 object->complex_basename,
                 inside != NULL ? inside : _("no version"),
                 outside != NULL ? outside : _("no version"));
    goto done;
  }

  /* if inside_value and outside_value match, then symbol versions are okay */

done:
  g_free(inside);
  g_free(outside);
}


static void
check_symbol (GschemMessagesDockable *messages_dockable,
              const gchar *filename, OBJECT *obj)
{
  GList *symlist;
  unsigned int len;
  char *refdes;

  g_return_if_fail (obj != NULL);
  g_return_if_fail (obj->type == OBJ_COMPLEX || obj->type == OBJ_PLACEHOLDER);

  if (obj->complex_embedded)
    return;

  symlist = s_clib_search (obj->complex_basename, CLIB_EXACT);
  len = g_list_length (symlist);
  g_list_free (symlist);

  refdes = o_attrib_search_object_attribs_by_name (obj, "refdes", 0);
  if (refdes == NULL)
    refdes = g_strdup (_("(unknown)"));

  if (len == 0)
    add_message (messages_dockable,
                 type_other, severity_error, filename, refdes, obj, NULL,
                 _("symbol \"%s\" not found"),
                 obj->complex_basename);
  else if (len > 1) {
    add_message (messages_dockable,
                 type_other, severity_warning, filename, refdes, obj, NULL,
                 _("There are %d symbols with name \"%s\" in the library."),
                 len, obj->complex_basename);
    add_message (messages_dockable,
                 type_other, severity_supplemental_text, "", "", obj, NULL,
                 _("Picking one at random--this may not be what you want..."));
  }

  if (len >= 1)
    check_symversion (messages_dockable, filename, refdes, obj);

  g_free (refdes);
}


static gboolean
has_severity (GschemMessagesDockable *messages_dockable,
              GschemMessageSeverity severity)
{
  GtkTreeModel *tree_model = GTK_TREE_MODEL (messages_dockable->store);
  GtkTreeIter iter;

  if (gtk_tree_model_get_iter_first (tree_model, &iter))
    do {
      gint s;
      gtk_tree_model_get (tree_model, &iter,
                          COLUMN_SEVERITY, &s,
                          -1);
      if (severity >= s)
        return TRUE;
    } while (gtk_tree_model_iter_next (tree_model, &iter));

  return FALSE;
}


static void
update_messages (GschemMessagesDockable *messages_dockable)
{
  GschemToplevel *w_current = GSCHEM_DOCKABLE (messages_dockable)->w_current;
  TOPLEVEL *toplevel = gschem_toplevel_get_toplevel (w_current);
  PAGE *page = toplevel->page_current;
  g_return_if_fail (page != NULL);

  gchar *basename = g_path_get_basename (page->page_filename);

  clear_weak_refs (messages_dockable);
  gtk_list_store_clear (messages_dockable->store);

  check_floating_symversion (messages_dockable, basename);

  for (const GList *l = s_page_objects (page); l != NULL; l = l->next) {
    OBJECT *obj = (OBJECT *) l->data;
    if (obj->type == OBJ_COMPLEX || obj->type == OBJ_PLACEHOLDER)
      check_symbol (messages_dockable, basename, obj);
  }

  g_free (basename);

  /* If there were warnings, make sure the messages are visible.
     Try not to focus the dockable, though, as this might confuse users. */
  if (has_severity (messages_dockable, severity_warning)) {
    GtkWidget *widget, *notebook;
    switch (gschem_dockable_get_state (w_current->messages_dockable)) {
      case GSCHEM_DOCKABLE_STATE_DOCKED_LEFT:
      case GSCHEM_DOCKABLE_STATE_DOCKED_BOTTOM:
      case GSCHEM_DOCKABLE_STATE_DOCKED_RIGHT:
        widget = w_current->messages_dockable->widget;
        notebook = gtk_widget_get_parent (widget);
        if (GTK_IS_NOTEBOOK (notebook)) {
          gint i = gtk_notebook_page_num (GTK_NOTEBOOK (notebook), widget);
          if (i != -1)
            gtk_notebook_set_current_page (GTK_NOTEBOOK (notebook), i);
        }
        break;
      case GSCHEM_DOCKABLE_STATE_WINDOW:
        /* nothing to do */
        break;
      default:
        gschem_dockable_present (w_current->messages_dockable);
    }
  }
}


void
x_messages_page_changed (GschemToplevel *w_current)
{
  GschemMessagesDockable *messages_dockable =
    GSCHEM_MESSAGES_DOCKABLE (w_current->messages_dockable);

  update_messages (messages_dockable);
}


void
x_messages_update (GschemToplevel *w_current)
{
  GschemMessagesDockable *messages_dockable =
    GSCHEM_MESSAGES_DOCKABLE (w_current->messages_dockable);

  update_messages (messages_dockable);
}
