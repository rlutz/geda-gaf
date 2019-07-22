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

#include "gschem.h"
#include "../include/gschem_patch_dockable.h"

enum {
  COLUMN_FILENAME,
  COLUMN_STRING,
  COLUMN_OBJECT,
  COLUMN_COUNT
};

typedef void (*NotifyFunc) (void *, void *);


static gpointer parent_class = NULL;

static void class_init (GschemPatchDockableClass *class);
static void instance_init (GschemPatchDockable *patch_dockable);
static void dispose (GObject *object);
static GtkWidget *create_widget (GschemDockable *dockable);

static void assign_store_patch (GschemPatchDockable *patch_dockable,
                                GSList *objects);
static void clear_store (GschemPatchDockable *patch_dockable);
static GSList *find_objects_using_patch (GSList *pages, const char *text);
static GSList *get_pages (GList *pages, gboolean descend);
static GList *get_subpages (PAGE *page);
static void object_weakref_cb (OBJECT *object,
                               GschemPatchDockable *patch_dockable);
static void remove_object (GschemPatchDockable *patch_dockable,
                           OBJECT *object);
static void select_cb (GtkTreeSelection *selection,
                       GschemPatchDockable *patch_dockable);


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
  gtk_container_add (GTK_CONTAINER (scrolled), tree_widget);

  /* filename column */

  column = gtk_tree_view_column_new();
  gtk_tree_view_column_set_resizable (column, TRUE);
  gtk_tree_view_column_set_title (column, _("Filename"));

  gtk_tree_view_append_column (GTK_TREE_VIEW (tree_widget), column);

  renderer = gtk_cell_renderer_text_new();
  gtk_tree_view_column_pack_start(column, renderer, TRUE);
  gtk_tree_view_column_add_attribute(column, renderer, "text", 0);

  /* text column */

  column = gtk_tree_view_column_new();
  gtk_tree_view_column_set_resizable (column, TRUE);
  gtk_tree_view_column_set_title (column, _("Text"));

  gtk_tree_view_append_column (GTK_TREE_VIEW (tree_widget), column);

  renderer = gtk_cell_renderer_text_new();
  gtk_tree_view_column_pack_start(column, renderer, TRUE);
  gtk_tree_view_column_add_attribute(column, renderer, "text", 1);

  /* attach signal to detect user selection */

  selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (tree_widget));
  g_signal_connect (selection, "changed",
                    G_CALLBACK (select_cb), patch_dockable);

  gtk_widget_show_all (scrolled);
  return scrolled;
}


/******************************************************************************/


/*! \brief find instances of a given string
 *
 *  Finds instances of a given string and displays the result inside this
 *  widget.
 *
 *  \param [in] patch_dockable
 *  \param [in] pages a list of pages to search
 *  \param [in] text the text to find
 *  \param [in] descend decend the page heirarchy
 *  \return the number of objects found
 */
gboolean
gschem_patch_dockable_find (GschemPatchDockable *patch_dockable,
                            GList *pages, const char *text, gboolean descend)
{
  int count;
  GSList *objects = NULL;
  GSList *all_pages;

  all_pages = get_pages (pages, descend);

  objects = find_objects_using_patch (all_pages, text);

  g_slist_free (all_pages);

  assign_store_patch (patch_dockable, objects);

  count = g_slist_length (objects);
  g_slist_free (objects);

  return count != 0;
}


/*! \brief places object in the store so the user can see them
 *
 *  \param [in] patch_dockable
 *  \param [in] objects the list of objects to put in the store
 */
static void
assign_store_patch (GschemPatchDockable *patch_dockable, GSList *objects)
{
	static const char *UNKNOWN_FILE_NAME = "N/A";

  g_return_if_fail (patch_dockable != NULL);
  g_return_if_fail (patch_dockable->store != NULL);

  clear_store (patch_dockable);

  for (GSList *object_iter = objects;
       object_iter != NULL; object_iter = object_iter->next) {
    gschem_patch_hit_t *hit = (gschem_patch_hit_t *) object_iter->data;
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
                            COLUMN_FILENAME, UNKNOWN_FILE_NAME,
                            COLUMN_STRING, hit->text,
                            COLUMN_OBJECT, final_object,
                            -1);
        object_iter->data = NULL;
        continue;
      }


      found_pin = 0;
      for (GList *i = l; i != NULL; i = i->next) {
        final_object = i->data;
        if (final_object->type == OBJ_TEXT) {
          page_obj = gschem_page_get_page_object(final_object);
          if (o_is_visible (page_obj)) {
            found_pin = 1;
            break;
          }
        }
      }
      g_list_free(l);
      if (!found_pin) {
        g_warning ("no pin text to zoom to");
        page_obj = final_object = NULL;
      }

      if (final_object == NULL) {
        g_warning ("no text attrib?");
        page_obj = final_object = NULL;
      }
      if (page_obj != NULL)
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
                          COLUMN_STRING, hit->text,
                          COLUMN_OBJECT, final_object,
                          -1);
      g_free (basename);
    }
    else {
      gtk_list_store_set (patch_dockable->store,
                          &tree_iter,
                          COLUMN_FILENAME, UNKNOWN_FILE_NAME,
                          COLUMN_STRING, hit->text,
                          COLUMN_OBJECT, final_object,
                          -1);
    }
    free(hit);

    object_iter->data = NULL;
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


/*! \brief Find all objects that have outstanding patch mismatch
 *
 *  \param pages the list of pages to search
 *  \param text ???
 *  \return a list of objects that have mismatch
 */
static GSList*
find_objects_using_patch (GSList *pages, const char *text)
{
  GSList *object_list = NULL;
  gschem_patch_state_t st;

  if (gschem_patch_state_init(&st, text) != 0) {
    g_warning("Unable to open patch file %s\n", text);
    return NULL;
  }

  for (GSList *page_iter = pages;
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

      gschem_patch_state_build(&st, object);
    }
  }

  object_list = gschem_patch_state_execute(&st, object_list);
  gschem_patch_state_destroy(&st);

  return g_slist_reverse (object_list);
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


/*! \brief callback for user selecting an item
 *
 *  \param [in] selection
 *  \param [in] patch_dockable
 */
static void
select_cb (GtkTreeSelection *selection, GschemPatchDockable *patch_dockable)
{
  GtkTreeIter iter;
  gboolean success;

  g_return_if_fail (selection != NULL);
  g_return_if_fail (patch_dockable != NULL);

  success = gtk_tree_selection_get_selected (selection, NULL, &iter);

  if (success) {
    GValue value = G_VALUE_INIT;

    gtk_tree_model_get_value (GTK_TREE_MODEL (patch_dockable->store),
                              &iter,
                              COLUMN_OBJECT,
                              &value);

    if (G_VALUE_HOLDS_POINTER (&value)) {
      OBJECT *object = g_value_get_pointer (&value);

      if (object != NULL) {
        GschemPageView *view = gschem_toplevel_get_current_page_view (
          GSCHEM_DOCKABLE (patch_dockable)->w_current);
        g_return_if_fail (view != NULL);

        PAGE *page = gschem_page_view_get_page (view);
        g_return_if_fail (page != NULL);
        OBJECT *page_obj;

        g_return_if_fail (object != NULL);
        page_obj = gschem_page_get_page_object (object);
        g_return_if_fail (page_obj != NULL);

        if (page != page_obj->page)
          gschem_page_view_set_page (view, page_obj->page);

        gschem_page_view_zoom_text (view, object, TRUE);
      } else {
        g_warning ("NULL object encountered");
      }
    }

    g_value_unset (&value);
  }
}
