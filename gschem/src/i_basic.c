/* gEDA - GPL Electronic Design Automation
 * gschem - gEDA Schematic Capture
 * Copyright (C) 1998-2010 Ales Hvezda
 * Copyright (C) 1998-2010 gEDA Contributors (see ChangeLog for details)
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

#ifdef HAVE_STRING_H
#include <string.h>
#endif

#include "gschem.h"
#include "actions.decl.x"

/*! \brief Update status bar string
 *
 *  \par Function Description
 *  This function actually updates the status bar
 *  widget with the new string.
 *
 *  \param [in] w_current GschemToplevel structure
 *  \param [in] string The new string to be shown in the status bar
 */
static void i_update_status(GschemToplevel *w_current, const char *string)
{
  if (!w_current->bottom_widget) {
    return;
  }

  if (string) {
    /* NOTE: consider optimizing this if same label */
    gschem_bottom_widget_set_status_text (GSCHEM_BOTTOM_WIDGET (w_current->bottom_widget), string);
  }
}

/*! \brief Get string corresponding to the currently selected mode
 *
 *  \par Function Description
 *  Returns a string describing the currently
 *  selected mode.
 *
 *  \param [in] w_current GschemToplevel structure
 *  \returns a string that will only last until the next time
 *   the function is called (which is probably just fine, really)
 *   *EK* Egil Kvaleberg
 */
static const char *i_status_string(GschemToplevel *w_current)
{
  static char *buf = 0;

  switch ( w_current->event_state ) {
    case SELECT     : return _("Select Mode");
    case SBOX       : return _("Select Box Mode");
    case TEXTMODE   : return _("Text Mode");
    case PAN        : return _("Pan Mode");
    case PASTEMODE:
      g_free(buf);
      buf = g_strdup_printf(_("Paste %d Mode"), w_current->buffer_number+1);
      return buf;
    case NETMODE:
      if (gschem_options_get_magnetic_net_mode (w_current->options))
        return _("Magnetic Net Mode");
      else
        return _("Net Mode");
    case ARCMODE    : return _("Arc Mode");
    case BOXMODE    : return _("Box Mode");
    case BUSMODE    : return _("Bus Mode");
    case CIRCLEMODE : return _("Circle Mode");
    case COMPMODE   : return _("Component Mode");
    case COPYMODE   : return _("Copy Mode");
    case MCOPYMODE  : return _("Multiple Copy Mode");
    case LINEMODE   : return _("Line Mode");
    case MIRRORMODE : return _("Mirror Mode");
    case MOVEMODE   : return _("Move Mode");
    case PATHMODE   : return _("Path Mode");
    case PICTUREMODE: return _("Picture Mode");
    case PINMODE    : return _("Pin Mode");
    case ROTATEMODE : return _("Rotate Mode");
    case GRIPS      : return _("Modify Mode");
    case ZOOMBOX    : return _("Zoom Box");
  }
  g_assert_not_reached();
  return ""; /* should not happen */
}

/*! \brief Show state field
 *
 *  \par Function Description
 *  Show state field on screen, possibly with the
 *  addition of an extra message
 *
 *  \param [in] w_current GschemToplevel structure
 *  \param [in] message The string to be displayed
 */
void i_show_state(GschemToplevel *w_current, const char *message)
{
  TOPLEVEL *toplevel = gschem_toplevel_get_toplevel (w_current);
  gchar *what_to_say;
  const gchar *array[5] = { NULL };
  int i = 3; /* array[4] must be NULL */
  SNAP_STATE snap_mode;

  /* Fill in the string array */
  array[i--] = i_status_string(w_current);

  snap_mode = gschem_options_get_snap_mode (w_current->options);

  if(toplevel->show_hidden_text)
    array[i--] = _("Show Hidden");

  if(snap_mode == SNAP_OFF)
    array[i--] = _("Snap Off");
  else if (snap_mode == SNAP_RESNAP)
    array[i--] = _("Resnap Active");

  if(message && message[0])
    array[i] = message;

  /* Skip over NULLs */
  while(array[i] == NULL)
    i++;

  what_to_say = g_strjoinv(" - ", (gchar **) array + i);

  if(w_current->keyaccel_string) {
     gchar *p = what_to_say;

     what_to_say = g_strdup_printf("%s \t\t %s", w_current->keyaccel_string,
           what_to_say);
     g_free(p);
  }

  i_update_status(w_current, what_to_say);
  g_free(what_to_say);
}


/*! \brief Mark start of an editing action
 *
 *  \par Function Description
 *  Calls i_action_update_status() informing it that the new
 *  editing action is started.
 *
 *  \param [in] w_current GschemToplevel structure
 */
void i_action_start (GschemToplevel *w_current)
{
  i_action_update_status (w_current, TRUE);
}


/*! \brief Mark end of an editing action
 *
 *  \par Function Description
 *  Calls i_action_update_status() informing it that the current
 *  editing action is finished.
 *
 *  \param [in] w_current GschemToplevel structure
 */
void i_action_stop (GschemToplevel *w_current)
{
  i_action_update_status (w_current, FALSE);
}


/*! \brief Update status of an editing action
 *
 *  \par Function Description
 *  Checks if the current action state has been changed (an action
 *  was started or finished) and informs the bottom widget to make
 *  it update the status text color accordingly
 *
 *  \param [in] w_current GschemToplevel structure
 */
void i_action_update_status (GschemToplevel *w_current, gboolean inside_action)
{
  if (w_current->inside_action != inside_action) {
    w_current->inside_action = inside_action;
    gschem_bottom_widget_set_status_text_color (GSCHEM_BOTTOM_WIDGET (w_current->bottom_widget),
                                                inside_action);
  }
}


/*! \brief Set new state, then show state field
 *
 *  \par Function Description
 *  Set new state, then show state field.
 *
 *  \param [in] w_current GschemToplevel structure
 *  \param [in] newstate The new state
 *   *EK* Egil Kvaleberg
 */
void i_set_state(GschemToplevel *w_current, enum x_states newstate)
{
  i_set_state_msg(w_current, newstate, NULL);
}

/*! \brief Set new state, then show state field including some
 *         message
 *
 *  \par Function Description
 *  Set new state, then show state field including some
 *  message.
 *
 *  \param [in] w_current GschemToplevel structure
 *  \param [in] newstate The new state
 *  \param [in] message Message to be shown
 *   *EK* Egil Kvaleberg
 */
void i_set_state_msg(GschemToplevel *w_current, enum x_states newstate,
		     const char *message)
{
  if ((newstate != w_current->event_state) || (message != NULL)) {
    w_current->event_state = newstate;
    i_update_toolbar (w_current);
  }
  i_show_state(w_current, message);
}

/*! \todo Finish function documentation!!!
 *  \brief
 *  \par Function Description
 *
 */
void i_update_middle_button (GschemToplevel *w_current,
                             GschemAction *action,
                             const char *string)
{
  g_return_if_fail (w_current != NULL);
  g_return_if_fail (w_current->bottom_widget != NULL);

  switch(w_current->middle_button) {

    /* remove this case eventually and make it a null case */
    case(ACTION):
      gschem_bottom_widget_set_middle_button_text (
          GSCHEM_BOTTOM_WIDGET (w_current->bottom_widget),
          _("Action"));
      break;

#ifdef HAVE_LIBSTROKE
    case(STROKE):
      gschem_bottom_widget_set_middle_button_text (
          GSCHEM_BOTTOM_WIDGET (w_current->bottom_widget),
          _("Stroke"));
    break;
#else
    /* remove this case eventually and make it a null case */
    case(STROKE):
      gschem_bottom_widget_set_middle_button_text (
          GSCHEM_BOTTOM_WIDGET (w_current->bottom_widget),
          _("none"));
      break;
#endif

    case(REPEAT):
      if ((string != NULL) && (action != NULL))
      {
        char *temp_string = g_strconcat (_("Repeat/"), string, NULL);

        gschem_bottom_widget_set_middle_button_text (
            GSCHEM_BOTTOM_WIDGET (w_current->bottom_widget),
            temp_string);

        g_free(temp_string);
      } else {
        gschem_bottom_widget_set_middle_button_text (
            GSCHEM_BOTTOM_WIDGET (w_current->bottom_widget),
            _("Repeat/none"));
      }
      break;

    case(MID_MOUSEPAN_ENABLED):
      gschem_bottom_widget_set_middle_button_text (
          GSCHEM_BOTTOM_WIDGET (w_current->bottom_widget),
          _("Pan"));
      break;

    default:
      gschem_bottom_widget_set_middle_button_text (
          GSCHEM_BOTTOM_WIDGET (w_current->bottom_widget),
          _("none"));
  }

  if (action != NULL)
    w_current->last_action = action;
}

/*! \todo Finish function documentation!!!
 *  \brief
 *  \param [in] w_current GschemToplevel structure
 *
 */
void i_update_toolbar(GschemToplevel *w_current)
{
  if (!w_current->toolbars)
	return;

  gschem_action_set_active (action_edit_select,
                            w_current->event_state == SELECT ||
                            w_current->event_state == GRIPS ||
                            w_current->event_state == MOVEMODE ||
                            w_current->event_state == PASTEMODE ||
                            w_current->event_state == SBOX, w_current);

  gschem_action_set_active (action_add_arc,
                            w_current->event_state == ARCMODE, w_current);
  gschem_action_set_active (action_add_box,
                            w_current->event_state == BOXMODE, w_current);
  gschem_action_set_active (action_add_bus,
                            w_current->event_state == BUSMODE, w_current);
  gschem_action_set_active (action_add_circle,
                            w_current->event_state == CIRCLEMODE, w_current);
  gschem_action_set_active (action_add_line,
                            w_current->event_state == LINEMODE, w_current);
  gschem_action_set_active (action_add_net,
                            w_current->event_state == NETMODE, w_current);
  gschem_action_set_active (action_add_path,
                            w_current->event_state == PATHMODE, w_current);
  gschem_action_set_active (action_add_picture,
                            w_current->event_state == PICTUREMODE, w_current);
  gschem_action_set_active (action_add_pin,
                            w_current->event_state == PINMODE, w_current);

  gschem_action_set_active (action_add_component,
                            w_current->event_state == COMPMODE, w_current);
  gschem_action_set_active (action_add_text,
                            w_current->event_state == TEXTMODE, w_current);

  gschem_action_set_active (action_edit_copy,
                            w_current->event_state == COPYMODE, w_current);
  gschem_action_set_active (action_edit_mcopy,
                            w_current->event_state == MCOPYMODE, w_current);
  gschem_action_set_active (action_edit_rotate_90,
                            w_current->event_state == ROTATEMODE, w_current);
  gschem_action_set_active (action_edit_mirror,
                            w_current->event_state == MIRRORMODE, w_current);

  gschem_action_set_active (action_view_pan,
                            w_current->event_state == PAN, w_current);
  gschem_action_set_active (action_view_zoom_box,
                            w_current->event_state == ZOOMBOX, w_current);
}


/*! \brief Update sensitivity of the Edit/Paste menu item
 *
 *  \par Function Description
 *  Asynchronous callback to update sensitivity of the Edit/Paste
 *  menu item.
 */
static void clipboard_usable_cb (int usable, void *userdata)
{
  GschemToplevel *w_current = userdata;
  gschem_action_set_sensitive (action_clipboard_paste, usable, w_current);
}

static gboolean
selected_at_least_one_text_object(GschemToplevel *w_current)
{
  OBJECT *obj;
  TOPLEVEL *toplevel = gschem_toplevel_get_toplevel (w_current);
  GList *list = geda_list_get_glist(toplevel->page_current->selection_list);

  while(list != NULL) {
    obj = (OBJECT *) list->data;
    if (obj->type == OBJ_TEXT)
      return TRUE;
    list = g_list_next(list);
  }
  return FALSE;
}


/*! \brief Update sensitivity of relevant menu items
 *
 *  \par Function Description
 *  Update sensitivity of relevant menu items.
 *
 *  \param [in] w_current GschemToplevel structure
 */
void i_update_menus(GschemToplevel *w_current)
{
  gboolean have_text_selected;
  TOPLEVEL *toplevel = gschem_toplevel_get_toplevel (w_current);
  /*
   * This is very simplistic.  Right now it just disables all menu
   * items which get greyed out when a component is not selected.
   * Eventually what gets enabled/disabled
   * should be based on what is in the selection list
   */

  g_assert(w_current != NULL);
  g_assert(toplevel->page_current != NULL);

  x_clipboard_query_usable (w_current, clipboard_usable_cb, w_current);

  if (o_select_selected (w_current)) {
    have_text_selected = selected_at_least_one_text_object(w_current);

    /* since one or more things are selected, we set these TRUE */
    /* These strings should NOT be internationalized */
    gschem_action_set_sensitive (action_clipboard_cut, TRUE, w_current);
    gschem_action_set_sensitive (action_clipboard_copy, TRUE, w_current);
    gschem_action_set_sensitive (action_edit_deselect, TRUE, w_current);
    gschem_action_set_sensitive (action_edit_delete, TRUE, w_current);
    gschem_action_set_sensitive (action_edit_copy, TRUE, w_current);
    gschem_action_set_sensitive (action_edit_mcopy, TRUE, w_current);
    gschem_action_set_sensitive (action_edit_move, TRUE, w_current);
    gschem_action_set_sensitive (action_edit_rotate_90, TRUE, w_current);
    gschem_action_set_sensitive (action_edit_mirror, TRUE, w_current);
    gschem_action_set_sensitive (action_edit_edit, TRUE, w_current);
    gschem_action_set_sensitive (action_edit_pin_type, TRUE, w_current);
    gschem_action_set_sensitive (action_edit_text, TRUE, w_current);
    gschem_action_set_sensitive (action_edit_slot, TRUE, w_current);
    gschem_action_set_sensitive (action_edit_color, TRUE, w_current);
    gschem_action_set_sensitive (action_edit_lock, TRUE, w_current);
    gschem_action_set_sensitive (action_edit_unlock, TRUE, w_current);
    gschem_action_set_sensitive (action_edit_embed, TRUE, w_current);
    gschem_action_set_sensitive (action_edit_unembed, TRUE, w_current);
    gschem_action_set_sensitive (action_edit_update, TRUE, w_current);
    gschem_action_set_sensitive (action_edit_linetype, TRUE, w_current);
    gschem_action_set_sensitive (action_edit_filltype, TRUE, w_current);
    gschem_action_set_sensitive (action_hierarchy_down_schematic, TRUE, w_current);
    gschem_action_set_sensitive (action_hierarchy_down_symbol, TRUE, w_current);
    gschem_action_set_sensitive (action_hierarchy_documentation, TRUE, w_current);
    gschem_action_set_sensitive (action_attributes_attach, TRUE, w_current);
    gschem_action_set_sensitive (action_attributes_detach, TRUE, w_current);
    gschem_action_set_sensitive (action_attributes_show_value, have_text_selected, w_current);
    gschem_action_set_sensitive (action_attributes_show_name, have_text_selected, w_current);
    gschem_action_set_sensitive (action_attributes_show_both, have_text_selected, w_current);
    gschem_action_set_sensitive (action_attributes_visibility_toggle, have_text_selected, w_current);

  } else {
    /* Nothing is selected, grey these out */
    /* These strings should NOT be internationalized */
    gschem_action_set_sensitive (action_clipboard_cut, FALSE, w_current);
    gschem_action_set_sensitive (action_clipboard_copy, FALSE, w_current);
    gschem_action_set_sensitive (action_edit_deselect, FALSE, w_current);
    gschem_action_set_sensitive (action_edit_delete, FALSE, w_current);
    gschem_action_set_sensitive (action_edit_copy, FALSE, w_current);
    gschem_action_set_sensitive (action_edit_mcopy, FALSE, w_current);
    gschem_action_set_sensitive (action_edit_move, FALSE, w_current);
    gschem_action_set_sensitive (action_edit_rotate_90, FALSE, w_current);
    gschem_action_set_sensitive (action_edit_mirror, FALSE, w_current);
    gschem_action_set_sensitive (action_edit_edit, FALSE, w_current);
    gschem_action_set_sensitive (action_edit_pin_type, FALSE, w_current);
    gschem_action_set_sensitive (action_edit_text, FALSE, w_current);
    gschem_action_set_sensitive (action_edit_slot, FALSE, w_current);
    gschem_action_set_sensitive (action_edit_color, FALSE, w_current);
    gschem_action_set_sensitive (action_edit_lock, FALSE, w_current);
    gschem_action_set_sensitive (action_edit_unlock, FALSE, w_current);
    gschem_action_set_sensitive (action_edit_embed, FALSE, w_current);
    gschem_action_set_sensitive (action_edit_unembed, FALSE, w_current);
    gschem_action_set_sensitive (action_edit_update, FALSE, w_current);
    gschem_action_set_sensitive (action_edit_linetype, FALSE, w_current);
    gschem_action_set_sensitive (action_edit_filltype, FALSE, w_current);
    gschem_action_set_sensitive (action_hierarchy_down_schematic, FALSE, w_current);
    gschem_action_set_sensitive (action_hierarchy_down_symbol, FALSE, w_current);
    gschem_action_set_sensitive (action_hierarchy_documentation, FALSE, w_current);
    gschem_action_set_sensitive (action_attributes_attach, FALSE, w_current);
    gschem_action_set_sensitive (action_attributes_detach, FALSE, w_current);
    gschem_action_set_sensitive (action_attributes_show_value, FALSE, w_current);
    gschem_action_set_sensitive (action_attributes_show_name, FALSE, w_current);
    gschem_action_set_sensitive (action_attributes_show_both, FALSE, w_current);
    gschem_action_set_sensitive (action_attributes_visibility_toggle, FALSE, w_current);
  }


}

/*! \brief Set filename as gschem window title
 *
 *  \par Function Description
 *  Set filename as gschem window title using
 *  the gnome HID format style.
 *
 *  \param [in] w_current GschemToplevel structure
 *  \param [in] string The filename
 *  \param [in] string 'Page changed' indication in window's title
 */
void i_set_filename(GschemToplevel *w_current, const gchar *string, const gchar *changed)
{
  gchar *print_string=NULL;
  gchar *filename=NULL;

  if (!w_current->main_window)
    return;
  if (string == NULL)
    return;

  filename = g_path_get_basename(string);

  print_string = g_strdup_printf("%s%s - gschem", changed, filename);

  gtk_window_set_title(GTK_WINDOW(w_current->main_window),
		       print_string);

  g_free(print_string);
  g_free(filename);
}

/*! \brief Write the grid settings to the gschem status bar
 *
 *  \par Function Description
 *  Write the grid settings to the gschem status bar.
 *  The function takes the current grid paramters of gschem
 *  and prints it to the status bar.
 *  The format is "Grid([SnapGridSize],[CurrentGridSize])"
 *
 *  \param [in] w_current GschemToplevel structure
 */
void
i_update_grid_info (GschemToplevel *w_current)
{
  g_return_if_fail (w_current != NULL);

  if (w_current->bottom_widget != NULL) {
    g_object_set (GSCHEM_BOTTOM_WIDGET (w_current->bottom_widget),
        "snap-mode", gschem_options_get_snap_mode (w_current->options),
        "snap-size", gschem_options_get_snap_size (w_current->options),
        "grid-mode", gschem_options_get_grid_mode (w_current->options),
        "grid-size", x_grid_query_drawn_spacing (w_current),
        NULL);
  }
}



/*! \brief Write the grid settings to the gschem status bar
 *
 *  \param [in] view The page view originating the signal
 *  \param [in] w_current GschemToplevel structure
 */
void
i_update_grid_info_callback (GschemPageView *view, GschemToplevel *w_current)
{
  i_update_grid_info (w_current);
}


/*! \todo Finish function documentation!!!
 *  \brief
 *  \par Function Description
 *
 */
void
i_cancel (GschemToplevel *w_current)
{
  TOPLEVEL *toplevel = gschem_toplevel_get_toplevel (w_current);

  g_return_if_fail (w_current != NULL);

  if (w_current->event_state == COMPMODE) {
    /* user hit escape key when placing components */

    /* Undraw any outline of the place list */
    o_place_invalidate_rubber (w_current, FALSE);
    w_current->rubber_visible = 0;

    /* De-select the lists in the component selector */
    x_compselect_deselect (w_current);

    /* Present the component selector again */
    gschem_dockable_present (w_current->compselect_dockable);
  }

  if (w_current->inside_action) {
    /* If we're cancelling from a move action, re-wind the
     * page contents back to their state before we started */
    o_move_cancel (w_current);
  }

    /* If we're cancelling from a grip action, call the specific cancel
     * routine to reset the visibility of the object being modified */
  if (w_current->event_state == GRIPS) {
    o_grips_cancel (w_current);
  }

  /* Free the place list and its contents. If we were in a move
   * action, the list (refering to objects on the page) would
   * already have been cleared in o_move_cancel(), so this is OK. */
  if (toplevel->page_current != NULL) {
    s_delete_object_glist(toplevel, toplevel->page_current->place_list);
    toplevel->page_current->place_list = NULL;
  }

  /* leave this on for now... but it might have to change */
  /* this is problematic since we don't know what the right mode */
  /* (when you cancel inside an action) should be */
  i_set_state(w_current, SELECT);

  /* clear the key guile command-sequence */
  g_keys_reset (w_current);

  gschem_page_view_invalidate_all (gschem_toplevel_get_current_page_view (w_current));

  i_action_stop (w_current);
}


/*! \section buffer-menu Buffer Menu Callback Functions */
/*! \todo Finish function documentation!!!
 *  \brief
 *  \par Function Description
 *
 */
void
i_buffer_copy (GschemToplevel *w_current, int n, GschemAction *action)
{
  gchar *msg;

  g_return_if_fail (w_current != NULL);

  if (!o_select_selected (w_current))
    return;

  /* TRANSLATORS: The number is the number of the buffer that the
   * selection is being copied to. */
  msg = g_strdup_printf(_("Copy %i"), n);
  i_update_middle_button(w_current, action, msg);
  g_free (msg);
  o_buffer_copy(w_current, n-1);
  i_update_menus(w_current);
}

/*! \todo Finish function documentation!!!
 *  \brief
 *  \par Function Description
 *
 */
void
i_buffer_cut (GschemToplevel *w_current, int n, GschemAction *action)
{
  gchar *msg;

  g_return_if_fail (w_current != NULL);

  if (!o_select_selected (w_current))
    return;

  /* TRANSLATORS: The number is the number of the buffer that the
   * selection is being cut to. */
  msg = g_strdup_printf(_("Cut %i"), n);
  i_update_middle_button(w_current, action, msg);
  g_free (msg);
  o_buffer_cut(w_current, n-1);
  i_update_menus(w_current);
}

/*! \todo Finish function documentation!!!
 *  \brief
 *  \par Function Description
 *
 */
void
i_buffer_paste (GschemToplevel *w_current, int n, GschemAction *action)
{
  gchar *msg;
  int empty;

  /* Choose a default position to start pasting. This is required to
   * make pasting when the cursor is outside the screen or pasting via
   * menu work as expected. */
  gint wx = 0, wy = 0;

  g_return_if_fail (w_current != NULL);

  /* TRANSLATORS: The number is the number of the buffer that is being
   * pasted to the schematic. */
  msg = g_strdup_printf(_("Paste %i"), n);
  i_update_middle_button(w_current, action, msg);
  g_free (msg);

  g_action_get_position (TRUE, &wx, &wy);

  empty = o_buffer_paste_start (w_current, wx, wy, n-1);

  if (empty) {
    i_set_state_msg(w_current, SELECT, _("Empty buffer"));
  }
}
