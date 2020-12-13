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

/*! \file gschem_patch_dockable.c
 * \brief List diffs resulting from a patch.
 */

#include <config.h>

#include <stdio.h>
#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif
#ifdef HAVE_STRING_H
#include <string.h>
#endif

#include <sys/stat.h>
#include <libgen.h>  /* for dirname(3) */

#include "gschem.h"
#include "../include/gschem_patch_dockable.h"

enum {
  COLUMN_FILENAME,
  COLUMN_LOCATION,
  COLUMN_ACTION,
  COLUMN_HAS_ERROR,
  COLUMN_OBJECT,
  COLUMN_COUNT
};

typedef void (*NotifyFunc) (void *, void *);


static gpointer parent_class = NULL;

static void class_init (GschemPatchDockableClass *class);
static void instance_init (GschemPatchDockable *patch_dockable);
static void dispose (GObject *object);
static GtkWidget *create_widget (GschemDockable *dockable);

static void add_hit_to_store (GschemPatchDockable *patch_dockable,
                              gschem_patch_hit_t *hit);
static void clear_store (GschemPatchDockable *patch_dockable);
static GSList *get_pages (GList *pages, gboolean descend);
static GList *get_subpages (PAGE *page);
static void object_weakref_cb (OBJECT *object,
                               GschemPatchDockable *patch_dockable);
static void remove_object (GschemPatchDockable *patch_dockable,
                           OBJECT *object);
static void tree_selection_changed (GtkTreeSelection *selection,
                                    gpointer user_data);


GType
gschem_patch_dockable_get_type ()
{
  static GType type = 0;

  if (type == 0) {
    static const GTypeInfo info = {
      sizeof (GschemPatchDockableClass),
      NULL,                                     /* base_init */
      NULL,                                     /* base_finalize */
      (GClassInitFunc) class_init,
      NULL,                                     /* class_finalize */
      NULL,                                     /* class_data */
      sizeof (GschemPatchDockable),
      0,                                        /* n_preallocs */
      (GInstanceInitFunc) instance_init,
      NULL                                      /* value_table */
    };

    type = g_type_register_static (GSCHEM_TYPE_DOCKABLE,
                                   "GschemPatchDockable",
                                   &info, 0);
  }

  return type;
}


static void
class_init (GschemPatchDockableClass *class)
{
  parent_class = g_type_class_peek_parent (class);

  GSCHEM_DOCKABLE_CLASS (class)->create_widget = create_widget;

  G_OBJECT_CLASS (class)->dispose = dispose;
}


static void
instance_init (GschemPatchDockable *patch_dockable)
{
  patch_dockable->store = gtk_list_store_new (COLUMN_COUNT,
                                              G_TYPE_STRING,
                                              G_TYPE_STRING,
                                              G_TYPE_STRING,
                                              G_TYPE_BOOLEAN,
                                              G_TYPE_POINTER);
}


static void
dispose (GObject *object)
{
  GschemPatchDockable *patch_dockable = GSCHEM_PATCH_DOCKABLE (object);

  if (patch_dockable->store) {
    clear_store (patch_dockable);
    g_clear_object (&patch_dockable->store);
  }

  G_OBJECT_CLASS (parent_class)->dispose (object);
}


static GtkWidget *
create_widget (GschemDockable *dockable)
{
  GschemPatchDockable *patch_dockable = GSCHEM_PATCH_DOCKABLE (dockable);

  GtkTreeViewColumn *column;
  GtkCellRenderer *renderer;
  GtkWidget *scrolled;
  GtkTreeSelection *selection;
  GtkWidget *tree_widget;

  scrolled = gtk_scrolled_window_new (NULL, NULL);

  tree_widget = gtk_tree_view_new_with_model (
    GTK_TREE_MODEL (patch_dockable->store));
  gtk_tree_view_set_search_column (GTK_TREE_VIEW (tree_widget),
                                   COLUMN_LOCATION);
  gtk_container_add (GTK_CONTAINER (scrolled), tree_widget);

  /* filename column */

  column = gtk_tree_view_column_new ();
  gtk_tree_view_column_set_resizable (column, TRUE);
  gtk_tree_view_column_set_title (column, _("Filename"));

  gtk_tree_view_append_column (GTK_TREE_VIEW (tree_widget), column);

  renderer = gtk_cell_renderer_text_new ();
  gtk_tree_view_column_pack_start (column, renderer, TRUE);
  gtk_tree_view_column_add_attribute (
    column, renderer, "text", COLUMN_FILENAME);
  gtk_tree_view_column_add_attribute (
    column, renderer, "foreground-set", COLUMN_HAS_ERROR);
  g_object_set (renderer, "foreground", "red", NULL);

  /* location column */

  column = gtk_tree_view_column_new ();
  gtk_tree_view_column_set_resizable (column, TRUE);
  gtk_tree_view_column_set_title (column, _("Comp/Pin"));

  gtk_tree_view_append_column (GTK_TREE_VIEW (tree_widget), column);

  renderer = gtk_cell_renderer_text_new ();
  gtk_tree_view_column_pack_start (column, renderer, TRUE);
  gtk_tree_view_column_add_attribute (
    column, renderer, "text", COLUMN_LOCATION);
  gtk_tree_view_column_add_attribute (
    column, renderer, "foreground-set", COLUMN_HAS_ERROR);
  g_object_set (renderer, "foreground", "red", NULL);

  /* action column */

  column = gtk_tree_view_column_new ();
  gtk_tree_view_column_set_resizable (column, TRUE);
  gtk_tree_view_column_set_title (column, _("Required action"));

  gtk_tree_view_append_column (GTK_TREE_VIEW (tree_widget), column);

  renderer = gtk_cell_renderer_text_new ();
  gtk_tree_view_column_pack_start (column, renderer, TRUE);
  gtk_tree_view_column_add_attribute (
    column, renderer, "text", COLUMN_ACTION);
  gtk_tree_view_column_add_attribute (
    column, renderer, "foreground-set", COLUMN_HAS_ERROR);
  g_object_set (renderer, "foreground", "red", NULL);

  /* attach signal to detect user selection */

  selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (tree_widget));
  g_signal_connect (selection, "changed",
                    G_CALLBACK (tree_selection_changed), patch_dockable);

  gtk_widget_show_all (scrolled);
  return scrolled;
}


/******************************************************************************/


gchar *
x_patch_guess_filename (PAGE *page)
{
  if (page->patch_filename != NULL)
    return g_strdup (page->patch_filename);

  if (page->is_untitled)
    return NULL;

  size_t len = strlen (page->page_filename);
  if (len < 4 ||
      (g_ascii_strcasecmp (page->page_filename + len - 4, ".sch") != 0 &&
       g_ascii_strcasecmp (page->page_filename + len - 4, ".sym") != 0))
    return g_strdup_printf ("%s.bap", page->page_filename);

  gchar *fn = g_strdup (page->page_filename);
  strcpy (fn + len - 4, ".bap");
  return fn;
}


/*! \brief Let the user select a patch file, and import that patch file.
 */
void
x_patch_import (GschemToplevel *w_current)
{
  PAGE *page = gschem_toplevel_get_toplevel (w_current)->page_current;
  GtkWidget *dialog;
  gchar *patch_filename;
  GtkWidget *extra_widget;
  GtkFileFilter *filter;

  dialog = gtk_file_chooser_dialog_new (_("Import Patch..."),
                                        GTK_WINDOW (w_current->main_window),
                                        GTK_FILE_CHOOSER_ACTION_OPEN,
                                        GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                                        GTK_STOCK_OPEN,   GTK_RESPONSE_ACCEPT,
                                        NULL);
  gtk_dialog_set_alternative_button_order (GTK_DIALOG (dialog),
                                           GTK_RESPONSE_ACCEPT,
                                           GTK_RESPONSE_CANCEL, -1);

  patch_filename = x_patch_guess_filename (page);

  if (patch_filename != NULL &&
      g_file_test (patch_filename, G_FILE_TEST_EXISTS |
                                   G_FILE_TEST_IS_REGULAR))
    gtk_file_chooser_set_filename (GTK_FILE_CHOOSER (dialog), patch_filename);
  else
    /* dirname(3) modifies its argument, but that's fine here */
    gtk_file_chooser_set_current_folder (GTK_FILE_CHOOSER (dialog),
                                         dirname (patch_filename));

  g_free (patch_filename);

  extra_widget = gtk_check_button_new_with_label (_("Descend into hierarchy"));
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (extra_widget),
                                page->patch_descend);
  gtk_file_chooser_set_extra_widget (GTK_FILE_CHOOSER (dialog), extra_widget);

  filter = gtk_file_filter_new ();
  gtk_file_filter_set_name (filter, _("Back-annotation patches"));
  gtk_file_filter_add_pattern (filter, "*.bap");
  gtk_file_chooser_add_filter (GTK_FILE_CHOOSER (dialog), filter);

  filter = gtk_file_filter_new ();
  gtk_file_filter_set_name (filter, _("All files"));
  gtk_file_filter_add_pattern (filter, "*");
  gtk_file_chooser_add_filter (GTK_FILE_CHOOSER (dialog), filter);

  if (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_ACCEPT) {
    g_free (page->patch_filename);
    page->patch_filename =
      gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (dialog));
    page->patch_descend =
      gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (extra_widget));

    x_patch_do_import (w_current, page);
  }

  gtk_widget_destroy (dialog);
}


/*! \brief Find all objects that have an outstanding patch mismatch.
 *
 * Uses the page's current patch filename and descent flag.
 *
 * The results are placed in the dockable's GtkListStore.
 */
void
x_patch_do_import (GschemToplevel *w_current, PAGE *page)
{
  GschemPatchDockable *patch_dockable =
    GSCHEM_PATCH_DOCKABLE (w_current->patch_dockable);

  GList *pages = geda_list_get_glist (w_current->toplevel->pages);
  gschem_patch_state_t st;
  struct stat buf;
  GSList *all_pages, *objects;

  g_return_if_fail (patch_dockable != NULL);
  g_return_if_fail (patch_dockable->store != NULL);

  g_return_if_fail (page->patch_filename != NULL);

  if (gschem_patch_state_init (&st, page->patch_filename) != 0) {
    g_warning (_("Unable to open patch file %s\n"), page->patch_filename);

    GtkWidget *dialog = gtk_message_dialog_new (
      GTK_WINDOW (w_current->main_window),
      GTK_DIALOG_DESTROY_WITH_PARENT,
      GTK_MESSAGE_ERROR,
      GTK_BUTTONS_CLOSE,
      _("Failed to load patch file \"%s\".\n"
        "See the standard error output for more information."),
      page->patch_filename);
    gtk_window_set_title (GTK_WINDOW (dialog), _("Failed to import patch"));
    gtk_dialog_run (GTK_DIALOG (dialog));
    gtk_widget_destroy (dialog);
    return;
  }

  if (stat (page->patch_filename, &buf) != -1) {
    page->patch_seen_on_disk = TRUE;
    page->patch_mtime = buf.st_mtim;
    x_window_update_patch_change_notification (w_current, page);
  }

  all_pages = get_pages (pages, page->patch_descend);

  for (GSList *page_iter = all_pages;
       page_iter != NULL; page_iter = page_iter->next) {
    PAGE *page = (PAGE *) page_iter->data;

    if (page == NULL) {
      g_warning ("NULL page encountered");
      continue;
    }

    for (const GList *object_iter = s_page_objects (page);
         object_iter != NULL; object_iter = object_iter->next) {
      OBJECT *object = (OBJECT *) object_iter->data;

      if (object == NULL) {
        g_warning ("NULL object encountered");
        continue;
      }

      gschem_patch_state_build (&st, object);
    }
  }

  g_slist_free (all_pages);

  objects = gschem_patch_state_execute (&st);
  gschem_patch_state_destroy (&st);

  clear_store (patch_dockable);

  for (GSList *object_iter = objects;
       object_iter != NULL; object_iter = object_iter->next) {
    gschem_patch_hit_t *hit = (gschem_patch_hit_t *) object_iter->data;
    add_hit_to_store (patch_dockable, hit);
  }

  gschem_patch_free_hit_list (objects);

  if (objects != NULL)
    gschem_dockable_present (GSCHEM_DOCKABLE (patch_dockable));
}


/*! \brief Place a result in the store so the user can see and select it.
 */
static void
add_hit_to_store (GschemPatchDockable *patch_dockable, gschem_patch_hit_t *hit)
{
  char *basename;
  OBJECT *final_object = NULL;
  GtkTreeIter tree_iter;

  /* TODO: this is an ugly workaround: can't put pins or objects
     directly on the list because they have no object page; use
     their complex object's first visible text instead
     Fix: be able to jump to any OBJECT
   */
  {
    OBJECT *page_obj;
    GList *l;
    int found_pin;

    if (hit->object != NULL)
      l = o_attrib_return_attribs (hit->object);
    else
      l = NULL;

    if (l == NULL) {
      gtk_list_store_append (patch_dockable->store, &tree_iter);
      gtk_list_store_set (patch_dockable->store,
                          &tree_iter,
                          COLUMN_FILENAME, _("N/A"),
                          COLUMN_LOCATION, hit->loc_name,
                          COLUMN_ACTION, hit->action,
                          COLUMN_HAS_ERROR, hit->object == NULL,
                          COLUMN_OBJECT, final_object,
                          -1);
      return;
    }

    found_pin = 0;
    for (GList *i = l; i != NULL; i = i->next) {
      final_object = i->data;
      if (final_object->type == OBJ_TEXT) {
        page_obj = gschem_page_get_page_object (final_object);
        if (o_is_visible (page_obj)) {
          found_pin = 1;
          break;
        }
      }
    }
    g_list_free (l);
    if (!found_pin) {
      g_warning ("no pin text to zoom to");
      page_obj = final_object = NULL;
    }

    if (final_object == NULL) {
      g_warning ("no text attrib?");
      page_obj = final_object = NULL;
    }
    if (page_obj != NULL && !page_obj->page->is_untitled)
      basename = g_path_get_basename (page_obj->page->page_filename);
    else
      basename = NULL;
  }
  s_object_weak_ref (final_object, (NotifyFunc) object_weakref_cb,
                     patch_dockable);

  gtk_list_store_append (patch_dockable->store, &tree_iter);

  if (basename != NULL) {
    gtk_list_store_set (patch_dockable->store,
                        &tree_iter,
                        COLUMN_FILENAME, basename,
                        COLUMN_LOCATION, hit->loc_name,
                        COLUMN_ACTION, hit->action,
                        COLUMN_HAS_ERROR, hit->object == NULL,
                        COLUMN_OBJECT, final_object,
                        -1);
    g_free (basename);
  } else {
    gtk_list_store_set (patch_dockable->store,
                        &tree_iter,
                        COLUMN_FILENAME, _("N/A"),
                        COLUMN_LOCATION, hit->loc_name,
                        COLUMN_ACTION, hit->action,
                        COLUMN_HAS_ERROR, hit->object == NULL,
                        COLUMN_OBJECT, final_object,
                        -1);
  }
}


/*! \brief delete all items from the list store
 *
 *  This function deletes all items in the list store and removes all the weak
 *  references to the objects.
 *
 *  \param [in] patch_dockable
 */
static void
clear_store (GschemPatchDockable *patch_dockable)
{
  GtkTreeIter iter;
  gboolean valid;

  g_return_if_fail (patch_dockable != NULL);
  g_return_if_fail (patch_dockable->store != NULL);

  valid = gtk_tree_model_get_iter_first (
    GTK_TREE_MODEL (patch_dockable->store), &iter);

  while (valid) {
    GValue value = G_VALUE_INIT;

    gtk_tree_model_get_value (GTK_TREE_MODEL (patch_dockable->store),
                              &iter,
                              COLUMN_OBJECT,
                              &value);

    if (G_VALUE_HOLDS_POINTER (&value)) {
      OBJECT *object = g_value_get_pointer (&value);

      if (object != NULL)
        s_object_weak_unref (object, (NotifyFunc) object_weakref_cb,
                             patch_dockable);
    }

    g_value_unset (&value);

    valid = gtk_tree_model_iter_next (
      GTK_TREE_MODEL (patch_dockable->store), &iter);
  }

  gtk_list_store_clear (patch_dockable->store);
}


/*! \brief obtain a list of pages for an operation
 *
 *  Descends the heirarchy of pages, if selected, and removes duplicate pages.
 *
 *  \param [in] pages the list of pages to begin search
 *  \param [in] descend alose locates subpages
 *  \return a list of all the pages
 */
static GSList*
get_pages (GList *pages, gboolean descend)
{
  GList *input_list = g_list_copy (pages);
  GSList *output_list = NULL;
  GHashTable *visit_list = g_hash_table_new (NULL, NULL);

  while (input_list != NULL) {
    PAGE *page = (PAGE*) input_list->data;

    input_list = g_list_delete_link (input_list, input_list);

    if (page == NULL) {
      g_warning ("NULL page encountered");
      continue;
    }

    /** \todo the following function becomes available in glib 2.32 */
    /* if (g_hash_table_contains (visit_list, page)) { */

    if (g_hash_table_lookup_extended (visit_list, page, NULL, NULL)) {
      continue;
    }

    output_list = g_slist_prepend (output_list, page);
    g_hash_table_insert (visit_list, page, NULL);

    if (descend) {
      input_list = g_list_concat (input_list, get_subpages (page));
    }
  }

  g_hash_table_destroy (visit_list);

  return g_slist_reverse (output_list);
}


/*! \brief get the subpages of a schematic page
 *
 *  if any subpages are not loaded, this function will load them.
 *
 *  \param [in] page the parent page
 *  \return a list of all the subpages
 */
static GList*
get_subpages (PAGE *page)
{
  GList *page_list = NULL;

  g_return_val_if_fail (page != NULL, NULL);

  for (const GList *object_iter = s_page_objects (page);
       object_iter != NULL; object_iter = object_iter->next) {
    OBJECT *object = (OBJECT *) object_iter->data;
    char *attrib;
    char **filenames;

    if (object == NULL) {
      g_warning ("NULL object encountered");
      continue;
    }

    if (object->type != OBJ_COMPLEX) {
      continue;
    }

    attrib = o_attrib_search_attached_attribs_by_name (object,
                                                       "source",
                                                       0);

    if (attrib == NULL) {
      attrib = o_attrib_search_inherited_attribs_by_name (object,
                                                          "source",
                                                          0);
    }

    if (attrib == NULL) {
      continue;
    }

    filenames = g_strsplit (attrib, ",", 0);

    if (filenames == NULL) {
      continue;
    }

    for (char **iter = filenames; *iter != NULL; iter++) {
      PAGE *subpage = s_hierarchy_load_subpage (page, *iter, NULL);

      if (subpage != NULL) {
        page_list = g_list_prepend (page_list, subpage);
      }
    }

    g_strfreev (filenames);
  }

  return g_list_reverse (page_list);
}


/*! \brief callback for an object that has been destroyed
 *
 *  \param [in] object the object that has been destroyed
 *  \param [in] patch_dockable
 */
static void
object_weakref_cb (OBJECT *object, GschemPatchDockable *patch_dockable)
{
  g_return_if_fail (patch_dockable != NULL);

  remove_object (patch_dockable, object);
}


/*! \brief remove an object from the store
 *
 *  This function gets called in response to the object deletion. And, doesn't
 *  dereference the object.
 *
 *  This function doesn't remove the weak reference, under the assumption that
 *  the object is being destroyed.
 *
 *  \param [in] patch_dockable
 *  \param [in] object the object to remove from the store
 */
static void
remove_object (GschemPatchDockable *patch_dockable, OBJECT *object)
{
  GtkTreeIter iter;
  gboolean valid;

  g_return_if_fail (object != NULL);
  g_return_if_fail (patch_dockable != NULL);
  g_return_if_fail (patch_dockable->store != NULL);

  valid = gtk_tree_model_get_iter_first (
    GTK_TREE_MODEL (patch_dockable->store), &iter);

  while (valid) {
    GValue value = G_VALUE_INIT;

    gtk_tree_model_get_value (GTK_TREE_MODEL (patch_dockable->store),
                              &iter,
                              COLUMN_OBJECT,
                              &value);

    if (G_VALUE_HOLDS_POINTER (&value)) {
      OBJECT *other = g_value_get_pointer (&value);

      if (object == other) {
        g_value_unset (&value);
        valid = gtk_list_store_remove (patch_dockable->store, &iter);
        continue;
      }
    }

    g_value_unset (&value);
    valid = gtk_tree_model_iter_next (
      GTK_TREE_MODEL (patch_dockable->store), &iter);
  }
}


/*! \brief Callback for tree selection "changed" signal.
 *
 * Switches to the page (if applicable) and zooms to the location of
 * the mismatch.
 */
static void
tree_selection_changed (GtkTreeSelection *selection, gpointer user_data)
{
  GschemPatchDockable *patch_dockable = GSCHEM_PATCH_DOCKABLE (user_data);
  g_return_if_fail (selection != NULL);
  g_return_if_fail (patch_dockable != NULL);

  GtkTreeIter iter;
  if (!gtk_tree_selection_get_selected (selection, NULL, &iter))
    return;

  GValue value = G_VALUE_INIT;
  gtk_tree_model_get_value (GTK_TREE_MODEL (patch_dockable->store), &iter,
                            COLUMN_OBJECT, &value);
  if (!G_VALUE_HOLDS_POINTER (&value)) {
    g_value_unset (&value);
    return;
  }

  OBJECT *object = g_value_get_pointer (&value);
  g_value_unset (&value);
  if (object == NULL)
    return;

  OBJECT *page_obj = gschem_page_get_page_object (object);
  g_return_if_fail (page_obj != NULL);


  GschemPageView *view = gschem_toplevel_get_current_page_view (
    GSCHEM_DOCKABLE (patch_dockable)->w_current);
  g_return_if_fail (view != NULL);

  x_window_set_current_page (GSCHEM_DOCKABLE (patch_dockable)->w_current,
                             page_obj->page);

  gschem_page_view_zoom_text (view, object, TRUE);
}
