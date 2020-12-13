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
#include <stdlib.h>
#include <sys/types.h>
#ifdef HAVE_SYS_PARAM_H
#include <sys/param.h>
#endif
#include <sys/stat.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#ifdef HAVE_STRING_H
#include <string.h>
#endif

#include "gschem.h"
#include "actions.decl.x"
#include <gdk/gdkkeysyms.h>

#include "../include/gschem_compselect_dockable.h"

/*! \def COMPSELECT_FILTER_INTERVAL
 *  \brief The time interval between request and actual filtering
 *
 *  This constant is the time-lag between user modifications in the
 *  filter entry and the actual evaluation of the filter which
 *  ultimately update the display. It helps reduce the frequency of
 *  evaluation of the filter as user types.
 *
 *  Unit is milliseconds.
 */
#define COMPSELECT_FILTER_INTERVAL 200


enum compselect_behavior {
  BEHAVIOR_REFERENCE,
  BEHAVIOR_EMBED,
  BEHAVIOR_INCLUDE
};

enum {
  INUSE_COLUMN_SYMBOL,
  N_INUSE_COLUMNS
};

enum {
  LIB_COLUMN_SYMBOL_OR_SOURCE,
  LIB_COLUMN_NAME,
  LIB_COLUMN_IS_SYMBOL,
  N_LIB_COLUMNS
};

/* Both the inuse model and symbol rows of the lib model store the
   symbol pointer in column 0.  Define a special constant which is
   used whenever we depend on this fact. */
enum {
  COMMON_COLUMN_SYMBOL
};

enum {
  ATTRIBUTE_COLUMN_NAME = 0,
  ATTRIBUTE_COLUMN_VALUE,
  NUM_ATTRIBUTE_COLUMNS
};


static GObjectClass *compselect_parent_class = NULL;


static void compselect_class_init      (GschemCompselectDockableClass *class);
static void compselect_constructed     (GObject *object);
static void compselect_dispose         (GObject *object);
static void compselect_finalize        (GObject *object);

static GtkWidget *compselect_create_widget (GschemDockable *dockable);


static void
update_attributes_model (GschemCompselectDockable *compselect, gchar *filename);
static void
compselect_callback_tree_selection_changed (GtkTreeSelection *selection,
                                            gpointer          user_data);


static void
compselect_place (GschemCompselectDockable *compselect)
{
  GschemToplevel *w_current = GSCHEM_DOCKABLE (compselect)->w_current;
  TOPLEVEL *toplevel = gschem_toplevel_get_toplevel (w_current);

  enum compselect_behavior behavior =
    gtk_combo_box_get_active (compselect->combobox_behaviors);

        w_current->include_complex = w_current->embed_complex = 0;
        switch (behavior) {
            case BEHAVIOR_REFERENCE:
              break;
            case BEHAVIOR_EMBED:
              w_current->embed_complex   = 1;
              break;
            case BEHAVIOR_INCLUDE:
              w_current->include_complex = 1;
              break;
            default:
              g_assert_not_reached ();
        }

        if (w_current->event_state == COMPMODE) {
          /* Delete the component which was being placed */
          if (w_current->rubber_visible)
            o_place_invalidate_rubber (w_current, FALSE);
          w_current->rubber_visible = 0;
          s_delete_object_glist (toplevel,
                                 toplevel->page_current->place_list);
          toplevel->page_current->place_list = NULL;
        } else {
          /* Cancel whatever other action is currently in progress */
          o_redraw_cleanstates (w_current);
        }

        if (compselect->selected_symbol == NULL) {
          /* If there is no symbol selected, switch to SELECT mode */
          i_set_state (w_current, SELECT);
          i_action_stop (w_current);
        } else {
          /* Otherwise set the new symbol to place */
          o_complex_prepare_place (w_current, compselect->selected_symbol);
        }
}


static void
select_symbol (GschemCompselectDockable *compselect, CLibSymbol *symbol)
{
  if (symbol == compselect->selected_symbol)
    return;
  compselect->selected_symbol = symbol;

  /* update in-use and library selections */
  GtkTreeSelection *selection;
  GtkTreeModel *model;
  GtkTreeIter iter;

  selection = gtk_tree_view_get_selection (compselect->inusetreeview);
  if (gtk_tree_selection_get_selected (selection, &model, &iter)) {
    CLibSymbol *sym = NULL;
    gtk_tree_model_get (model, &iter, INUSE_COLUMN_SYMBOL, &sym, -1);
    if (sym != symbol) {
      g_signal_handlers_block_by_func (
        selection, compselect_callback_tree_selection_changed, compselect);
      gtk_tree_selection_unselect_all (selection);
      g_signal_handlers_unblock_by_func (
        selection, compselect_callback_tree_selection_changed, compselect);
    }
  }

  selection = gtk_tree_view_get_selection (compselect->libtreeview);
  if (gtk_tree_selection_get_selected (selection, &model, &iter)) {
    CLibSymbol *sym = NULL;
    gboolean is_sym = FALSE;
    gtk_tree_model_get (model, &iter, LIB_COLUMN_SYMBOL_OR_SOURCE, &sym,
                                      LIB_COLUMN_IS_SYMBOL, &is_sym, -1);
    if (is_sym && sym != symbol) {
      g_signal_handlers_block_by_func (
        selection, compselect_callback_tree_selection_changed, compselect);
      gtk_tree_selection_unselect_all (selection);
      g_signal_handlers_unblock_by_func (
        selection, compselect_callback_tree_selection_changed, compselect);
    }
  }

  /* update the preview with new symbol data */
  gchar *buffer = symbol ? s_clib_symbol_get_data (symbol) : NULL;
  g_object_set (compselect->preview,
                "buffer", buffer,
                "active", buffer != NULL,
                NULL);
  g_free (buffer);

  /* update the attributes with the toplevel of the preview widget*/
  if (symbol == NULL) {
    compselect->is_selected = FALSE;
    update_attributes_model (compselect, NULL);
  } else {
    g_free (compselect->selected_filename);
    compselect->selected_filename = s_clib_symbol_get_filename (symbol);
    compselect->is_selected = TRUE;
    update_attributes_model (compselect, compselect->selected_filename);

    gschem_action_set_sensitive (action_add_last_component,
                                 compselect->selected_filename != NULL,
                                 GSCHEM_DOCKABLE (compselect)->w_current);
  }

  /* signal a component has been selected to parent of dockable */
  compselect_place (compselect);
}


static void
compselect_cancel (GschemDockable *dockable)
{
  GschemToplevel *w_current = dockable->w_current;

  if (w_current->event_state == COMPMODE) {
    /* Cancel the place operation currently in progress */
    o_redraw_cleanstates (w_current);

    /* return to the default state */
    i_set_state (w_current, SELECT);
    i_action_stop (w_current);
  }
}


static void
compselect_post_present (GschemDockable *dockable)
{
  GschemCompselectDockable *compselect = GSCHEM_COMPSELECT_DOCKABLE (dockable);

  GtkWidget *current_tab, *entry_filter;
  GtkNotebook *compselect_notebook;

  gtk_editable_select_region (GTK_EDITABLE (compselect->entry_filter), 0, -1);

  /* Set the focus to the filter entry only if it is in the current
     displayed tab */
  compselect_notebook = GTK_NOTEBOOK (compselect->viewtabs);
  current_tab = gtk_notebook_get_nth_page (compselect_notebook,
                                           gtk_notebook_get_current_page (compselect_notebook));
  entry_filter = GTK_WIDGET (compselect->entry_filter);
  if (gtk_widget_is_ancestor (entry_filter, current_tab)) {
    gtk_widget_grab_focus (entry_filter);
  }
}


void
x_compselect_deselect (GschemToplevel *w_current)
{
  GschemCompselectDockable *compselect =
    GSCHEM_COMPSELECT_DOCKABLE (w_current->compselect_dockable);

  select_symbol (compselect, NULL);
}


/*! \brief Sets data for a particular cell of the in use treeview.
 *  \par Function Description
 *  This function determines what data is to be displayed in the
 *  "in use" symbol selection view.
 *
 *  The model is a list of symbols. s_clib_symbol_get_name() is called
 *  to get the text to display.
 */
static void
inuse_treeview_set_cell_data (GtkTreeViewColumn *tree_column,
                            GtkCellRenderer   *cell,
                            GtkTreeModel      *tree_model,
                            GtkTreeIter       *iter,
                            gpointer           data)
{
  CLibSymbol *symbol;

  gtk_tree_model_get (tree_model, iter, INUSE_COLUMN_SYMBOL, &symbol, -1);
  g_object_set (G_OBJECT (cell), "text", s_clib_symbol_get_name (symbol), NULL);
}

/*! \brief Returns whether a library treeview node represents a symbol. */

static gboolean is_symbol(GtkTreeModel *tree_model, GtkTreeIter *iter)
{
  gboolean result;
  gtk_tree_model_get (tree_model, iter, LIB_COLUMN_IS_SYMBOL, &result, -1);
  return result;
}

/*! \brief Determines visibility of items of the library treeview.
 *  \par Function Description
 *  This is the function used to filter entries of the component
 *  selection tree.
 *
 *  \param [in] model The current selection in the treeview.
 *  \param [in] iter  An iterator on a component or folder in the tree.
 *  \param [in] data  The component selection dockable.
 *  \returns TRUE if item should be visible, FALSE otherwise.
 */
static gboolean
lib_model_filter_visible_func (GtkTreeModel *model,
                                      GtkTreeIter  *iter,
                                      gpointer      data)
{
  GschemCompselectDockable *compselect = GSCHEM_COMPSELECT_DOCKABLE (data);
  CLibSymbol *sym;
  const gchar *compname;
  gchar *compname_upper, *text_upper, *pattern;
  const gchar *text;
  gboolean ret;

  g_assert (GSCHEM_IS_COMPSELECT_DOCKABLE (data));

  if (compselect->entry_filter == NULL)
    return TRUE;
  text = gtk_entry_get_text (compselect->entry_filter);
  if (g_ascii_strcasecmp (text, "") == 0) {
    return TRUE;
  }

  /* If this is a source, only display it if it has children that
   * match */
  if (!is_symbol (model, iter)) {
    GtkTreeIter iter2;

    ret = FALSE;
    if (gtk_tree_model_iter_children (model, &iter2, iter))
      do {
        if (lib_model_filter_visible_func (model, &iter2, data)) {
          ret = TRUE;
          break;
        }
      } while (gtk_tree_model_iter_next (model, &iter2));
  } else {
    gtk_tree_model_get (model, iter,
                        LIB_COLUMN_SYMBOL_OR_SOURCE, &sym,
                        -1);
    compname = s_clib_symbol_get_name (sym);
    /* Do a case insensitive comparison, converting the strings
       to uppercase */
    compname_upper = g_ascii_strup (compname, -1);
    text_upper = g_ascii_strup (text, -1);
    pattern = g_strconcat ("*", text_upper, "*", NULL);
    ret = g_pattern_match_simple (pattern, compname_upper);
    g_free (compname_upper);
    g_free (text_upper);
    g_free (pattern);
  }

  return ret;
}


/*! \brief Handles activation (e.g. double-clicking) of a component row
 *  \par Function Description
 *  Component row activated handler:
 *  As a convenience to the user, expand / contract any node with children.
 *  Hide the component selector if a node without children is activated.
 *
 *  \param [in] tree_view The component treeview.
 *  \param [in] path      The GtkTreePath to the activated row.
 *  \param [in] column    The GtkTreeViewColumn in which the activation occurred.
 *  \param [in] user_data The component selection dockable.
 */
static void
tree_row_activated (GtkTreeView       *tree_view,
                    GtkTreePath       *path,
                    GtkTreeViewColumn *column,
                    gpointer           user_data)
{
  GtkTreeModel *model;
  GtkTreeIter iter;
  GschemCompselectDockable *compselect = GSCHEM_COMPSELECT_DOCKABLE (user_data);

  model = gtk_tree_view_get_model (tree_view);
  gtk_tree_model_get_iter (model, &iter, path);

  if (tree_view == compselect->inusetreeview ||
      /* No special handling required */
      (tree_view == compselect->libtreeview && is_symbol (model, &iter))) {
       /* Tree view needs to check that we're at a symbol node */
    CLibSymbol *symbol = NULL;
    gtk_tree_model_get (model, &iter, COMMON_COLUMN_SYMBOL, &symbol, -1);
    select_symbol (compselect, symbol);

    GschemDockable *dockable = GSCHEM_DOCKABLE (compselect);
    switch (gschem_dockable_get_state (dockable)) {
      case GSCHEM_DOCKABLE_STATE_DIALOG:
        /* if shown as dialog, hide */
        gschem_dockable_hide (dockable);
        return;
      case GSCHEM_DOCKABLE_STATE_WINDOW:
        /* if shown as detached window, focus main window */
        gtk_widget_grab_focus (dockable->w_current->drawing_area);
        x_window_present (dockable->w_current);
      default:
        /* if docked, focus drawing area */
        gtk_widget_grab_focus (dockable->w_current->drawing_area);
    }
    return;
  }

  if (gtk_tree_view_row_expanded (tree_view, path))
    gtk_tree_view_collapse_row (tree_view, path);
  else
    gtk_tree_view_expand_row (tree_view, path, FALSE);
}

/*! \brief GCompareFunc to sort an text object list by the object strings
 */
static gint
sort_object_text (OBJECT *a, OBJECT *b)
{
  return strcmp (a->text->string, b->text->string);
}

/*! \brief Update the model of the attributes treeview
 *  \par Function Description
 *  This function takes the toplevel attributes from the preview widget and
 *  puts them into the model of the <b>attrtreeview</b> widget.
 *  \param [in] compselect       The dockable compselect
 *  \param [in] preview_toplevel The toplevel of the preview widget
 */
static void
update_attributes_model (GschemCompselectDockable *compselect, gchar *filename)
{
  GtkListStore *model;
  GtkTreeIter iter;
  GtkTreeViewColumn *column;
  GList *o_iter, *o_attrlist;
  gchar *name, *value;
  OBJECT *o_current;
  EdaConfig *cfg;
  gchar **filter_list;
  gint i;
  gsize n;

  model = GTK_LIST_STORE (gtk_tree_view_get_model (compselect->attrtreeview));
  gtk_list_store_clear (model);

  /* Invalidate the column width for the attribute value column, so
   * the column is re-sized based on the new data being shown. Symbols
   * with long values are common, and we don't want having viewed those
   * forcing a h-scroll-bar on symbols with short valued attributes.
   *
   * We might also consider invalidating the attribute name columns,
   * however that gives an inconsistent column division when swithing
   * between symbols, which doesn't look nice. For now, assume that
   * the name column can keep the max width gained whilst previewing.
   */
  column = gtk_tree_view_get_column (compselect->attrtreeview,
                                     ATTRIBUTE_COLUMN_VALUE);
  gtk_tree_view_column_queue_resize (column);

  PAGE *preview_page = gschem_page_view_get_page (
                         GSCHEM_PAGE_VIEW (compselect->preview));
  if (preview_page == NULL)
    return;

  o_attrlist = o_attrib_find_floating_attribs (s_page_objects (preview_page));

  cfg = filename != NULL ? eda_config_get_context_for_path (filename)
                         : eda_config_get_user_context ();
  filter_list = eda_config_get_string_list (cfg, "gschem.library",
                                            "component-attributes", &n, NULL);

  if (filter_list == NULL || (n > 0 && strcmp (filter_list[0], "*") == 0)) {
    /* display all attributes in alphabetical order */
    o_attrlist = g_list_sort (o_attrlist, (GCompareFunc) sort_object_text);
    for (o_iter = o_attrlist; o_iter != NULL; o_iter = g_list_next (o_iter)) {
      o_current = o_iter->data;
      o_attrib_get_name_value (o_current, &name, &value);
      gtk_list_store_append (model, &iter);
      gtk_list_store_set (model, &iter,
                          ATTRIBUTE_COLUMN_NAME, name,
                          ATTRIBUTE_COLUMN_VALUE, value, -1);
      g_free (name);
      g_free (value);
    }
  } else {
    /* display only attribute that are in the filter list */
    for (i = 0; i < n; i++) {
      for (o_iter = o_attrlist; o_iter != NULL; o_iter = g_list_next (o_iter)) {
        o_current = o_iter->data;
        if (o_attrib_get_name_value (o_current, &name, &value)) {
          if (strcmp (name, filter_list[i]) == 0) {
            gtk_list_store_append (model, &iter);
            gtk_list_store_set (model, &iter,
                                ATTRIBUTE_COLUMN_NAME, name,
                                ATTRIBUTE_COLUMN_VALUE, value, -1);
          }
          g_free (name);
          g_free (value);
        }
      }
    }
  }

  g_strfreev (filter_list);
  g_list_free (o_attrlist);
}

/*! \brief Handles changes in the treeview selection.
 *  \par Function Description
 *  This is the callback function that is called every time the user
 *  select a row in either component treeview of the dockable.
 *
 *  If the selection is not a selection of a component (a directory
 *  name), it does nothing. Otherwise it retrieves the #CLibSymbol
 *  from the model.
 *
 *  It then calls compselect_place to let its parent know that a
 *  component has been selected.
 *
 *  \param [in] selection The current selection in the treeview.
 *  \param [in] user_data The component selection dockable.
 */
static void
compselect_callback_tree_selection_changed (GtkTreeSelection *selection,
                                            gpointer          user_data)
{
  GtkTreeView *view;
  GtkTreeModel *model;
  GtkTreeIter iter;
  GschemCompselectDockable *compselect = GSCHEM_COMPSELECT_DOCKABLE (user_data);
  CLibSymbol *sym = NULL;

  if (gtk_tree_selection_get_selected (selection, &model, &iter)) {

    view = gtk_tree_selection_get_tree_view (selection);
    if (view == compselect->inusetreeview ||
        /* No special handling required */
        (view == compselect->libtreeview && is_symbol (model, &iter))) {
         /* Tree view needs to check that we're at a symbol node */

      gtk_tree_model_get (model, &iter, COMMON_COLUMN_SYMBOL, &sym, -1);
    }
  }

  select_symbol (compselect, sym);
}

/*! \brief Requests re-evaluation of the filter.
 *  \par Function Description
 *  This is the timeout function for the filtering of component in the
 *  tree of the dockable.
 *
 *  The timeout this callback is attached to is removed after the
 *  function.
 *
 *  \param [in] data The component selection dockable.
 *  \returns FALSE to remove the timeout.
 */
static gboolean
compselect_filter_timeout (gpointer data)
{
  GschemCompselectDockable *compselect = GSCHEM_COMPSELECT_DOCKABLE (data);
  GtkTreeModel *model;

  /* resets the source id in compselect */
  compselect->filter_timeout = 0;

  model = gtk_tree_view_get_model (compselect->libtreeview);

  if (model != NULL) {
    const gchar *text = gtk_entry_get_text (compselect->entry_filter);
    gtk_tree_model_filter_refilter (GTK_TREE_MODEL_FILTER (model));
    if (strcmp (text, "") != 0) {
      /* filter text not-empty */
      gtk_tree_view_expand_all (compselect->libtreeview);
    } else {
      /* filter text is empty, collapse expanded tree */
      gtk_tree_view_collapse_all (compselect->libtreeview);
    }
  }

  /* return FALSE to remove the source */
  return FALSE;
}

/*! \brief Callback function for the changed signal of the filter entry.
 *  \par Function Description
 *  This function monitors changes in the entry filter of the dockable.
 *
 *  It specifically manages the sensitivity of the clear button of the
 *  entry depending on its contents. It also requests an update of the
 *  component list by re-evaluating filter at every changes.
 *
 *  \param [in] editable  The filter text entry.
 *  \param [in] user_data The component selection dockable.
 */
static void
compselect_callback_filter_entry_changed (GtkEditable *editable,
                                          gpointer  user_data)
{
  GschemCompselectDockable *compselect = GSCHEM_COMPSELECT_DOCKABLE (user_data);
  GtkWidget *button;
  gboolean sensitive;

  /* turns button off if filter entry is empty */
  /* turns it on otherwise */
  button    = GTK_WIDGET (compselect->button_clear);
  sensitive =
    (g_ascii_strcasecmp (gtk_entry_get_text (compselect->entry_filter),
                         "") != 0);
  if (gtk_widget_is_sensitive (button) != sensitive) {
    gtk_widget_set_sensitive (button, sensitive);
  }

  /* Cancel any pending update of the component list filter */
  if (compselect->filter_timeout != 0)
    g_source_remove (compselect->filter_timeout);

  /* Schedule an update of the component list filter in
   * COMPSELECT_FILTER_INTERVAL milliseconds */
  compselect->filter_timeout = g_timeout_add (COMPSELECT_FILTER_INTERVAL,
                                              compselect_filter_timeout,
                                              compselect);

}

/*! \brief Handles a click on the clear button.
 *  \par Function Description
 *  This is the callback function called every time the user press the
 *  clear button associated with the filter.
 *
 *  It resets the filter entry, indirectly causing re-evaluation
 *  of the filter on the list of symbols to update the display.
 *
 *  \param [in] button    The clear button
 *  \param [in] user_data The component selection dockable.
 */
static void
compselect_callback_filter_button_clicked (GtkButton *button,
                                           gpointer   user_data)
{
  GschemCompselectDockable *compselect = GSCHEM_COMPSELECT_DOCKABLE (user_data);

  /* clears text in text entry for filter */
  gtk_entry_set_text (compselect->entry_filter, "");

}

/*! \brief Handles changes of behavior.
 *  \par Function Description
 *  This function is called every time the value of the option menu
 *  for behaviors is modified.
 *
 *  It calls compselect_place to let the parent know that the
 *  requested behavior for the next adding of a component has been
 *  changed.
 *
 *  \param [in] optionmenu The behavior option menu.
 *  \param [in] user_data  The component selection dockable.
 */
static void
compselect_callback_behavior_changed (GtkOptionMenu *optionmenu,
                                      gpointer user_data)
{
  GschemCompselectDockable *compselect = GSCHEM_COMPSELECT_DOCKABLE (user_data);

  compselect_place (compselect);
}

/* \brief Create the tree model for the "In Use" view.
 * \par Function Description
 * Creates a straightforward list of symbols which are currently in
 * use, using s_toplevel_get_symbols().
 */
static void
create_inuse_tree_model (GschemCompselectDockable *compselect)
{
  GtkListStore *store;
  GList *symhead, *symlist;
  GtkTreeIter iter;

  store = GTK_LIST_STORE (gtk_list_store_new (N_INUSE_COLUMNS, G_TYPE_POINTER));

  symhead = s_toplevel_get_symbols (
    GSCHEM_DOCKABLE (compselect)->w_current->toplevel);

  for (symlist = symhead;
       symlist != NULL;
       symlist = g_list_next (symlist)) {

    gtk_list_store_append (store, &iter);

    gtk_list_store_set (store, &iter,
                        INUSE_COLUMN_SYMBOL, symlist->data,
                        -1);
  }

  g_list_free (symhead);

  gtk_tree_view_set_model (compselect->inusetreeview, GTK_TREE_MODEL (store));
  g_object_unref (store);  /* release initially owned reference */
}

/* \brief Helper function for create_lib_tree_model. */

static void populate_component_store(GtkTreeStore *store, GList **srclist,
                                     GtkTreeIter *parent, const char *prefix)
{
  CLibSource *source = (CLibSource *)(*srclist)->data;
  const char *name = s_clib_source_get_name (source);

  char *text, *new_prefix;
  GList *new_srclist;

  if (*name == '\0') {
    text = NULL;
    new_prefix = NULL;
    new_srclist = NULL;
  } else if (*name != '/') {
    /* directory added by component-library */
    text = g_strdup (name);
    new_prefix = NULL;
    new_srclist = NULL;
  } else {
    /* directory added by component-library-search */
    size_t prefix_len = strlen (prefix);
    g_assert (strncmp (name, prefix, prefix_len) == 0);
    char *p = strchr (name + prefix_len + 1, '/');

    if (p != NULL) {
      /* There is a parent directory that was skipped
         because it doesn't contain symbols. */
      source = NULL;
      text = g_strndup (name + prefix_len, p - name - prefix_len);
      new_prefix = g_strndup (name, p - name + 1);
      new_srclist = *srclist;
    } else {
      size_t name_len = strlen(name);
      text = g_strndup (name + prefix_len, name_len - prefix_len);
      new_prefix = g_strndup (name, name_len + 1); /* reserve one extra byte */
      new_prefix[name_len] = '/';
      new_prefix[name_len + 1] = '\0';
      new_srclist = g_list_next (*srclist);
    }
  }

  GtkTreeIter iter;
  gtk_tree_store_append (store, &iter, parent);
  gtk_tree_store_set (store, &iter,
                      LIB_COLUMN_SYMBOL_OR_SOURCE, source,
                      LIB_COLUMN_NAME, text,
                      LIB_COLUMN_IS_SYMBOL, FALSE,
                      -1);
  g_free (text);

  /* Look ahead, adding subdirectories. */
  while (new_srclist != NULL &&
         strncmp(s_clib_source_get_name ((CLibSource *)new_srclist->data),
                 new_prefix, strlen(new_prefix)) == 0) {
    *srclist = new_srclist;
    populate_component_store(store, srclist, &iter, new_prefix);
    new_srclist = g_list_next (*srclist);
  }
  g_free (new_prefix);

  /* populate symbols */
  GList *symhead, *symlist;
  GtkTreeIter iter2;
  symhead = s_clib_source_get_symbols (source);
  for (symlist = symhead;
       symlist != NULL;
       symlist = g_list_next (symlist)) {

    gtk_tree_store_append (store, &iter2, &iter);
    gtk_tree_store_set (store, &iter2,
                        LIB_COLUMN_SYMBOL_OR_SOURCE, symlist->data,
                        LIB_COLUMN_NAME,
                          s_clib_symbol_get_name ((CLibSymbol *)symlist->data),
                        LIB_COLUMN_IS_SYMBOL, TRUE,
                        -1);
  }
  g_list_free (symhead);
}

/* \brief Create the tree model for the "Library" view.
 * \par Function Description
 * Creates a tree where the branches are the available component
 * sources and the leaves are the symbols.
 */
static void
create_lib_tree_model (GschemCompselectDockable *compselect)
{
  GtkTreeStore *store;
  GList *srchead, *srclist;
  EdaConfig *cfg = eda_config_get_user_context ();
  gboolean sort = eda_config_get_boolean (cfg, "gschem.library", "sort", NULL);

  store = GTK_TREE_STORE (gtk_tree_store_new (N_LIB_COLUMNS, G_TYPE_POINTER,
                                                             G_TYPE_STRING,
                                                             G_TYPE_BOOLEAN));

  /* populate component store */
  srchead = s_clib_get_sources (sort);
  for (srclist = srchead;
       srclist != NULL;
       srclist = g_list_next (srclist)) {
    populate_component_store(store, &srclist, NULL, "/");
  }
  g_list_free (srchead);


  /* create filtered model */
  GtkTreeModel *model = GTK_TREE_MODEL (
    g_object_new (GTK_TYPE_TREE_MODEL_FILTER,
                  "child-model", GTK_TREE_MODEL (store),
                  "virtual-root", NULL,
                  NULL));
  g_object_unref (store);  /* release initially owned reference */

  gtk_tree_model_filter_set_visible_func (GTK_TREE_MODEL_FILTER (model),
                                          lib_model_filter_visible_func,
                                          compselect, NULL);

  gtk_tree_view_set_model (compselect->libtreeview, model);
  g_object_unref (model);  /* release initially owned reference */

  /* re-expand library tree if filter text is not-empty */
  if (compselect->entry_filter != NULL &&
      gtk_entry_get_text (compselect->entry_filter)[0] != '\0')
    gtk_tree_view_expand_all (compselect->libtreeview);
}

/*! \brief Helper function for \ref select_symbol_by_filename.
 */
static gboolean
find_tree_iter_by_filename (GtkTreeModel *tree_model,
                            GtkTreeIter *iter_return,
                            GtkTreeIter *parent,
                            const gchar *filename)
{
  GtkTreeIter iter;

  if (gtk_tree_model_iter_children (tree_model, &iter, parent))
    do {
      if (gtk_tree_model_get_n_columns (tree_model) == N_INUSE_COLUMNS ||
          is_symbol (tree_model, &iter)) {
        /* node is a symbol */
        CLibSymbol *symbol;
        gtk_tree_model_get (tree_model, &iter,
                            COMMON_COLUMN_SYMBOL, &symbol, -1);
        gchar *fn = s_clib_symbol_get_filename (symbol);
        if (strcmp (fn, filename) == 0) {
          *iter_return = iter;
          g_free (fn);
          return TRUE;
        }
        g_free (fn);
        continue;
      }

      /* node is a source */
      CLibSource *source;
      gtk_tree_model_get (tree_model, &iter,
                          LIB_COLUMN_SYMBOL_OR_SOURCE, &source, -1);

      gboolean recurse;
      if (source == NULL)
        /* this is a virtual node that was added for sub-directory sources */
        recurse = TRUE;
      else {
        const gchar *directory = s_clib_source_get_directory (source);
        recurse = strncmp (directory, filename, strlen (directory)) == 0 &&
                  filename[strlen (directory)] == '/';
      }
      if (recurse &&
          find_tree_iter_by_filename (tree_model, iter_return, &iter, filename))
        return TRUE;
    } while (gtk_tree_model_iter_next (tree_model, &iter));

  return FALSE;
}

static void
select_symbol_by_filename (GschemCompselectDockable *compselect,
                           const gchar *filename)
{
  g_return_if_fail (filename != NULL);

  GtkTreeView *tree_view = NULL;
  switch (gtk_notebook_get_current_page (compselect->viewtabs)) {
  case 0:
    tree_view = compselect->inusetreeview;
    break;
  case 1:
    tree_view = compselect->libtreeview;
    break;
  default:
    g_assert_not_reached ();
  }

  GtkTreeModel *model = gtk_tree_view_get_model (tree_view);
  GtkTreeIter iter;
  if (!find_tree_iter_by_filename (model, &iter, NULL, filename))
    /* no matching symbol found */
    return;

  /* expand the path to the node so it can be selected */
  GtkTreePath *path = gtk_tree_model_get_path (model, &iter);
  if (gtk_tree_path_up (path))
    gtk_tree_view_expand_to_path (tree_view, path);
  gtk_tree_path_free (path);

  /* now select the node */
  gtk_tree_selection_select_iter (
    gtk_tree_view_get_selection (tree_view), &iter);
}

static void
library_updated (gpointer user_data)
{
  GschemCompselectDockable *compselect = GSCHEM_COMPSELECT_DOCKABLE (user_data);
  gboolean was_selected = compselect->is_selected;

  /* Refresh the "Library" view */
  create_lib_tree_model (compselect);

  /* Refresh the "In Use" view */
  create_inuse_tree_model (compselect);

  /* re-select previously selected symbol */
  if (was_selected && compselect->selected_filename != NULL)
    select_symbol_by_filename (compselect, compselect->selected_filename);
}

void
x_compselect_select_previous_symbol (GschemToplevel *w_current)
{
  GschemCompselectDockable *compselect =
    GSCHEM_COMPSELECT_DOCKABLE (w_current->compselect_dockable);

  if (compselect->selected_filename != NULL)
    select_symbol_by_filename (compselect, compselect->selected_filename);
}

/* \brief On-demand refresh of the component library.
 * \par Function Description
 * Requests a rescan of the component library in order to pick up any
 * new signals, and then updates the component selector.
 */
static void
compselect_callback_refresh_library (GtkButton *button, gpointer user_data)
{
  /* Rescan the libraries for symbols */
  s_clib_refresh ();
}

/*! \brief Creates the treeview for the "In Use" view. */
static GtkWidget*
create_inuse_treeview (GschemCompselectDockable *compselect)
{
  GtkWidget *scrolled_win, *treeview, *vbox, *hbox, *button;
  GtkTreeSelection *selection;
  GtkCellRenderer *renderer;
  GtkTreeViewColumn *column;

  vbox = GTK_WIDGET (g_object_new (GTK_TYPE_VBOX,
                                   /* GtkContainer */
                                   "border-width", 5,
                                   /* GtkBox */
                                   "homogeneous",  FALSE,
                                   "spacing",      5,
                                   NULL));

  /* Create a scrolled window to accomodate the treeview */
  scrolled_win = GTK_WIDGET (
    g_object_new (GTK_TYPE_SCROLLED_WINDOW,
                  /* GtkContainer */
                  "border-width", 5,
                  /* GtkScrolledWindow */
                  "hscrollbar-policy", GTK_POLICY_AUTOMATIC,
                  "vscrollbar-policy", GTK_POLICY_ALWAYS,
                  "shadow-type",       GTK_SHADOW_ETCHED_IN,
                  NULL));

  /* Create the treeview */
  treeview = GTK_WIDGET (g_object_new (GTK_TYPE_TREE_VIEW,
                                       /* GtkTreeView */
                                       "rules-hint", TRUE,
                                       "headers-visible", FALSE,
                                       NULL));

  /* set the inuse treeview of compselect */
  compselect->inusetreeview = GTK_TREE_VIEW (treeview);

  create_inuse_tree_model (compselect);

  g_signal_connect (treeview,
                    "row-activated",
                    G_CALLBACK (tree_row_activated),
                    compselect);

  /* Connect callback to selection */
  selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (treeview));
  gtk_tree_selection_set_mode (selection, GTK_SELECTION_SINGLE);
  g_signal_connect (selection,
                    "changed",
                    G_CALLBACK (compselect_callback_tree_selection_changed),
                    compselect);

  /* Insert a column for symbol name */
  renderer = GTK_CELL_RENDERER (
    g_object_new (GTK_TYPE_CELL_RENDERER_TEXT,
                  /* GtkCellRendererText */
                  "editable", FALSE,
                  NULL));
  column = GTK_TREE_VIEW_COLUMN (
    g_object_new (GTK_TYPE_TREE_VIEW_COLUMN,
                  /* GtkTreeViewColumn */
                  "title", _("Symbols"),
                  NULL));
  gtk_tree_view_column_pack_start (column, renderer, TRUE);
  gtk_tree_view_column_set_cell_data_func (column, renderer,
                                           inuse_treeview_set_cell_data,
                                           NULL, NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (treeview), column);

  /* Add the treeview to the scrolled window */
  gtk_container_add (GTK_CONTAINER (scrolled_win), treeview);

  /* add the scrolled window for directories to the vertical box */
  gtk_box_pack_start (GTK_BOX (vbox), scrolled_win,
                      TRUE, TRUE, 0);

  /* -- refresh button area -- */
  hbox = GTK_WIDGET (g_object_new (GTK_TYPE_HBOX,
                                          /* GtkBox */
                                          "homogeneous", FALSE,
                                          "spacing",     3,
                                          NULL));
  /* create the refresh button */
  button = GTK_WIDGET (g_object_new (GTK_TYPE_BUTTON,
                                     /* GtkWidget */
                                     "sensitive", TRUE,
                                     /* GtkButton */
                                     "relief",    GTK_RELIEF_NONE,
                                     NULL));
  gtk_container_add (GTK_CONTAINER (button),
                     gtk_image_new_from_stock (GTK_STOCK_REFRESH,
                                            GTK_ICON_SIZE_SMALL_TOOLBAR));
  /* add the refresh button to the horizontal box at the end */
  gtk_box_pack_end (GTK_BOX (hbox), button, FALSE, FALSE, 0);
  g_signal_connect (button,
                    "clicked",
                    G_CALLBACK (compselect_callback_refresh_library),
                    compselect);

  /* add the refresh button area to the vertical box */
  gtk_box_pack_start (GTK_BOX (vbox), hbox,
                      FALSE, FALSE, 0);

  return vbox;
}

/*! \brief Creates the treeview for the "Library" view */
static GtkWidget *
create_lib_treeview (GschemCompselectDockable *compselect)
{
  GtkWidget *libtreeview, *vbox, *scrolled_win, *label,
    *hbox, *entry, *button;
  GtkTreeSelection *selection;
  GtkCellRenderer *renderer;
  GtkTreeViewColumn *column;

  /* -- library selection view -- */

  /* vertical box for component selection and search entry */
  vbox = GTK_WIDGET (g_object_new (GTK_TYPE_VBOX,
                                   /* GtkContainer */
                                   "border-width", 5,
                                   /* GtkBox */
                                   "homogeneous",  FALSE,
                                   "spacing",      5,
                                   NULL));

  scrolled_win = GTK_WIDGET (
    g_object_new (GTK_TYPE_SCROLLED_WINDOW,
                  /* GtkContainer */
                  "border-width", 5,
                  /* GtkScrolledWindow */
                  "hscrollbar-policy", GTK_POLICY_AUTOMATIC,
                  "vscrollbar-policy", GTK_POLICY_ALWAYS,
                  "shadow-type",       GTK_SHADOW_ETCHED_IN,
                  NULL));
  /* create the treeview */
  libtreeview = GTK_WIDGET (g_object_new (GTK_TYPE_TREE_VIEW,
                                          /* GtkTreeView */
                                          "rules-hint", TRUE,
                                          "headers-visible", FALSE,
                                          NULL));

  /* set directory/component treeview of compselect */
  compselect->libtreeview = GTK_TREE_VIEW (libtreeview);

  create_lib_tree_model (compselect);

  g_signal_connect (libtreeview,
                    "row-activated",
                    G_CALLBACK (tree_row_activated),
                    compselect);

  /* connect callback to selection */
  selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (libtreeview));
  gtk_tree_selection_set_mode (selection, GTK_SELECTION_SINGLE);
  g_signal_connect (selection,
                    "changed",
                    G_CALLBACK (compselect_callback_tree_selection_changed),
                    compselect);

  /* insert a column to treeview for library/symbol name */
  renderer = GTK_CELL_RENDERER (
    g_object_new (GTK_TYPE_CELL_RENDERER_TEXT,
                  /* GtkCellRendererText */
                  "editable", FALSE,
                  NULL));
  column = GTK_TREE_VIEW_COLUMN (
    g_object_new (GTK_TYPE_TREE_VIEW_COLUMN,
                  /* GtkTreeViewColumn */
                  "title", _("Symbols"),
                  NULL));
  gtk_tree_view_column_pack_start (column, renderer, TRUE);
  gtk_tree_view_column_add_attribute (column, renderer, "text",
                                      LIB_COLUMN_NAME);
  gtk_tree_view_append_column (GTK_TREE_VIEW (libtreeview), column);

  /* add the treeview to the scrolled window */
  gtk_container_add (GTK_CONTAINER (scrolled_win), libtreeview);

  /* add the scrolled window for directories to the vertical box */
  gtk_box_pack_start (GTK_BOX (vbox), scrolled_win,
                      TRUE, TRUE, 0);


  /* -- filter area -- */
  hbox = GTK_WIDGET (g_object_new (GTK_TYPE_HBOX,
                                          /* GtkBox */
                                          "homogeneous", FALSE,
                                          "spacing",     3,
                                          NULL));

  /* create the entry label */
  label = GTK_WIDGET (g_object_new (GTK_TYPE_LABEL,
                                    /* GtkMisc */
                                    "xalign", 0.0,
                                    /* GtkLabel */
                                    "label",  _("Filter:"),
                                    NULL));
  /* add the search label to the filter area */
  gtk_box_pack_start (GTK_BOX (hbox), label,
                      FALSE, FALSE, 0);

  /* create the text entry for filter in components */
  entry = GTK_WIDGET (g_object_new (GTK_TYPE_ENTRY,
                                    /* GtkEntry */
                                    "text", "",
                                    NULL));
  gtk_widget_set_size_request (entry, 10, -1);
  g_signal_connect (entry,
                    "changed",
                    G_CALLBACK (compselect_callback_filter_entry_changed),
                    compselect);

  /* add the filter entry to the filter area */
  gtk_box_pack_start (GTK_BOX (hbox), entry,
                      TRUE, TRUE, 0);
  /* set filter entry of compselect */
  compselect->entry_filter = GTK_ENTRY (entry);
  /* and init the event source for component filter */
  compselect->filter_timeout = 0;

  /* create the erase button for filter entry */
  button = GTK_WIDGET (g_object_new (GTK_TYPE_BUTTON,
                                     /* GtkWidget */
                                     "sensitive", FALSE,
                                     /* GtkButton */
                                     "relief",    GTK_RELIEF_NONE,
                                     NULL));

  gtk_container_add (GTK_CONTAINER (button),
                     gtk_image_new_from_stock (GTK_STOCK_CLEAR,
                                               GTK_ICON_SIZE_SMALL_TOOLBAR));
  g_signal_connect (button,
                    "clicked",
                    G_CALLBACK (compselect_callback_filter_button_clicked),
                    compselect);
  /* add the clear button to the filter area */
  gtk_box_pack_start (GTK_BOX (hbox), button,
                      FALSE, FALSE, 0);
  /* set clear button of compselect */
  compselect->button_clear = GTK_BUTTON (button);

  /* create the refresh button */
  button = GTK_WIDGET (g_object_new (GTK_TYPE_BUTTON,
                                     /* GtkWidget */
                                     "sensitive", TRUE,
                                     /* GtkButton */
                                     "relief",    GTK_RELIEF_NONE,
                                     NULL));
  gtk_container_add (GTK_CONTAINER (button),
                     gtk_image_new_from_stock (GTK_STOCK_REFRESH,
                                            GTK_ICON_SIZE_SMALL_TOOLBAR));
  /* add the refresh button to the filter area */
  gtk_box_pack_start (GTK_BOX (hbox), button, FALSE, FALSE, 0);
  g_signal_connect (button,
                    "clicked",
                    G_CALLBACK (compselect_callback_refresh_library),
                    compselect);

  /* add the filter area to the vertical box */
  gtk_box_pack_start (GTK_BOX (vbox), hbox,
                      FALSE, FALSE, 0);

  compselect->libtreeview = GTK_TREE_VIEW (libtreeview);

  return vbox;
}

/*! \brief Creates the treeview widget for the attributes
 */
static GtkWidget*
create_attributes_treeview (GschemCompselectDockable *compselect)
{
  GtkWidget *attrtreeview, *scrolled_win;
  GtkListStore *model;
  GtkCellRenderer *renderer;
  GtkTreeViewColumn *column;

  model = gtk_list_store_new (NUM_ATTRIBUTE_COLUMNS,
                              G_TYPE_STRING, G_TYPE_STRING);

  attrtreeview = GTK_WIDGET (g_object_new (GTK_TYPE_TREE_VIEW,
                                           /* GtkTreeView */
                                           "model",      model,
                                           "headers-visible", FALSE,
                                           "rules-hint", TRUE,
                                           "tooltip-column",
                                             ATTRIBUTE_COLUMN_VALUE,
                                           NULL));

  /* two columns for name and value of the attributes */
  renderer = GTK_CELL_RENDERER (g_object_new (GTK_TYPE_CELL_RENDERER_TEXT,
                                              "editable", FALSE,
                                              NULL));

  column = GTK_TREE_VIEW_COLUMN (g_object_new (GTK_TYPE_TREE_VIEW_COLUMN,
                                               "title", _("Name"),
                                               "resizable", TRUE,
                                               NULL));
  gtk_tree_view_column_pack_start (column, renderer, TRUE);
  gtk_tree_view_column_add_attribute (column, renderer, "text",
                                      ATTRIBUTE_COLUMN_NAME);
  gtk_tree_view_append_column (GTK_TREE_VIEW (attrtreeview), column);

  column = GTK_TREE_VIEW_COLUMN (g_object_new (GTK_TYPE_TREE_VIEW_COLUMN,
                                               "title", _("Value"),
                                               "resizable", TRUE,
                                               NULL));
  gtk_tree_view_column_pack_start (column, renderer, TRUE);
  gtk_tree_view_column_add_attribute (column, renderer, "text",
                                      ATTRIBUTE_COLUMN_VALUE);
  gtk_tree_view_append_column (GTK_TREE_VIEW (attrtreeview), column);

  scrolled_win = GTK_WIDGET (g_object_new (GTK_TYPE_SCROLLED_WINDOW,
                                           /* GtkScrolledWindow */
                                           "hscrollbar-policy", GTK_POLICY_AUTOMATIC,
                                           "vscrollbar-policy", GTK_POLICY_AUTOMATIC,
                                           "shadow-type",       GTK_SHADOW_ETCHED_IN,
                                           NULL));

  gtk_container_add (GTK_CONTAINER (scrolled_win), attrtreeview);

  compselect->attrtreeview = GTK_TREE_VIEW (attrtreeview);

  return scrolled_win;
}

/*! \brief Create the combo box for behaviors.
 *  \par Function Description
 *  This function creates and returns a <B>GtkComboBox</B> for
 *  selecting the behavior when a component is added to the sheet.
 */
static GtkWidget*
create_behaviors_combo_box (void)
{
  GtkWidget *combobox;

  combobox = gtk_combo_box_new_text ();

  /* Note: order of items in menu is important */
  /* BEHAVIOR_REFERENCE */
  gtk_combo_box_append_text (GTK_COMBO_BOX (combobox),
                             _("Reference symbol (default)"));
  /* BEHAVIOR_EMBED */
  gtk_combo_box_append_text (GTK_COMBO_BOX (combobox),
                             _("Embed symbol in schematic"));
  /* BEHAVIOR_INCLUDE */
  gtk_combo_box_append_text (GTK_COMBO_BOX (combobox),
                             _("Include symbol as individual objects"));

  gtk_combo_box_set_active (GTK_COMBO_BOX (combobox), 0);

  return combobox;
}

GType
gschem_compselect_dockable_get_type ()
{
  static GType compselect_type = 0;

  if (!compselect_type) {
    static const GTypeInfo compselect_info = {
      sizeof (GschemCompselectDockableClass),
      NULL, /* base_init */
      NULL, /* base_finalize */
      (GClassInitFunc) compselect_class_init,
      NULL, /* class_finalize */
      NULL, /* class_data */
      sizeof (GschemCompselectDockable),
      0,    /* n_preallocs */
      NULL  /* instance_init */
    };

    compselect_type = g_type_register_static (GSCHEM_TYPE_DOCKABLE,
                                              "GschemCompselectDockable",
                                              &compselect_info, 0);
  }

  return compselect_type;
}


/*! \brief GschemDockable "save_internal_geometry" class method handler
 *
 *  \par Function Description
 *  Save the dockable's current internal geometry.
 *
 *  \param [in] dockable   The GschemCompselectDockable to save the geometry of.
 *  \param [in] key_file   The GKeyFile to save the geometry data to.
 *  \param [in] group_name The group name in the key file to store the data under.
 */
static void
compselect_save_internal_geometry (GschemDockable *dockable,
                                   EdaConfig *cfg,
                                   gchar *group_name)
{
  GschemCompselectDockable *compselect = GSCHEM_COMPSELECT_DOCKABLE (dockable);
  gint position;

  position = gtk_paned_get_position (GTK_PANED (compselect->hpaned));
  eda_config_set_int (cfg, group_name, "hpaned", position);

  position = gtk_paned_get_position (GTK_PANED (compselect->vpaned));
  eda_config_set_int (cfg, group_name, "vpaned", position);

  position = gtk_notebook_get_current_page (compselect->viewtabs);
  eda_config_set_int (cfg, group_name, "source-tab", position);

  eda_config_set_boolean (cfg, group_name, "preview-expanded",
                          gtk_expander_get_expanded (
                            GTK_EXPANDER (compselect->preview_expander)));

  eda_config_set_boolean (cfg, group_name, "attribs-expanded",
                          gtk_expander_get_expanded (
                            GTK_EXPANDER (compselect->attribs_expander)));
}


/*! \brief GschemDockable "restore_internal_geometry" class method handler
 *
 *  \par Function Description
 *  Restore the dockable's current internal geometry.
 *
 *  \param [in] dockable   The GschemCompselectDockable to restore the geometry of.
 *  \param [in] key_file   The GKeyFile to save the geometry data to.
 *  \param [in] group_name The group name in the key file to store the data under.
 */
static void
compselect_restore_internal_geometry (GschemDockable *dockable,
                                      EdaConfig *cfg,
                                      gchar *group_name)
{
  GschemCompselectDockable *compselect = GSCHEM_COMPSELECT_DOCKABLE (dockable);
  gint position;
  gboolean expanded;
  GError *error = NULL;

  position = eda_config_get_int (cfg, group_name, "hpaned", NULL);
  if (position == 0)
    position = 300;
  gtk_paned_set_position (GTK_PANED (compselect->hpaned), position);

  position = eda_config_get_int (cfg, group_name, "vpaned", NULL);
  if (position != 0)
    gtk_paned_set_position (GTK_PANED (compselect->vpaned), position);

  position = eda_config_get_int (cfg, group_name, "source-tab", &error);
  if (error != NULL) {
    position = 1;
    g_clear_error (&error);
  }
  gtk_notebook_set_current_page (compselect->viewtabs, position);

  expanded = eda_config_get_boolean (cfg, group_name, "preview-expanded", &error);
  if (error != NULL) {
    expanded = TRUE;
    g_clear_error (&error);
  }
  gtk_expander_set_expanded (GTK_EXPANDER (compselect->preview_expander),
                             expanded);

  expanded = eda_config_get_boolean (cfg, group_name, "attribs-expanded", &error);
  if (error != NULL) {
    expanded = TRUE;
    g_clear_error (&error);
  }
  gtk_expander_set_expanded (GTK_EXPANDER (compselect->attribs_expander),
                             expanded);
}


static void
compselect_class_init (GschemCompselectDockableClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  GschemDockableClass *gschem_dockable_class = GSCHEM_DOCKABLE_CLASS (klass);

  gschem_dockable_class->create_widget = compselect_create_widget;
  gschem_dockable_class->post_present = compselect_post_present;
  gschem_dockable_class->cancel = compselect_cancel;

  gschem_dockable_class->save_internal_geometry =
    compselect_save_internal_geometry;
  gschem_dockable_class->restore_internal_geometry =
    compselect_restore_internal_geometry;

  gobject_class->constructed  = compselect_constructed;
  gobject_class->dispose      = compselect_dispose;
  gobject_class->finalize     = compselect_finalize;

  compselect_parent_class = g_type_class_peek_parent (klass);
}

/*! \brief Make <widget> the child of <container>, removing other
 *         parent-child relations if necessary.
 */
static void
reparent (GtkWidget *container, GtkWidget *widget)
{
  GtkWidget *old_child = gtk_bin_get_child (GTK_BIN (container));
  GtkWidget *old_parent = widget ? gtk_widget_get_parent (widget) : NULL;
  if (old_child == widget && old_parent == container)
    return;

  if (old_child != NULL)
    gtk_container_remove (GTK_CONTAINER (container), old_child);
  if (old_parent != NULL)
    gtk_container_remove (GTK_CONTAINER (old_parent), widget);

  if (container != NULL && widget != NULL)
    gtk_container_add (GTK_CONTAINER (container), widget);
}

/*! \brief Make <top> and <bottom> the children of <container>,
 *         removing other parent-child relations if necessary.
 */
static void
reparent2 (GtkWidget *container, GtkWidget *top, GtkWidget *bottom)
{
  GtkWidget *top_parent = gtk_widget_get_parent (top);
  GtkWidget *bottom_parent = gtk_widget_get_parent (bottom);
  if (top_parent == container && bottom_parent == container)
    return;

  if (top_parent != NULL)
    gtk_container_remove (GTK_CONTAINER (top_parent), top);
  if (bottom_parent != NULL)
    gtk_container_remove (GTK_CONTAINER (bottom_parent), bottom);

  do {
    GList *old_children =
      gtk_container_get_children (GTK_CONTAINER (container));
    if (old_children == NULL)
      break;
    gtk_container_remove (GTK_CONTAINER (container), old_children->data);
  } while (1);

  if (GTK_IS_PANED (container)) {
    gtk_paned_pack1 (GTK_PANED (container), top, TRUE, FALSE);
    gtk_paned_pack2 (GTK_PANED (container), bottom, FALSE, FALSE);
  } else if (GTK_IS_BOX (container)) {
    gtk_box_pack_start (GTK_BOX (container), top, TRUE, TRUE, 0);
    gtk_box_pack_start (GTK_BOX (container), bottom, FALSE, FALSE, 0);
  } else
    g_assert_not_reached ();
}

/*! \brief Put the appropriate widget hierarchy for vertical layout in place.
 */
static void
compselect_update_vertical_hierarchy (GschemCompselectDockable *compselect)
{
  gboolean expanded0 = gtk_expander_get_expanded (
    GTK_EXPANDER (compselect->preview_expander));
  gboolean expanded1 = gtk_expander_get_expanded (
    GTK_EXPANDER (compselect->attribs_expander));

  GtkWidget *container0 = expanded0 ? compselect->preview_paned
                                    : compselect->preview_box;
  GtkWidget *container1 = expanded1 ? compselect->attribs_paned
                                    : compselect->attribs_box;

  reparent (compselect->preview_expander,
            expanded0 ? compselect->preview_content : NULL);
  reparent (compselect->attribs_expander,
            expanded1 ? compselect->attribs_content : NULL);

  reparent2 (container0, compselect->vbox, compselect->preview_expander);
  reparent2 (container1, container0, compselect->attribs_expander);
  reparent (compselect->top, container1);
}

/*! \brief Set the current layout for this dockable.
 *
 * Tiled layout is the classical behavior of gschem: the tree view is
 * on the left, preview and attributes are on the right.
 *
 * Vertical layout is used for tall aspect ratios: the tree view is at
 * the top, preview and attributes are below and expandable.
 */
static void
compselect_set_tiled (GschemCompselectDockable *compselect, gboolean tiled)
{
  GtkWidget *top_widget = gtk_bin_get_child (GTK_BIN (compselect->top));

  if (tiled) {
    if (top_widget == compselect->vbox)
      return;

    gtk_container_set_border_width (
      GTK_CONTAINER (compselect->preview_content), 5);
    gtk_container_set_border_width (
      GTK_CONTAINER (compselect->attribs_content), 5);

    reparent (compselect->preview_frame, compselect->preview_content);
    reparent (compselect->attribs_frame, compselect->attribs_content);
    reparent (compselect->top, compselect->vbox);
    gtk_widget_show (compselect->vpaned);

  } else {
    if (top_widget == compselect->attribs_paned ||
        top_widget == compselect->attribs_box)
      return;

    gtk_container_set_border_width (
      GTK_CONTAINER (compselect->preview_content), 0);
    gtk_container_set_border_width (
      GTK_CONTAINER (compselect->attribs_content), 0);

    gtk_widget_hide (compselect->vpaned);
    compselect_update_vertical_hierarchy (compselect);
  }
}

/*! \brief Return whether tiled layout is in effect.
 */
static gboolean
compselect_is_tiled (GschemCompselectDockable *compselect)
{
  GtkWidget *top_widget = gtk_bin_get_child (GTK_BIN (compselect->top));

  return top_widget != compselect->attribs_paned &&
         top_widget != compselect->attribs_box;
}

/*! \brief Remove prelight state from an expander.
 *
 * Expanders are prelit (painted in a shaded color) while the pointer
 * hovers over them.  This prelight state is not correctly removed
 * when the widget hierarchy changes.  This function synthesizes a
 * leave event to un-prelight the expander explicitly.
 */
static void
compselect_unprelight_expander (GschemCompselectDockable *compselect,
                                GtkWidget *expander)
{
  GdkWindow *window = NULL;
  if (expander == compselect->preview_expander)
    window = compselect->preview_expander_event_window;
  if (expander == compselect->attribs_expander)
    window = compselect->attribs_expander_event_window;

  if (window == NULL)
    return;

  GdkEvent *event = gdk_event_new (GDK_LEAVE_NOTIFY);
  event->crossing.window = g_object_ref (window);
  event->crossing.send_event = TRUE;
  event->crossing.subwindow = g_object_ref (window);
  event->crossing.time = GDK_CURRENT_TIME;
  event->crossing.x = 0;
  event->crossing.y = 0;
  event->crossing.x_root = 0;
  event->crossing.y_root = 0;
  event->crossing.mode = GDK_CROSSING_STATE_CHANGED;
  event->crossing.detail = GDK_NOTIFY_UNKNOWN;
  event->crossing.focus = FALSE;
  event->crossing.state = 0;

  gtk_widget_event (expander, event);
  gdk_event_free(event);
}

/*! \brief Callback function: Size of the whole dockable changed.
 *
 * Changes from and to tiled layout when the aspect ratio of the
 * dockable becomes wider and taller than 1:2, respectively.
 */
static void
compselect_top_size_allocate (GtkWidget *widget,
                              GdkRectangle *allocation,
                              gpointer user_data)
{
  compselect_set_tiled (GSCHEM_COMPSELECT_DOCKABLE (user_data),
                        allocation->width * 2 > allocation->height);
}

/*! \brief Callback function: Pointer hovers over expander.
 *
 * In order to synthesize leave notify events for the expanders, we
 * need pointers to their event windows.  These can't be accessed
 * directly, so as a workaround, wait for an enter notify event and
 * store the pointers for later.
 */
static gboolean
compselect_expander_enter_notify_event (GtkWidget *widget,
                                        GdkEvent *event,
                                        gpointer user_data)
{
  GschemCompselectDockable *compselect = GSCHEM_COMPSELECT_DOCKABLE (user_data);

  if (widget == compselect->preview_expander)
    compselect->preview_expander_event_window = event->crossing.window;
  if (widget == compselect->attribs_expander)
    compselect->attribs_expander_event_window = event->crossing.window;

  return FALSE;
}

/*! \brief Callback function: Expander activated.
 *
 * Update the widget hierarchy and prelight state of the expanders.
 */
static void
compselect_expander_notify_expanded (GObject *expander,
                                     GParamSpec *pspec,
                                     gpointer user_data)
{
  GschemCompselectDockable *compselect = GSCHEM_COMPSELECT_DOCKABLE (user_data);

  if (!compselect_is_tiled (compselect)) {
    compselect_unprelight_expander (compselect, GTK_WIDGET (expander));
    compselect_update_vertical_hierarchy (compselect);
  }
}

/*! \brief Callback function: Size of preview/attribute area changed.
 *
 * This function does two things:
 *
 * - When the content widgets are assigned a size for the first time,
 *   it calculates the difference between the stored and actual size
 *   and moves the corresponding vpaned's handle accordingly.  (This
 *   is necessary because there is no direct way to set the handle
 *   position relative to the far end of the paned.)
 *
 * - On subsequent size changes, it updates the stored size for that
 *   widget.
 */
static void
compselect_content_size_allocate (GtkWidget *widget,
                                  GdkRectangle *allocation,
                                  gpointer user_data)
{
  GschemCompselectDockable *compselect = GSCHEM_COMPSELECT_DOCKABLE (user_data);
  if (compselect_is_tiled (compselect))
    return;

  gboolean *size_allocated;
  const gchar *key;
  gint default_height;
  GtkPaned *vpaned;

  if (widget == compselect->preview_content) {
    size_allocated = &compselect->preview_size_allocated;
    key = "preview-height";
    default_height = 150;
    vpaned = GTK_PANED (compselect->preview_paned);
  } else if (widget == compselect->attribs_content) {
    size_allocated = &compselect->attribs_size_allocated;
    key = "attribs-height";
    default_height = 150;
    vpaned = GTK_PANED (compselect->attribs_paned);
  } else
    g_assert_not_reached ();

  if (*size_allocated == FALSE) {
    gint height = eda_config_get_int (eda_config_get_user_context (),
                                      GSCHEM_DOCKABLE (compselect)->group_name,
                                      key, NULL);
    if (height <= 0)
      height = default_height;

    gtk_paned_set_position (vpaned,
                            gtk_paned_get_position (vpaned)
                              - height
                              + allocation->height);
    *size_allocated = TRUE;
    return;
  }

  if (allocation->height > 0)
    eda_config_set_int (eda_config_get_user_context (),
                        GSCHEM_DOCKABLE (compselect)->group_name,
                        key, allocation->height);
}

static GtkWidget *
compselect_create_widget (GschemDockable *dockable)
{
  GschemCompselectDockable *compselect = GSCHEM_COMPSELECT_DOCKABLE (dockable);
  GtkWidget *inuseview, *libview, *notebook;
  GtkWidget *combobox, *preview;

  /* notebook for library and inuse views */
  inuseview = create_inuse_treeview (compselect);
  libview = create_lib_treeview (compselect);
  notebook = gtk_notebook_new ();
  compselect->viewtabs = GTK_NOTEBOOK (notebook);
  gtk_notebook_append_page (GTK_NOTEBOOK (notebook), inuseview,
                            gtk_label_new (_("In Use")));
  gtk_notebook_append_page (GTK_NOTEBOOK (notebook), libview,
                            gtk_label_new (_("Libraries")));

  /* vertical pane containing preview and attributes */
  compselect->preview_frame = gtk_frame_new (_("Preview"));
  compselect->attribs_frame = gtk_frame_new (_("Attributes"));
  compselect->vpaned = gtk_vpaned_new ();
  gtk_widget_set_size_request (compselect->vpaned, 25, -1);
  gtk_paned_pack1 (GTK_PANED (compselect->vpaned), compselect->preview_frame,
                   FALSE, FALSE);
  gtk_paned_pack2 (GTK_PANED (compselect->vpaned), compselect->attribs_frame,
                   FALSE, FALSE);

  /* horizontal pane containing selection and preview/attributes */
  compselect->hpaned = gtk_hpaned_new ();
  gtk_paned_pack1 (GTK_PANED (compselect->hpaned), notebook, TRUE, FALSE);
  gtk_paned_pack2 (GTK_PANED (compselect->hpaned), compselect->vpaned,
                   TRUE, FALSE);

  /* behavior combo box at the bottom */
  combobox = create_behaviors_combo_box ();
  compselect->combobox_behaviors = GTK_COMBO_BOX (combobox);
  g_signal_connect (combobox, "changed",
                    G_CALLBACK (compselect_callback_behavior_changed),
                    compselect);

  /* top-level vbox */
  compselect->vbox = gtk_vbox_new (FALSE, DIALOG_V_SPACING);
  gtk_box_pack_start (GTK_BOX (compselect->vbox), compselect->hpaned,
                      TRUE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (compselect->vbox), combobox, FALSE, FALSE, 0);
  gtk_widget_show_all (compselect->vbox);
  g_object_ref (compselect->vbox);


  /* preview area */
  preview = gschem_preview_new ();
  compselect->preview = GSCHEM_PREVIEW (preview);
  gtk_widget_set_size_request (preview, 160, 120);

  compselect->preview_content = gtk_alignment_new (.5, .5, 1., 1.);
  gtk_widget_set_size_request (compselect->preview_content, 0, 15);
  g_signal_connect (compselect->preview_content, "size-allocate",
                    G_CALLBACK (compselect_content_size_allocate), compselect);
  gtk_container_add (GTK_CONTAINER (compselect->preview_content), preview);
  gtk_widget_show_all (compselect->preview_content);
  g_object_ref (compselect->preview_content);

  compselect->preview_box = gtk_vbox_new (FALSE, 3);
  gtk_widget_show (compselect->preview_box);
  g_object_ref (compselect->preview_box);

  compselect->preview_paned = gtk_vpaned_new ();
  gtk_widget_show (compselect->preview_paned);
  g_object_ref (compselect->preview_paned);

  compselect->preview_expander = gtk_expander_new_with_mnemonic (_("Preview"));
  gtk_expander_set_spacing (GTK_EXPANDER (compselect->preview_expander), 3);
  g_signal_connect (compselect->preview_expander, "enter-notify-event",
                    G_CALLBACK (compselect_expander_enter_notify_event),
                    compselect);
  g_signal_connect (compselect->preview_expander, "notify::expanded",
                    G_CALLBACK (compselect_expander_notify_expanded),
                    compselect);
  gtk_widget_show (compselect->preview_expander);
  g_object_ref (compselect->preview_expander);

  compselect->preview_expander_event_window = NULL;
  compselect->preview_size_allocated = FALSE;


  /* attributes area */
  compselect->attribs_content = create_attributes_treeview (compselect);
  gtk_widget_set_size_request (compselect->attribs_content, -1, 20);
  g_signal_connect (compselect->attribs_content, "size-allocate",
                    G_CALLBACK (compselect_content_size_allocate), compselect);
  gtk_widget_show_all (compselect->attribs_content);
  g_object_ref (compselect->attribs_content);

  compselect->attribs_box = gtk_vbox_new (FALSE, 3);
  gtk_widget_show (compselect->attribs_box);
  g_object_ref (compselect->attribs_box);

  compselect->attribs_paned = gtk_vpaned_new ();
  gtk_widget_show (compselect->attribs_paned);
  g_object_ref (compselect->attribs_paned);

  compselect->attribs_expander =
    gtk_expander_new_with_mnemonic (_("Attributes"));
  gtk_expander_set_spacing (GTK_EXPANDER (compselect->attribs_expander), 3);
  g_signal_connect (compselect->attribs_expander, "enter-notify-event",
                    G_CALLBACK (compselect_expander_enter_notify_event),
                    compselect);
  g_signal_connect (compselect->attribs_expander, "notify::expanded",
                    G_CALLBACK (compselect_expander_notify_expanded),
                    compselect);
  gtk_widget_show (compselect->attribs_expander);
  g_object_ref (compselect->attribs_expander);

  compselect->attribs_expander_event_window = NULL;
  compselect->attribs_size_allocated = FALSE;


  /* top-level container widget (will contain one of the other widgets
     according to the selected layout) */
  compselect->top = gtk_alignment_new (.5, .5, 1., 1.);
  gtk_container_set_border_width (GTK_CONTAINER (compselect->top),
                                  DIALOG_BORDER_SPACING);
  g_signal_connect (compselect->top, "size-allocate",
                    G_CALLBACK (compselect_top_size_allocate), compselect);
  gtk_widget_show (compselect->top);
  return compselect->top;
}

static void
compselect_constructed (GObject *object)
{
  G_OBJECT_CLASS (compselect_parent_class)->constructed (object);

  s_clib_add_update_callback (library_updated, object);
}

static void
compselect_dispose (GObject *object)
{
  GschemCompselectDockable *compselect = GSCHEM_COMPSELECT_DOCKABLE (object);

  s_clib_remove_update_callback (object);

  g_clear_object (&compselect->vbox);

  g_clear_object (&compselect->preview_content);
  g_clear_object (&compselect->preview_box);
  g_clear_object (&compselect->preview_paned);
  g_clear_object (&compselect->preview_expander);

  g_clear_object (&compselect->attribs_content);
  g_clear_object (&compselect->attribs_box);
  g_clear_object (&compselect->attribs_paned);
  g_clear_object (&compselect->attribs_expander);

  G_OBJECT_CLASS (compselect_parent_class)->dispose (object);
}

static void
compselect_finalize (GObject *object)
{
  GschemCompselectDockable *compselect = GSCHEM_COMPSELECT_DOCKABLE (object);

  g_free (compselect->selected_filename);

  if (compselect->filter_timeout != 0) {
    g_source_remove (compselect->filter_timeout);
    compselect->filter_timeout = 0;
  }

  G_OBJECT_CLASS (compselect_parent_class)->finalize (object);
}
