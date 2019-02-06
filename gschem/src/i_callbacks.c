/* gEDA - GPL Electronic Design Automation
 * gschem - gEDA Schematic Capture
 * Copyright (C) 1998-2010 Ales Hvezda
 * Copyright (C) 1998-2011 gEDA Contributors (see ChangeLog for details)
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
#ifdef HAVE_STRING_H
#include <string.h>
#endif
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include "gschem.h"

/*! \brief */
#define DELIMITERS ", "

/* every i_callback functions have the same footprint */
#define DEFINE_I_CALLBACK(name)				\
	void i_callback_ ## name(gpointer data)

/* right now, all callbacks except for the ones on the File menu have
 * the middle button shortcut. Let me (Ales) know if we should also
 * shortcut the File button */

/*! \section file-menu File Menu Callback Functions */

DEFINE_I_CALLBACK(file_new)
{
  /*! \todo Perhaps this should be renamed to page_new... */
  GschemToplevel *w_current = GSCHEM_TOPLEVEL (data);
  PAGE *page;

  g_return_if_fail (w_current != NULL);

  /* create a new page */
  page = x_window_open_page (w_current, NULL);
  g_return_if_fail (page != NULL);

  x_window_set_current_page (w_current, page);
  s_log_message (_("New page created [%s]\n"), page->page_filename);
}

DEFINE_I_CALLBACK(file_new_window)
{
  GschemToplevel *w_current = NULL;
  PAGE *page = NULL;

  w_current = x_window_new (NULL);
  g_return_if_fail (w_current != NULL);

  page = x_window_open_page (w_current, NULL);
  g_return_if_fail (page != NULL);

  x_window_set_current_page (w_current, page);

  s_log_message (_("New Window created [%s]\n"), page->page_filename);
}

DEFINE_I_CALLBACK(file_open)
{
  /*! \todo Perhaps this should be renamed to page_open... */
  GschemToplevel *w_current = GSCHEM_TOPLEVEL (data);

  g_return_if_fail (w_current != NULL);

  x_fileselect_open (w_current);
}

DEFINE_I_CALLBACK(file_script)
{
  GschemToplevel *w_current = GSCHEM_TOPLEVEL (data);

  g_return_if_fail (w_current != NULL);
  setup_script_selector(w_current);
}

DEFINE_I_CALLBACK(file_save)
{
  GschemToplevel *w_current = GSCHEM_TOPLEVEL (data);
  PAGE *page;
  EdaConfig *cfg;
  gchar *untitled_name;

  g_return_if_fail (w_current != NULL);

  page = gschem_toplevel_get_toplevel (w_current)->page_current;

  if (page == NULL) {
    return;
  }

  /*! \bug This is a dreadful way of figuring out whether a page is
   *  newly-created or not. */
  cfg = eda_config_get_context_for_path (page->page_filename);
  untitled_name = eda_config_get_string (cfg, "gschem", "default-filename", NULL);
  if (strstr(page->page_filename, untitled_name)) {
    x_fileselect_save (w_current);
  } else {
    x_window_save_page (w_current, page, page->page_filename);
  }
  g_free (untitled_name);
}

DEFINE_I_CALLBACK(file_save_all)
{
  GschemToplevel *w_current = GSCHEM_TOPLEVEL (data);

  g_return_if_fail (w_current != NULL);

  if (s_page_save_all(gschem_toplevel_get_toplevel (w_current))) {
     i_set_state_msg(w_current, SELECT, _("Failed to Save All"));
  } else {
     i_set_state_msg(w_current, SELECT, _("Saved All"));
  }

  x_pagesel_update (w_current);
  i_update_menus(w_current);
}

DEFINE_I_CALLBACK(file_save_as)
{
  GschemToplevel *w_current = GSCHEM_TOPLEVEL (data);

  g_return_if_fail (w_current != NULL);
  x_fileselect_save (w_current);
}

DEFINE_I_CALLBACK(file_print)
{
  GschemToplevel *w_current = GSCHEM_TOPLEVEL (data);
  x_print (w_current);
}

DEFINE_I_CALLBACK(file_write_png)
{
  GschemToplevel *w_current = GSCHEM_TOPLEVEL (data);

  g_return_if_fail (w_current != NULL);

  x_image_setup(w_current);
}

/*! \brief Close a window. */

DEFINE_I_CALLBACK(file_close)
{
  GschemToplevel *w_current = GSCHEM_TOPLEVEL (data);

  g_return_if_fail (w_current != NULL);

  s_log_message(_("Closing Window\n"));
  x_window_close(w_current);
}

DEFINE_I_CALLBACK(file_quit)
{
  GschemToplevel *w_current = GSCHEM_TOPLEVEL (data);

  g_return_if_fail (w_current != NULL);
  x_window_close_all(w_current);
}

/*! \section edit-menu Edit Menu Callback Functions */

DEFINE_I_CALLBACK(edit_undo)
{
  GschemToplevel *w_current = GSCHEM_TOPLEVEL (data);

  /* If we're cancelling from a move action, re-wind the
   * page contents back to their state before we started.
   *
   * It "might" be nice to sub-undo rotates / zoom changes
   * made whilst moving components, but when the undo code
   * hits s_page_delete(), the place list objects are free'd.
   * Since they are also contained in the schematic page, a
   * crash occurs when the page objects are free'd.
   * */
  if (w_current->inside_action) {
    i_cancel (w_current);
  } else {
    GschemPageView *page_view = gschem_toplevel_get_current_page_view (w_current);
    g_return_if_fail (page_view != NULL);

    PAGE *page = gschem_page_view_get_page (page_view);

    if (page != NULL) {
      o_undo_callback (w_current, page, UNDO_ACTION);
    }
  }
}

DEFINE_I_CALLBACK(edit_redo)
{
  GschemToplevel *w_current = GSCHEM_TOPLEVEL (data);
  g_return_if_fail (w_current != NULL);

  GschemPageView *page_view = gschem_toplevel_get_current_page_view (w_current);
  g_return_if_fail (page_view != NULL);

  PAGE *page = gschem_page_view_get_page (page_view);

  if (page != NULL) {
    o_undo_callback (w_current, page, REDO_ACTION);
  }
}

DEFINE_I_CALLBACK(edit_select)
{
  /* Select also does not update the middle button shortcut. */
  GschemToplevel *w_current = GSCHEM_TOPLEVEL (data);
  o_redraw_cleanstates(w_current);

  /* this is probably the only place this should be */
  i_set_state(w_current, SELECT);
  i_action_stop (w_current);
}

/*! \brief Select all objects on page. */

DEFINE_I_CALLBACK (edit_select_all)
{
  GschemToplevel *w_current = GSCHEM_TOPLEVEL (data);
  o_redraw_cleanstates (w_current);

  o_select_visible_unlocked (w_current);

  i_set_state (w_current, SELECT);
  i_action_stop (w_current);
  i_update_menus (w_current);
}

/*! \brief Deselect all objects on page. */

DEFINE_I_CALLBACK (edit_deselect)
{
  GschemToplevel *w_current = GSCHEM_TOPLEVEL (data);
  o_redraw_cleanstates (w_current);

  o_select_unselect_all (w_current);

  i_set_state (w_current, SELECT);
  i_action_stop (w_current);
  i_update_menus (w_current);
}

DEFINE_I_CALLBACK(edit_copy)
{
  GschemToplevel *w_current = GSCHEM_TOPLEVEL (data);
  gint wx, wy;

  g_return_if_fail (w_current != NULL);

  i_update_middle_button(w_current, i_callback_edit_copy, _("Copy"));

  if (o_select_return_first_object(w_current)) {
    o_redraw_cleanstates(w_current);
    if (g_action_get_position (TRUE, &wx, &wy)) {
      o_copy_start(w_current, wx, wy);
    }
    i_set_state (w_current, COPYMODE);
  } else {
    i_set_state_msg(w_current, SELECT, _("Select objs first"));
  }
}

DEFINE_I_CALLBACK(edit_mcopy)
{
  GschemToplevel *w_current = GSCHEM_TOPLEVEL (data);
  gint wx, wy;

  g_return_if_fail (w_current != NULL);

  i_update_middle_button(w_current, i_callback_edit_mcopy, _("Multiple Copy"));

  if (o_select_return_first_object(w_current)) {
    o_redraw_cleanstates(w_current);
    if (g_action_get_position (TRUE, &wx, &wy)) {
      o_copy_start(w_current, wx, wy);
    }
    i_set_state (w_current, MCOPYMODE);
  } else {
    i_set_state_msg(w_current, SELECT, _("Select objs first"));
  }
}

DEFINE_I_CALLBACK(edit_move)
{
  GschemToplevel *w_current = GSCHEM_TOPLEVEL (data);
  gint wx, wy;

  g_return_if_fail (w_current != NULL);

  i_update_middle_button(w_current, i_callback_edit_move, _("Move"));

  if (o_select_return_first_object(w_current)) {
    o_redraw_cleanstates(w_current);
    if (g_action_get_position (TRUE, &wx, &wy)) {
      o_move_start(w_current, wx, wy);
    }
    i_set_state (w_current, MOVEMODE);
  } else {
    i_set_state_msg(w_current, SELECT, _("Select objs first"));
  }
}

DEFINE_I_CALLBACK(edit_delete)
{
  GschemToplevel *w_current = GSCHEM_TOPLEVEL (data);

  g_return_if_fail (w_current != NULL);

  i_update_middle_button(w_current, i_callback_edit_delete, _("Delete"));

  if (o_select_return_first_object(w_current)) {
    o_redraw_cleanstates(w_current);
    o_delete_selected(w_current);
    /* if you delete the objects you must go into select
     * mode after the delete */
    i_action_stop (w_current);
    i_set_state(w_current, SELECT);
    i_update_menus(w_current);
  }
}

DEFINE_I_CALLBACK(edit_edit)
{
  GschemToplevel *w_current = GSCHEM_TOPLEVEL (data);

  g_return_if_fail (w_current != NULL);

  i_update_middle_button(w_current, i_callback_edit_edit, _("Edit"));
  o_edit(w_current, geda_list_get_glist( gschem_toplevel_get_toplevel (w_current)->page_current->selection_list ) );
}

DEFINE_I_CALLBACK(edit_pin_type)
{
  GschemToplevel *w_current = GSCHEM_TOPLEVEL (data);

  g_return_if_fail (w_current != NULL);

  i_update_middle_button (w_current, i_callback_edit_pin_type, _("Edit pin type"));

  gschem_dockable_present (w_current->object_properties_dockable);
}

DEFINE_I_CALLBACK(edit_text)
{
  GschemToplevel *w_current = GSCHEM_TOPLEVEL (data);

  g_return_if_fail (w_current != NULL);

  i_update_middle_button(w_current, i_callback_edit_text, _("Edit Text"));

  gschem_dockable_present (w_current->text_properties_dockable);
}

DEFINE_I_CALLBACK(edit_slot)
{
  GschemToplevel *w_current = GSCHEM_TOPLEVEL (data);
  OBJECT *object;

  g_return_if_fail (w_current != NULL);

  object = o_select_return_first_object(w_current);

  i_update_middle_button(w_current, i_callback_edit_slot, _("Slot"));
  if (object) {
    o_slot_start(w_current, object);
  }
}

DEFINE_I_CALLBACK(edit_color)
{
  GschemToplevel *w_current = GSCHEM_TOPLEVEL (data);

  g_return_if_fail (w_current != NULL);

  i_update_middle_button(w_current, i_callback_edit_color, _("Color"));

  /* dialogs have been merged */
  gschem_dockable_present (w_current->object_properties_dockable);
}

/*! \brief Rotate all objects in the selection list by 90 degrees. */

DEFINE_I_CALLBACK(edit_rotate_90)
{
  gint wx, wy;
  GList *object_list;
  GschemToplevel *w_current = NULL;
  GschemPageView *view = NULL;
  PAGE* page = NULL;

  w_current = GSCHEM_TOPLEVEL (data);
  g_return_if_fail (w_current != NULL);

  view = (gschem_toplevel_get_current_page_view (w_current));
  g_return_if_fail (view != NULL);

  page = (gschem_page_view_get_page (view));

  if (page == NULL) {
    return;
  }

  if (w_current->inside_action && (page->place_list != NULL)) {
    o_place_rotate (w_current);
    return;
  }

  if (!g_action_get_position (TRUE, &wx, &wy)) {
    i_set_state(w_current, ROTATEMODE);
    i_update_middle_button(w_current, i_callback_edit_rotate_90, _("Rotate"));
    return;
  }

  o_redraw_cleanstates(w_current);

  object_list = geda_list_get_glist( gschem_toplevel_get_toplevel (w_current)->page_current->selection_list );

  if (object_list) {
    i_update_middle_button(w_current,
                           i_callback_edit_rotate_90, _("Rotate"));
    /* Allow o_rotate_world_update to redraw the objects */
    o_rotate_world_update(w_current, wx, wy, 90, object_list);
  }
}

DEFINE_I_CALLBACK(edit_mirror)
{
  gint wx, wy;
  GList *object_list;
  GschemToplevel *w_current = NULL;
  GschemPageView *view = NULL;
  PAGE* page = NULL;

  w_current = GSCHEM_TOPLEVEL (data);
  g_return_if_fail (w_current != NULL);

  view = (gschem_toplevel_get_current_page_view (w_current));
  g_return_if_fail (view != NULL);

  page = (gschem_page_view_get_page (view));

  if (page == NULL) {
    return;
  }

  if (w_current->inside_action && (page->place_list != NULL)) {
    o_place_mirror (w_current);
    return;
  }

  if (!g_action_get_position (TRUE, &wx, &wy)) {
    i_set_state(w_current, MIRRORMODE);
    i_update_middle_button(w_current, i_callback_edit_mirror, _("Mirror"));
    return;
  }

  o_redraw_cleanstates(w_current);

  object_list = geda_list_get_glist( gschem_toplevel_get_toplevel (w_current)->page_current->selection_list );

  if (object_list) {
    i_update_middle_button(w_current,
                           i_callback_edit_mirror, _("Mirror"));
    o_mirror_world_update(w_current, wx, wy, object_list);
  }
}

/*! \brief Lock all objects in selection list. */

DEFINE_I_CALLBACK(edit_lock)
{
  GschemToplevel *w_current = GSCHEM_TOPLEVEL (data);

  g_return_if_fail (w_current != NULL);

  i_update_middle_button(w_current, i_callback_edit_lock, _("Lock"));

  if (o_select_return_first_object(w_current)) {
    o_lock(w_current);
  }
}

/*! \brief Unlock all objects in selection list. */

DEFINE_I_CALLBACK(edit_unlock)
{
  GschemToplevel *w_current = GSCHEM_TOPLEVEL (data);

  g_return_if_fail (w_current != NULL);

  i_update_middle_button(w_current, i_callback_edit_unlock, _("Unlock"));
  if (o_select_return_first_object(w_current)) {
    o_unlock(w_current);
  }
}

DEFINE_I_CALLBACK(edit_translate)
{
  SNAP_STATE snap_mode;
  GschemToplevel *w_current = GSCHEM_TOPLEVEL (data);

  g_return_if_fail (w_current != NULL);

  i_update_middle_button(w_current,
                         i_callback_edit_translate, _("Translate"));

  snap_mode = gschem_options_get_snap_mode (w_current->options);

  if (snap_mode == SNAP_OFF) {
    s_log_message(_("WARNING: Do not translate with snap off!\n"));
    s_log_message(_("WARNING: Turning snap on and continuing "
                  "with translate.\n"));
    gschem_options_set_snap_mode (w_current->options, SNAP_GRID);
    i_show_state(w_current, NULL); /* update status on screen */
  }

  if (gschem_options_get_snap_size (w_current->options) != 100) {
    s_log_message(_("WARNING: Snap grid size is "
                  "not equal to 100!\n"));
    s_log_message(_("WARNING: If you are translating a symbol "
                  "to the origin, the snap grid size should be "
                  "set to 100\n"));
  }

  gtk_widget_show (w_current->translate_widget);
  gtk_widget_grab_focus (gschem_translate_widget_get_entry (GSCHEM_TRANSLATE_WIDGET (w_current->translate_widget)));
}

DEFINE_I_CALLBACK(edit_invoke_macro)
{
  GschemToplevel *w_current = GSCHEM_TOPLEVEL (data);

  g_return_if_fail (w_current != NULL);

  gtk_widget_show (w_current->macro_widget);
  gtk_widget_grab_focus (gschem_macro_widget_get_entry (GSCHEM_MACRO_WIDGET (w_current->macro_widget)));
}

/*! \brief Embed all objects in selection list. */

DEFINE_I_CALLBACK(edit_embed)
{
  GschemToplevel *w_current = GSCHEM_TOPLEVEL (data);
  OBJECT *o_current;

  g_return_if_fail (w_current != NULL);

  i_update_middle_button(w_current, i_callback_edit_embed, _("Embed"));
  /* anything selected ? */
  if (o_select_selected(w_current)) {
    /* yes, embed each selected component */
    GList *s_current =
      geda_list_get_glist( gschem_toplevel_get_toplevel (w_current)->page_current->selection_list );

    while (s_current != NULL) {
      o_current = (OBJECT *) s_current->data;
      g_assert (o_current != NULL);
      if ( (o_current->type == OBJ_COMPLEX) ||
	   (o_current->type == OBJ_PICTURE) ) {
        o_embed (gschem_toplevel_get_toplevel (w_current), o_current);
      }
      s_current = g_list_next(s_current);
    }
    o_undo_savestate_old(w_current, UNDO_ALL);
  } else {
    /* nothing selected, go back to select state */
    o_redraw_cleanstates(w_current);
    i_action_stop (w_current);
    i_set_state(w_current, SELECT);
  }
}

/*! \brief Unembed all objects in selection list. */

DEFINE_I_CALLBACK(edit_unembed)
{
  GschemToplevel *w_current = GSCHEM_TOPLEVEL (data);
  OBJECT *o_current;

  g_return_if_fail (w_current != NULL);

  i_update_middle_button(w_current, i_callback_edit_unembed, _("Unembed"));
  /* anything selected ? */
  if (o_select_selected(w_current)) {
    /* yes, unembed each selected component */
    GList *s_current =
      geda_list_get_glist( gschem_toplevel_get_toplevel (w_current)->page_current->selection_list );

    while (s_current != NULL) {
      o_current = (OBJECT *) s_current->data;
      g_assert (o_current != NULL);
      if ( (o_current->type == OBJ_COMPLEX) ||
           (o_current->type == OBJ_PICTURE) ) {
        o_unembed (gschem_toplevel_get_toplevel (w_current), o_current);
      }
      s_current = g_list_next(s_current);
    }
    o_undo_savestate_old(w_current, UNDO_ALL);
  } else {
    /* nothing selected, go back to select state */
    o_redraw_cleanstates(w_current);
    i_action_stop (w_current);
    i_set_state(w_current, SELECT);
  }
}

/*! \brief Update components. */

DEFINE_I_CALLBACK(edit_update)
{
  GschemToplevel *w_current = GSCHEM_TOPLEVEL (data);
  TOPLEVEL *toplevel = gschem_toplevel_get_toplevel (w_current);
  GList *selection;
  GList *selected_components = NULL;
  GList *iter;

  g_return_if_fail (w_current != NULL);

  i_update_middle_button(w_current, i_callback_edit_update, _("Update"));
  if (o_select_selected(w_current)) {

    /* Updating components modifies the selection. Therefore, create a
     * new list of only the OBJECTs we want to update from the current
     * selection, then iterate over that new list to perform the
     * update. */
    selection = geda_list_get_glist (toplevel->page_current->selection_list);
    for (iter = selection; iter != NULL; iter = g_list_next (iter)) {
      OBJECT *o_current = (OBJECT *) iter->data;
      if (o_current != NULL && o_current->type == OBJ_COMPLEX) {
        selected_components = g_list_prepend (selected_components, o_current);
      }
    }
    for (iter = selected_components; iter != NULL; iter = g_list_next (iter)) {
      OBJECT *o_current = (OBJECT *) iter->data;
      iter->data = o_update_component (w_current, o_current);
    }
    g_list_free (selected_components);

  } else {
    /* nothing selected, go back to select state */
    o_redraw_cleanstates(w_current);
    i_action_stop (w_current);
    i_set_state(w_current, SELECT);
  }
}

DEFINE_I_CALLBACK(edit_show_hidden)
{
  GschemToplevel *w_current = GSCHEM_TOPLEVEL (data);

  g_return_if_fail (w_current != NULL);

  i_update_middle_button(w_current,
                         i_callback_edit_show_hidden,
                         _("ShowHidden"));

  o_edit_show_hidden (w_current,
                      s_page_objects (gschem_toplevel_get_toplevel (w_current)->page_current));
}

DEFINE_I_CALLBACK(edit_find)
{
  GschemToplevel *w_current = GSCHEM_TOPLEVEL (data);

  g_return_if_fail (w_current != NULL);

  /* This is a new addition 3/15 to prevent this from executing
   * inside an action */
  if (w_current->inside_action)
    return;

  find_text_dialog(w_current);
}

DEFINE_I_CALLBACK(edit_find_patch)
{
  GschemToplevel *w_current = GSCHEM_TOPLEVEL (data);

  g_return_if_fail (w_current != NULL);

  /* This is a new addition 3/15 to prevent this from executing
   * inside an action */
  if (w_current->inside_action)
    return;

  find_patch_dialog(w_current);
}

DEFINE_I_CALLBACK(edit_hide_text)
{
  GschemToplevel *w_current = GSCHEM_TOPLEVEL (data);

  g_return_if_fail (w_current != NULL);

  /* This is a new addition 3/15 to prevent this from executing
   * inside an action */
  if (w_current->inside_action)
    return;

  hide_text_dialog(w_current);
}

DEFINE_I_CALLBACK(edit_show_text)
{
  GschemToplevel *w_current = GSCHEM_TOPLEVEL (data);

  g_return_if_fail (w_current != NULL);

  /* This is a new addition 3/15 to prevent this from executing
   * inside an action */
  if (w_current->inside_action)
    return;

  show_text_dialog(w_current);
}

DEFINE_I_CALLBACK(edit_autonumber_text)
{
  GschemToplevel *w_current = GSCHEM_TOPLEVEL (data);

  g_return_if_fail (w_current != NULL);

  /* This is a new addition 3/15 to prevent this from executing
   * inside an action */
  if (w_current->inside_action)
    return;

  autonumber_text_dialog(w_current);
}

DEFINE_I_CALLBACK(edit_linetype)
{
  GschemToplevel *w_current = GSCHEM_TOPLEVEL (data);

  g_return_if_fail (w_current != NULL);

  /* dialogs have been merged */
  gschem_dockable_present (w_current->object_properties_dockable);
}

DEFINE_I_CALLBACK(edit_filltype)
{
  GschemToplevel *w_current = GSCHEM_TOPLEVEL (data);

  g_return_if_fail (w_current != NULL);

  /* dialogs have been merged */
  gschem_dockable_present (w_current->object_properties_dockable);
}

/*! \section view-menu View Menu Callback Functions */

/*! \brief Toggle visibility of the menu bar. */

DEFINE_I_CALLBACK(view_menubar)
{
  GschemToplevel *w_current = GSCHEM_TOPLEVEL (data);
  g_return_if_fail (w_current != NULL);

  GtkWidget *w = w_current->menubar;
  if (w_current->handleboxes)
    w = gtk_widget_get_parent (w);

  gboolean show = !gtk_widget_get_visible (w);
  gtk_widget_set_visible (w, show);
}

/*! \brief Toggle visibility of the toolbar. */

DEFINE_I_CALLBACK(view_toolbar)
{
  GschemToplevel *w_current = GSCHEM_TOPLEVEL (data);
  g_return_if_fail (w_current != NULL);

  GtkWidget *w = w_current->toolbar;
  if (w == NULL)
    return;
  if (w_current->handleboxes)
    w = gtk_widget_get_parent (w);

  gboolean show = !gtk_widget_get_visible (w);
  gtk_widget_set_visible (w, show);
}

DEFINE_I_CALLBACK(view_redraw)
{
  /* repeat middle shortcut doesn't make sense on redraw,
     just hit right button */
  GschemToplevel *w_current = GSCHEM_TOPLEVEL (data);
  g_return_if_fail (w_current != NULL);

  gschem_page_view_invalidate_all (gschem_toplevel_get_current_page_view (w_current));
}

DEFINE_I_CALLBACK(view_zoom_full)
{
  /* repeat middle shortcut would get into the way of what user is try to do */
  GschemToplevel *w_current = GSCHEM_TOPLEVEL (data);
  g_return_if_fail (w_current != NULL);

  GschemPageView *page_view = gschem_toplevel_get_current_page_view (w_current);
  g_return_if_fail (page_view != NULL);

  /* scroll bar stuff */
  a_zoom(w_current, page_view, ZOOM_FULL, DONTCARE);

  if (w_current->undo_panzoom) {
    o_undo_savestate_old(w_current, UNDO_VIEWPORT_ONLY);
  }
}

DEFINE_I_CALLBACK(view_zoom_extents)
{
  /* repeat middle shortcut would get into the way of what user is try to do */
  GschemToplevel *w_current = GSCHEM_TOPLEVEL (data);
  g_return_if_fail (w_current != NULL);

  GschemPageView *page_view = gschem_toplevel_get_current_page_view (w_current);
  g_return_if_fail (page_view != NULL);

  gschem_page_view_zoom_extents (page_view, NULL);

  if (w_current->undo_panzoom) {
    o_undo_savestate_old(w_current, UNDO_VIEWPORT_ONLY);
  }
}

DEFINE_I_CALLBACK(view_zoom_box)
{
  /* repeat middle shortcut would get into the way of what user is try to do */
  GschemToplevel *w_current = GSCHEM_TOPLEVEL (data);
  gint wx, wy;

  g_return_if_fail (w_current != NULL);

  o_redraw_cleanstates(w_current);

  i_set_state(w_current, ZOOMBOX);

  if (g_action_get_position (FALSE, &wx, &wy)) {
    a_zoom_box_start(w_current, wx, wy);
  }
}

DEFINE_I_CALLBACK(view_zoom_in)
{
  /* repeat middle shortcut would get into the way of what user is try to do */
  GschemToplevel *w_current = GSCHEM_TOPLEVEL (data);
  g_return_if_fail (w_current != NULL);

  GschemPageView *page_view = gschem_toplevel_get_current_page_view (w_current);
  g_return_if_fail (page_view != NULL);

  a_zoom (w_current,
          page_view,
          ZOOM_IN,
          g_action_get_position (FALSE, NULL, NULL) ? HOTKEY : MENU);

  if (w_current->undo_panzoom) {
    o_undo_savestate_old(w_current, UNDO_VIEWPORT_ONLY);
  }
}

DEFINE_I_CALLBACK(view_zoom_out)
{
  /* repeat middle shortcut would get into the way of what user is try to do */
  GschemToplevel *w_current = GSCHEM_TOPLEVEL (data);
  g_return_if_fail (w_current != NULL);

  GschemPageView *page_view = gschem_toplevel_get_current_page_view (w_current);
  g_return_if_fail (page_view != NULL);

  a_zoom(w_current,
         page_view,
         ZOOM_OUT,
         g_action_get_position (FALSE, NULL, NULL) ? HOTKEY : MENU);

  if (w_current->undo_panzoom) {
    o_undo_savestate_old(w_current, UNDO_VIEWPORT_ONLY);
  }
}

DEFINE_I_CALLBACK(view_pan)
{
  gint wx, wy;

  GschemToplevel *w_current = GSCHEM_TOPLEVEL (data);
  g_return_if_fail (w_current != NULL);

  GschemPageView *page_view = gschem_toplevel_get_current_page_view (w_current);
  g_return_if_fail (page_view != NULL);

  i_update_middle_button(w_current, i_callback_view_pan, _("Pan"));

  if (!g_action_get_position (FALSE, &wx, &wy)) {
    o_redraw_cleanstates (w_current);
    i_action_stop (w_current);
    i_set_state (w_current, PAN);
  } else {
    gschem_page_view_pan (page_view, wx, wy);
    if (w_current->undo_panzoom) {
      o_undo_savestate_old(w_current, UNDO_VIEWPORT_ONLY);
    }
  }
}

/*! \brief Move the viewport to the left.
 *
 * The distance can be set with "keyboardpan-gain" scheme callback. */

DEFINE_I_CALLBACK(view_pan_left)
{
  GschemToplevel *w_current = GSCHEM_TOPLEVEL (data);
  g_return_if_fail (w_current != NULL);

  GschemPageView *page_view = gschem_toplevel_get_current_page_view (w_current);
  g_return_if_fail (page_view != NULL);

  gschem_page_view_pan_mouse (page_view, w_current->keyboardpan_gain, 0);
}

/*! \brief Move the viewport to the right.
 *
 * The distance can be set with "keyboardpan-gain" scheme callback. */

DEFINE_I_CALLBACK(view_pan_right)
{
  GschemToplevel *w_current = GSCHEM_TOPLEVEL (data);
  g_return_if_fail (w_current != NULL);

  GschemPageView *page_view = gschem_toplevel_get_current_page_view (w_current);
  g_return_if_fail (page_view != NULL);

  /* yes, that's a negative sign there */
  gschem_page_view_pan_mouse (page_view, -w_current->keyboardpan_gain, 0);
}

/*! \brief Move the viewport up.
 *
 * The distance can be set with "keyboardpan-gain" scheme callback. */

DEFINE_I_CALLBACK(view_pan_up)
{
  GschemToplevel *w_current = GSCHEM_TOPLEVEL (data);
  g_return_if_fail (w_current != NULL);

  GschemPageView *page_view = gschem_toplevel_get_current_page_view (w_current);
  g_return_if_fail (page_view != NULL);

  gschem_page_view_pan_mouse (page_view, 0, w_current->keyboardpan_gain);
}

/*! \brief Move the viewport down.
 *
 * The distance can be set with "keyboardpan-gain" scheme callback. */

DEFINE_I_CALLBACK(view_pan_down)
{
  GschemToplevel *w_current = GSCHEM_TOPLEVEL (data);
  g_return_if_fail (w_current != NULL);

  GschemPageView *page_view = gschem_toplevel_get_current_page_view (w_current);
  g_return_if_fail (page_view != NULL);

  /* yes, that's a negative sign there */
  gschem_page_view_pan_mouse (page_view, 0, -w_current->keyboardpan_gain);
}

DEFINE_I_CALLBACK (view_dark_colors)
{
  GschemToplevel *w_current = GSCHEM_TOPLEVEL (data);

  x_color_free ();
  /* Change the scheme here */
  g_scm_c_eval_string_protected ("(load (build-path geda-rc-path \"gschem-colormap-darkbg\"))");
  x_color_allocate ();

  gschem_page_view_invalidate_all (gschem_toplevel_get_current_page_view (w_current));
}

DEFINE_I_CALLBACK (view_light_colors)
{
  GschemToplevel *w_current = GSCHEM_TOPLEVEL (data);

  x_color_free ();
  /* Change the scheme here */
  g_scm_c_eval_string_protected ("(load (build-path geda-rc-path \"gschem-colormap-lightbg\"))");
  x_color_allocate ();

  gschem_page_view_invalidate_all (gschem_toplevel_get_current_page_view (w_current));
}

DEFINE_I_CALLBACK (view_bw_colors)
{
  GschemToplevel *w_current = GSCHEM_TOPLEVEL (data);

  x_color_free ();
  /* Change the scheme here */
  g_scm_c_eval_string_protected ("(load (build-path geda-rc-path \"gschem-colormap-bw\"))");
  x_color_allocate ();

  gschem_page_view_invalidate_all (gschem_toplevel_get_current_page_view (w_current));
}

/*! \section page-menu Page Menu Callback Functions */

DEFINE_I_CALLBACK(page_manager)
{
  GschemToplevel *w_current = GSCHEM_TOPLEVEL (data);

  g_return_if_fail (w_current != NULL);

  x_pagesel_open (w_current);
}

DEFINE_I_CALLBACK(page_next)
{
  GschemToplevel *w_current = GSCHEM_TOPLEVEL (data);
  TOPLEVEL *toplevel = gschem_toplevel_get_toplevel (w_current);
  PAGE *p_current = toplevel->page_current;
  PAGE *p_new;
  GList *iter;

  g_return_if_fail (w_current != NULL);

  iter = g_list_find( geda_list_get_glist( toplevel->pages ), p_current );
  iter = g_list_next( iter );

  if (iter == NULL) {
    return;
  }

  if (w_current->enforce_hierarchy) {
    p_new = s_hierarchy_find_next_page(toplevel->pages, p_current);
  } else {
    p_new = (PAGE *)iter->data;
  }

  if (p_new == NULL || p_new == p_current) {
    return;
  }

  x_window_set_current_page (w_current, p_new);
}

DEFINE_I_CALLBACK(page_prev)
{
  GschemToplevel *w_current = GSCHEM_TOPLEVEL (data);
  TOPLEVEL *toplevel = gschem_toplevel_get_toplevel (w_current);
  PAGE *p_current = toplevel->page_current;
  PAGE *p_new;
  GList *iter;

  g_return_if_fail (w_current != NULL);

  iter = g_list_find( geda_list_get_glist( toplevel->pages ), p_current );
  iter = g_list_previous( iter );

  if ( iter == NULL  )
    return;

  if (w_current->enforce_hierarchy) {
    p_new = s_hierarchy_find_prev_page(toplevel->pages, p_current);
  } else {
    p_new = (PAGE *)iter->data;
  }

  if (p_new == NULL || p_new == p_current) {
    return;
  }

  x_window_set_current_page (w_current, p_new);
}

DEFINE_I_CALLBACK(page_close)
{
  GschemToplevel *w_current = GSCHEM_TOPLEVEL (data);
  g_return_if_fail (w_current != NULL);

  PAGE *page = gschem_toplevel_get_toplevel (w_current)->page_current;

  if (page == NULL) {
    return;
  }

  if (page->CHANGED
      && !x_dialog_close_changed_page (w_current, page)) {
    return;
  }

  x_window_close_page (w_current, page);
}

DEFINE_I_CALLBACK(page_revert)
{
  /*! \bug may have memory leak? */
  GschemToplevel *w_current = GSCHEM_TOPLEVEL (data);
  PAGE *page_current = NULL;
  PAGE *page = NULL;
  gchar *filename;
  int page_control;
  int up;
  int response;
  GtkWidget* dialog;

  g_return_if_fail (w_current != NULL);

  page_current = gschem_toplevel_get_toplevel (w_current)->page_current;

  dialog = gtk_message_dialog_new ((GtkWindow*) w_current->main_window,
                                   GTK_DIALOG_DESTROY_WITH_PARENT,
                                   GTK_MESSAGE_QUESTION,
                                   GTK_BUTTONS_YES_NO,
                                   _("Really revert page?"));

  /* Set the alternative button order (ok, cancel, help) for other systems */
  gtk_dialog_set_alternative_button_order(GTK_DIALOG(dialog),
					  GTK_RESPONSE_YES,
					  GTK_RESPONSE_NO,
					  -1);

  response = gtk_dialog_run (GTK_DIALOG (dialog));
  gtk_widget_destroy (dialog);

  if (response != GTK_RESPONSE_YES )
    return;

  /* save this for later */
  filename = g_strdup (page_current->page_filename);
  page_control = page_current->page_control;
  up = page_current->up;

  /* delete the page, then re-open the file as a new page */
  x_window_close_page (w_current, page_current);

  /* Force symbols to be re-loaded from disk */
  s_clib_refresh();

  page = x_window_open_page (w_current, filename);
  g_return_if_fail (page != NULL);

  /* make sure we maintain the hierarchy info */
  page->page_control = page_control;
  page->up = up;

  x_window_set_current_page (w_current, page);
}

DEFINE_I_CALLBACK(page_print)
{
  GschemToplevel *w_current = GSCHEM_TOPLEVEL (data);

  s_page_print_all(gschem_toplevel_get_toplevel (w_current));
}

/*! \section clipboard-menu Clipboard Menu Callback Functions */

/*! \brief Copy selection to clipboard, via buffer 0. */

DEFINE_I_CALLBACK(clipboard_copy)
{
  GschemToplevel *w_current = GSCHEM_TOPLEVEL (data);

  g_return_if_fail (w_current != NULL);
  if (!o_select_selected (w_current)) return;

  i_update_middle_button (w_current, i_callback_clipboard_copy,
                          _("Copy to clipboard"));

  o_buffer_copy (w_current, CLIPBOARD_BUFFER);
}

/*! \brief Cut selection to clipboard, via buffer 0. */

DEFINE_I_CALLBACK(clipboard_cut)
{
  GschemToplevel *w_current = GSCHEM_TOPLEVEL (data);

  g_return_if_fail (w_current != NULL);
  if (!o_select_selected (w_current)) return;

  i_update_middle_button (w_current, i_callback_clipboard_cut,
                          _("Cut to clipboard"));

  o_redraw_cleanstates(w_current);
  o_buffer_cut (w_current, CLIPBOARD_BUFFER);
}

/*! \brief Start pasting clipboard contents, via buffer 0. */

DEFINE_I_CALLBACK(clipboard_paste)
{
  GschemToplevel *w_current = GSCHEM_TOPLEVEL (data);
  int empty;

  /* Choose a default position to start pasting. This is required to
   * make pasting when the cursor is outside the screen or pasting via
   * menu work as expected. */
  gint wx = 0, wy = 0;

  g_return_if_fail (w_current != NULL);

  i_update_middle_button (w_current, i_callback_clipboard_paste, _("Paste from clipboard"));

  g_action_get_position (TRUE, &wx, &wy);

  o_redraw_cleanstates(w_current);
  empty = o_buffer_paste_start (w_current, wx, wy, CLIPBOARD_BUFFER);

  if (empty) {
    i_set_state_msg (w_current, SELECT, _("Empty clipboard"));
  }
}

#define DEFINE_I_CALLBACK_BUF(op, n) \
  DEFINE_I_CALLBACK(buffer_ ## op ## n) { \
    i_buffer_ ## op (data, n, i_callback_buffer_ ## op ## n); \
  }

DEFINE_I_CALLBACK_BUF(copy,1)
DEFINE_I_CALLBACK_BUF(copy,2)
DEFINE_I_CALLBACK_BUF(copy,3)
DEFINE_I_CALLBACK_BUF(copy,4)
DEFINE_I_CALLBACK_BUF(copy,5)

DEFINE_I_CALLBACK_BUF(cut,1)
DEFINE_I_CALLBACK_BUF(cut,2)
DEFINE_I_CALLBACK_BUF(cut,3)
DEFINE_I_CALLBACK_BUF(cut,4)
DEFINE_I_CALLBACK_BUF(cut,5)

DEFINE_I_CALLBACK_BUF(paste,1)
DEFINE_I_CALLBACK_BUF(paste,2)
DEFINE_I_CALLBACK_BUF(paste,3)
DEFINE_I_CALLBACK_BUF(paste,4)
DEFINE_I_CALLBACK_BUF(paste,5)

#undef DEFINE_I_CALLBACK_BUF

/*! \section add-menu Add Menu Callback Functions */

DEFINE_I_CALLBACK(add_component)
{
  GschemToplevel *w_current = GSCHEM_TOPLEVEL (data);

  g_return_if_fail (w_current != NULL);

  o_redraw_cleanstates (w_current);

  i_set_state(w_current, COMPMODE);
  gschem_dockable_present (w_current->compselect_dockable);

  i_update_middle_button(w_current,
                         i_callback_add_component, _("Component"));

  i_set_state(w_current, SELECT);
}

DEFINE_I_CALLBACK(add_attribute)
{
  GschemToplevel *w_current = GSCHEM_TOPLEVEL (data);

  g_return_if_fail (w_current != NULL);

  attrib_edit_dialog(w_current, NULL,
                     g_action_get_position (TRUE, NULL, NULL) ? FROM_HOTKEY : FROM_MENU);
  i_update_middle_button(w_current, i_callback_add_attribute,
                         _("Attribute"));

  i_set_state(w_current, SELECT);
}

DEFINE_I_CALLBACK(add_net)
{
  GschemToplevel *w_current = GSCHEM_TOPLEVEL (data);
  gint wx, wy;

  g_return_if_fail (w_current != NULL);

  o_redraw_cleanstates(w_current);

  i_set_state(w_current, NETMODE);
  i_update_middle_button(w_current, i_callback_add_net, _("Net"));

  if (g_action_get_position (TRUE, &wx, &wy)) {
    o_net_reset(w_current);
    o_net_start(w_current, wx, wy);
  }
}

DEFINE_I_CALLBACK(add_bus)
{
  GschemToplevel *w_current = GSCHEM_TOPLEVEL (data);
  gint wx, wy;

  g_return_if_fail (w_current != NULL);

  o_redraw_cleanstates(w_current);
  o_invalidate_rubber (w_current);

  i_set_state(w_current, BUSMODE);
  i_update_middle_button(w_current, i_callback_add_bus, _("Bus"));

  if (g_action_get_position (TRUE, &wx, &wy)) {
    o_bus_start(w_current, wx, wy);
  }
}

DEFINE_I_CALLBACK(add_text)
{
  GschemToplevel *w_current = GSCHEM_TOPLEVEL (data);

  g_return_if_fail (w_current != NULL);

  o_redraw_cleanstates(w_current);
  o_invalidate_rubber (w_current);

  i_action_stop (w_current);
  i_set_state(w_current, SELECT);
  i_update_middle_button(w_current, i_callback_add_text, _("Text"));

  text_input_dialog(w_current);
}

DEFINE_I_CALLBACK(add_line)
{
  GschemToplevel *w_current = GSCHEM_TOPLEVEL (data);
  gint wx, wy;

  g_return_if_fail (w_current != NULL);

  o_redraw_cleanstates(w_current);
  o_invalidate_rubber (w_current);

  i_set_state(w_current, LINEMODE);
  i_update_middle_button(w_current, i_callback_add_line, _("Line"));

  if (g_action_get_position (TRUE, &wx, &wy)) {
    o_line_start(w_current, wx, wy);
  }
}

DEFINE_I_CALLBACK(add_path)
{
  GschemToplevel *w_current = GSCHEM_TOPLEVEL (data);

  g_assert (w_current != NULL);

  o_redraw_cleanstates (w_current);
  o_invalidate_rubber (w_current);

  i_set_state (w_current, PATHMODE);
  i_update_middle_button (w_current, i_callback_add_path, _("Path"));

  /* Don't start path here since setting of its first point and
   * control point requires the left button click and release */
}

DEFINE_I_CALLBACK(add_box)
{
  GschemToplevel *w_current = GSCHEM_TOPLEVEL (data);
  gint wx, wy;

  g_return_if_fail (w_current != NULL);

  o_redraw_cleanstates(w_current);
  o_invalidate_rubber (w_current);

  i_set_state(w_current, BOXMODE);
  i_update_middle_button(w_current, i_callback_add_box, _("Box"));

  if (g_action_get_position (TRUE, &wx, &wy)) {
    o_box_start(w_current, wx, wy);
  }
}

DEFINE_I_CALLBACK(add_picture)
{
  GschemToplevel *w_current = GSCHEM_TOPLEVEL (data);

  g_return_if_fail (w_current != NULL);

  o_redraw_cleanstates(w_current);
  o_invalidate_rubber (w_current);

  i_set_state(w_current, SELECT);

  picture_selection_dialog(w_current);
}

DEFINE_I_CALLBACK(add_circle)
{
  GschemToplevel *w_current = GSCHEM_TOPLEVEL (data);
  gint wx, wy;

  g_return_if_fail (w_current != NULL);

  o_redraw_cleanstates(w_current);
  o_invalidate_rubber (w_current);

  i_set_state(w_current, CIRCLEMODE);
  i_update_middle_button(w_current, i_callback_add_circle, _("Circle"));

  if (g_action_get_position (TRUE, &wx, &wy)) {
    o_circle_start(w_current, wx, wy);
  }
}

DEFINE_I_CALLBACK(add_arc)
{
  GschemToplevel *w_current = GSCHEM_TOPLEVEL (data);
  gint wx, wy;

  g_return_if_fail (w_current != NULL);

  o_redraw_cleanstates(w_current);
  o_invalidate_rubber (w_current);

  i_set_state(w_current, ARCMODE);
  i_update_middle_button(w_current, i_callback_add_arc, _("Arc"));

  if (g_action_get_position (TRUE, &wx, &wy)) {
    o_arc_start(w_current, wx, wy);
  }
}

DEFINE_I_CALLBACK(add_pin)
{
  GschemToplevel *w_current = GSCHEM_TOPLEVEL (data);
  gint wx, wy;

  g_return_if_fail (w_current != NULL);

  o_redraw_cleanstates(w_current);
  o_invalidate_rubber (w_current);

  i_set_state (w_current, PINMODE);
  i_update_middle_button(w_current, i_callback_add_pin, _("Pin"));

  if (g_action_get_position (TRUE, &wx, &wy)) {
    o_pin_start(w_current, wx, wy);
  }
}

/*! \section hierarchy-menu Hierarchy Menu Callback Functions */

DEFINE_I_CALLBACK(hierarchy_down_schematic)
{
  GschemToplevel *w_current = GSCHEM_TOPLEVEL (data);
  char *attrib=NULL;
  char *current_filename=NULL;
  int count=0;
  OBJECT *object=NULL;
  PAGE *save_first_page=NULL;
  PAGE *parent=NULL;
  PAGE *child = NULL;
  int loaded_flag=FALSE;
  int page_control = 0;
  int pcount = 0;
  int looking_inside=FALSE;

  g_return_if_fail (w_current != NULL);

  object = o_select_return_first_object(w_current);

  /* only allow going into symbols */
  if (object == NULL || object->type != OBJ_COMPLEX)
    return;

  parent = gschem_toplevel_get_toplevel (w_current)->page_current;
  attrib = o_attrib_search_attached_attribs_by_name (object, "source", count);

  /* if above is null, then look inside symbol */
  if (attrib == NULL) {
    attrib =
      o_attrib_search_inherited_attribs_by_name (object, "source", count);
    looking_inside = TRUE;
#if DEBUG
    printf("going to look inside now\n");
#endif
  }

  while (attrib) {

    /* look for source=filename,filename, ... */
    pcount = 0;
    current_filename = u_basic_breakup_string(attrib, ',', pcount);

    /* loop over all filenames */
    while(current_filename != NULL) {
      GError *err = NULL;
      s_log_message(_("Searching for source [%s]\n"), current_filename);
      child = s_hierarchy_down_schematic_single(gschem_toplevel_get_toplevel (w_current),
                                                current_filename,
                                                parent,
                                                page_control,
                                                HIERARCHY_NORMAL_LOAD,
                                                &err);
      gschem_toplevel_page_changed (w_current);

      /* s_hierarchy_down_schematic_single() will not zoom the loaded page */
      if (child != NULL) {
        s_page_goto (gschem_toplevel_get_toplevel (w_current), child);
        gschem_toplevel_page_changed (w_current);
        gschem_page_view_zoom_extents (gschem_toplevel_get_current_page_view (w_current),
                                       NULL);
        o_undo_savestate_old(w_current, UNDO_ALL);
        s_page_goto (gschem_toplevel_get_toplevel (w_current), parent);
        gschem_toplevel_page_changed (w_current);
      }

      /* save the first page */
      if ( !loaded_flag && (child != NULL)) {
        save_first_page = child;
      }

      /* now do some error fixing */
      if (child == NULL) {
        const char *msg = (err != NULL) ? err->message : "Unknown error.";
        char *secondary =
          g_strdup_printf (_("Failed to descend hierarchy into '%s': %s\n\n"
                             "The gschem log may contain more information."),
                           current_filename, msg);

        s_log_message(_("Failed to descend into '%s': %s\n"),
                      current_filename, msg);

        GtkWidget *dialog =
          gtk_message_dialog_new (GTK_WINDOW (w_current->main_window),
                                  GTK_DIALOG_MODAL, GTK_MESSAGE_ERROR,
                                  GTK_BUTTONS_OK,
                                  _("Failed to descend hierarchy."));
        g_object_set (G_OBJECT (dialog), "secondary-text", secondary, NULL);
        gtk_dialog_run (GTK_DIALOG (dialog));
        gtk_widget_destroy (dialog);
        g_free (secondary);
        g_error_free (err);

      } else {
        /* this only signifies that we tried */
        loaded_flag = TRUE;
        page_control = child->page_control;
      }

      g_free(current_filename);
      pcount++;
      current_filename = u_basic_breakup_string(attrib, ',', pcount);
    }

    g_free(attrib);
    g_free(current_filename);

    count++;

    /* continue looking outside first */
    if (!looking_inside) {
      attrib =
        o_attrib_search_attached_attribs_by_name (object, "source", count);
    }

    /* okay we were looking outside and didn't find anything,
     * so now we need to look inside the symbol */
    if (!looking_inside && attrib == NULL && !loaded_flag ) {
      looking_inside = TRUE;
#if DEBUG
      printf("switching to go to look inside\n");
#endif
    }

    if (looking_inside) {
#if DEBUG
      printf("looking inside\n");
#endif
      attrib =
        o_attrib_search_inherited_attribs_by_name (object, "source", count);
    }
  }

  if (loaded_flag && (save_first_page != NULL)) {
    x_window_set_current_page (w_current, save_first_page);
  }
}

DEFINE_I_CALLBACK(hierarchy_down_symbol)
{
  /*! \bug may cause problems with non-directory symbols */
  GschemToplevel *w_current = GSCHEM_TOPLEVEL (data);
  OBJECT *object;
  const CLibSymbol *sym;

  g_return_if_fail (w_current != NULL);

  object = o_select_return_first_object(w_current);
  if (object != NULL) {
    /* only allow going into symbols */
    if (object->type == OBJ_COMPLEX) {
      s_log_message(_("Searching for symbol [%s]\n"),
		    object->complex_basename);
      sym = s_clib_get_symbol_by_name (object->complex_basename);
      if (sym == NULL)
	return;
      if (s_clib_symbol_get_filename(sym) == NULL) {
	s_log_message(_("Symbol is not a real file."
			" Symbol cannot be loaded.\n"));
	return;
      }
      s_hierarchy_down_symbol(gschem_toplevel_get_toplevel (w_current), sym,
			      gschem_toplevel_get_toplevel (w_current)->page_current);
      gschem_toplevel_page_changed (w_current);

      x_window_set_current_page(w_current, gschem_toplevel_get_toplevel (w_current)->page_current);
      /* s_hierarchy_down_symbol() will not zoom the loaded page */
      gschem_page_view_zoom_extents (gschem_toplevel_get_current_page_view (w_current),
                                     NULL);
      o_undo_savestate_old(w_current, UNDO_ALL);
    }
  }
}

/*! \brief Return to the parent page in the hierarchy of schematics. */

DEFINE_I_CALLBACK(hierarchy_up)
{
  GschemToplevel *w_current = GSCHEM_TOPLEVEL (data);
  PAGE *page = NULL;
  PAGE *up_page = NULL;

  g_return_if_fail (w_current != NULL);

  page = gschem_toplevel_get_toplevel (w_current)->page_current;

  if (page == NULL) {
    return;
  }

  up_page = s_hierarchy_find_up_page (gschem_toplevel_get_toplevel (w_current)->pages, page);
  if (up_page == NULL) {
    s_log_message(_("Cannot find any schematics above the current one!\n"));
  } else {
    if (page->CHANGED && !x_dialog_close_changed_page (w_current, page))
      return;
    x_window_close_page (w_current, page);
    x_window_set_current_page(w_current, up_page);
  }
}

/*! \section attributes-menu Attributes Menu Callback Functions */

DEFINE_I_CALLBACK(attributes_attach)
{
  GschemToplevel *w_current = GSCHEM_TOPLEVEL (data);
  OBJECT *first_object;
  GList *s_current;
  GList *attached_objects = NULL;

  g_return_if_fail (w_current != NULL);

  /* This is a new addition 3/15 to prevent this from executing
   * inside an action */
  if (w_current->inside_action) {
    return;
  }

  /* do we want to update the shortcut outside of the ifs? */
  /* probably, if this fails the user may want to try again */
  i_update_middle_button(w_current, i_callback_attributes_attach,
                         _("Attach"));

  /* skip over head */
  s_current = geda_list_get_glist( gschem_toplevel_get_toplevel (w_current)->page_current->selection_list );
  if (!s_current) {
    return;
  }

  first_object = (OBJECT *) s_current->data;
  if (!first_object) {
    return;
  }

  /* skip over first object */
  s_current = g_list_next(s_current);
  while (s_current != NULL) {
    OBJECT *object = s_current->data;
    if (object != NULL) {
      o_attrib_attach (gschem_toplevel_get_toplevel (w_current), object, first_object, TRUE);
      attached_objects = g_list_prepend (attached_objects, object);
      gschem_toplevel_get_toplevel (w_current)->page_current->CHANGED=1;
    }
    s_current = g_list_next(s_current);
  }

  if (attached_objects != NULL) {
    g_run_hook_object_list (w_current, "%attach-attribs-hook",
                            attached_objects);
    g_list_free (attached_objects);
  }

  o_undo_savestate_old(w_current, UNDO_ALL);
}

DEFINE_I_CALLBACK(attributes_detach)
{
  GschemToplevel *w_current = GSCHEM_TOPLEVEL (data);
  GList *s_current;
  OBJECT *o_current;
  GList *detached_attribs = NULL;

  g_return_if_fail (w_current != NULL);

  /* This is a new addition 3/15 to prevent this from executing
   * inside an action */
  if (w_current->inside_action) {
    return;
  }

  /* same note as above on i_update_middle_button */
  i_update_middle_button(w_current, i_callback_attributes_detach,
                         _("Detach"));

  s_current = geda_list_get_glist( gschem_toplevel_get_toplevel (w_current)->page_current->selection_list );
  while (s_current != NULL) {
    o_current = (OBJECT *) s_current->data;
    if (o_current) {
      if (o_current->attribs) {
        detached_attribs = g_list_concat (g_list_copy (o_current->attribs),
                                          detached_attribs);
        o_attrib_detach_all (gschem_toplevel_get_toplevel (w_current), o_current);
        gschem_toplevel_get_toplevel (w_current)->page_current->CHANGED=1;
      }
    }
    s_current = g_list_next(s_current);
  }

  if (detached_attribs != NULL) {
    g_run_hook_object_list (w_current, "%detach-attribs-hook",
                            detached_attribs);
    g_list_free (detached_attribs);
  }

  o_undo_savestate_old(w_current, UNDO_ALL);
}

DEFINE_I_CALLBACK(attributes_show_name)
{
  GschemToplevel *w_current = GSCHEM_TOPLEVEL (data);
  TOPLEVEL *toplevel = gschem_toplevel_get_toplevel (w_current);

  g_return_if_fail (w_current != NULL);

  /* This is a new addition 3/15 to prevent this from executing
   * inside an action */
  if (w_current->inside_action) {
    return;
  }

  i_update_middle_button(w_current, i_callback_attributes_show_name,
                         _("ShowN"));

  if (o_select_selected (w_current)) {
    SELECTION *selection = toplevel->page_current->selection_list;
    GList *s_current;

    for (s_current = geda_list_get_glist (selection);
         s_current != NULL;
         s_current = g_list_next (s_current)) {
      OBJECT *object = (OBJECT*)s_current->data;
      if (object->type == OBJ_TEXT)
        o_attrib_toggle_show_name_value (w_current, object, SHOW_NAME);
    }

    o_undo_savestate_old (w_current, UNDO_ALL);
  }
}

DEFINE_I_CALLBACK(attributes_show_value)
{
  GschemToplevel *w_current = GSCHEM_TOPLEVEL (data);
  TOPLEVEL *toplevel = gschem_toplevel_get_toplevel (w_current);

  g_return_if_fail (w_current != NULL);

  /* This is a new addition 3/15 to prevent this from executing
   * inside an action */
  if (w_current->inside_action) {
    return;
  }

  i_update_middle_button(w_current, i_callback_attributes_show_value,
                         _("ShowV"));

  if (o_select_selected (w_current)) {
    SELECTION *selection = toplevel->page_current->selection_list;
    GList *s_current;

    for (s_current = geda_list_get_glist (selection);
         s_current != NULL;
         s_current = g_list_next (s_current)) {
      OBJECT *object = (OBJECT*)s_current->data;
      if (object->type == OBJ_TEXT)
        o_attrib_toggle_show_name_value (w_current, object, SHOW_VALUE);
    }

    o_undo_savestate_old (w_current, UNDO_ALL);
  }
}

DEFINE_I_CALLBACK(attributes_show_both)
{
  GschemToplevel *w_current = GSCHEM_TOPLEVEL (data);
  TOPLEVEL *toplevel = gschem_toplevel_get_toplevel (w_current);

  g_return_if_fail (w_current != NULL);

  /* This is a new addition 3/15 to prevent this from executing
   * inside an action */
  if (w_current->inside_action) {
    return;
  }

  i_update_middle_button(w_current, i_callback_attributes_show_both,
                         _("ShowB"));

  if (o_select_selected (w_current)) {
    SELECTION *selection = toplevel->page_current->selection_list;
    GList *s_current;

    for (s_current = geda_list_get_glist (selection);
         s_current != NULL;
         s_current = g_list_next (s_current)) {
      OBJECT *object = (OBJECT*)s_current->data;
      if (object->type == OBJ_TEXT)
        o_attrib_toggle_show_name_value (w_current, object, SHOW_NAME_VALUE);
    }

    o_undo_savestate_old (w_current, UNDO_ALL);
  }
}

DEFINE_I_CALLBACK(attributes_visibility_toggle)
{
  GschemToplevel *w_current = GSCHEM_TOPLEVEL (data);
  TOPLEVEL *toplevel = gschem_toplevel_get_toplevel (w_current);

  g_return_if_fail (w_current != NULL);

  /* This is a new addition 3/15 to prevent this from executing
   * inside an action */
  if (w_current->inside_action) {
    return;
  }

  i_update_middle_button(w_current,
                         i_callback_attributes_visibility_toggle,
                         _("VisToggle"));

  if (o_select_selected (w_current)) {
    SELECTION *selection = toplevel->page_current->selection_list;
    GList *s_current;

    for (s_current = geda_list_get_glist (selection);
         s_current != NULL;
         s_current = g_list_next (s_current)) {
      OBJECT *object = (OBJECT*)s_current->data;
      if (object->type == OBJ_TEXT)
        o_attrib_toggle_visibility (w_current, object);
    }

    o_undo_savestate_old (w_current, UNDO_ALL);
  }
}

/*! \section options-menu Options Menu Callback Functions */

DEFINE_I_CALLBACK(options_text_size)
{
  /* repeat last command doesn't make sense on options either??? (does it?) */
  GschemToplevel *w_current = GSCHEM_TOPLEVEL (data);

  g_return_if_fail (w_current != NULL);
  gschem_dockable_present (w_current->text_properties_dockable);
}

DEFINE_I_CALLBACK(options_snap_size)
{
  GschemToplevel *w_current = GSCHEM_TOPLEVEL (data);

  g_return_if_fail (w_current != NULL);
  gschem_dockable_present (w_current->options_dockable);
}

/*! \brief Multiply snap grid size by two. */

DEFINE_I_CALLBACK(options_scale_up_snap_size)
{
  GschemToplevel *w_current = GSCHEM_TOPLEVEL (data);

  g_return_if_fail (w_current != NULL);

  gschem_options_scale_snap_up (w_current->options);
}

/*! \brief Divide snap grid size by two (if it's and even number). */

DEFINE_I_CALLBACK(options_scale_down_snap_size)
{
  GschemToplevel *w_current = GSCHEM_TOPLEVEL (data);

  g_return_if_fail (w_current != NULL);

  gschem_options_scale_snap_down (w_current->options);
}

DEFINE_I_CALLBACK(options_afeedback)
{
  /* repeat last command doesn't make sense on options either??? (does it?) */
  GschemToplevel *w_current = GSCHEM_TOPLEVEL (data);

  g_return_if_fail (w_current != NULL);

  if (w_current->actionfeedback_mode == BOUNDINGBOX) {
    w_current->actionfeedback_mode = OUTLINE;
    s_log_message(_("Action feedback mode set to OUTLINE\n"));
  } else {
    w_current->actionfeedback_mode = BOUNDINGBOX;
    s_log_message(_("Action feedback mode set to BOUNDINGBOX\n"));
  }
  if (w_current->inside_action &&
      gschem_toplevel_get_toplevel (w_current)->page_current->place_list != NULL)
    o_place_invalidate_rubber (w_current, FALSE);
}

DEFINE_I_CALLBACK(options_grid)
{
  GRID_MODE grid_mode;
  GschemToplevel *w_current = GSCHEM_TOPLEVEL (data);

  g_return_if_fail (w_current != NULL);

  gschem_options_cycle_grid_mode (w_current->options);

  grid_mode = gschem_options_get_grid_mode (w_current->options);

  switch (grid_mode) {
    case GRID_MODE_NONE: s_log_message (_("Grid OFF\n"));           break;
    case GRID_MODE_DOTS: s_log_message (_("Dot grid selected\n"));  break;
    case GRID_MODE_MESH: s_log_message (_("Mesh grid selected\n")); break;
    default:             s_log_message (_("Invalid grid mode\n"));
  }
}

DEFINE_I_CALLBACK(options_snap)
{
  SNAP_STATE snap_mode;
  GschemToplevel *w_current = GSCHEM_TOPLEVEL (data);

  g_return_if_fail (w_current != NULL);

  gschem_options_cycle_snap_mode (w_current->options);

  snap_mode = gschem_options_get_snap_mode (w_current->options);

  switch (snap_mode) {
  case SNAP_OFF:
    s_log_message(_("Snap OFF (CAUTION!)\n"));
    break;
  case SNAP_GRID:
    s_log_message(_("Snap ON\n"));
    break;
  case SNAP_RESNAP:
    s_log_message(_("Snap back to the grid (CAUTION!)\n"));
    break;
  default:
    g_critical("options_snap: toplevel->snap out of range: %d\n",
               snap_mode);
  }

  i_show_state(w_current, NULL); /* update status on screen */
  i_update_grid_info (w_current);
}

DEFINE_I_CALLBACK(options_rubberband)
{
  /* Rubber band is cool !
   * Added on/off option from the pull down menu
   * Chris Ellec - January 2001 */
  GschemToplevel *w_current = GSCHEM_TOPLEVEL (data);

  g_return_if_fail (w_current != NULL);

  gschem_options_cycle_net_rubber_band_mode (w_current->options);

  if (gschem_options_get_net_rubber_band_mode (w_current->options)) {
    s_log_message(_("Rubber band ON\n"));
  } else {
    s_log_message(_("Rubber band OFF \n"));
  }
}

/*! \brief Toggle magnetic net mode option ON and OFF. */

DEFINE_I_CALLBACK(options_magneticnet)
{
  GschemToplevel *w_current = GSCHEM_TOPLEVEL (data);

  g_return_if_fail (w_current != NULL);

  gschem_options_cycle_magnetic_net_mode (w_current->options);

  if (gschem_options_get_magnetic_net_mode (w_current->options)) {
    s_log_message(_("magnetic net mode: ON\n"));
  }
  else {
    s_log_message(_("magnetic net mode: OFF\n"));
  }

  i_show_state(w_current, NULL);
}

DEFINE_I_CALLBACK(options_show_log_window)
{
  GschemToplevel *w_current = GSCHEM_TOPLEVEL (data);

  g_return_if_fail (w_current != NULL);

  gschem_dockable_present (w_current->log_dockable);
}

DEFINE_I_CALLBACK(cancel)
{
  GschemToplevel *w_current = GSCHEM_TOPLEVEL (data);
  g_return_if_fail (w_current != NULL);

  i_cancel (w_current);
}

/*! \section help-menu Help Menu Callback Functions */

DEFINE_I_CALLBACK(help_about)
{
  GschemToplevel *w_current = GSCHEM_TOPLEVEL (data);

  g_return_if_fail (w_current != NULL);
  about_dialog(w_current);
}

DEFINE_I_CALLBACK(help_hotkeys)
{
  GschemToplevel *w_current = GSCHEM_TOPLEVEL (data);

  g_return_if_fail (w_current != NULL);
  x_dialog_hotkeys(w_current);
}

DEFINE_I_CALLBACK(options_show_coord_window)
{
  GschemToplevel *w_current = GSCHEM_TOPLEVEL (data);

  g_return_if_fail (w_current != NULL);
  coord_dialog (w_current, 0, 0);
}
