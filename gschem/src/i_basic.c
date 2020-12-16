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
    case OGNRSTMODE : return _("Reset Origin Mode");
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
          pgettext ("mmb", "Action"));
      break;

    case(STROKE):
      gschem_bottom_widget_set_middle_button_text (
          GSCHEM_BOTTOM_WIDGET (w_current->bottom_widget),
          pgettext ("mmb", "Stroke"));
      break;

    case(REPEAT):
      if ((string != NULL) && (action != NULL))
      {
        char *temp_string = g_strdup_printf (
            pgettext ("mmb", "Repeat/%s"), string);

        gschem_bottom_widget_set_middle_button_text (
            GSCHEM_BOTTOM_WIDGET (w_current->bottom_widget),
            temp_string);

        g_free(temp_string);
      } else {
        gschem_bottom_widget_set_middle_button_text (
            GSCHEM_BOTTOM_WIDGET (w_current->bottom_widget),
            pgettext ("mmb", "Repeat/none"));
      }
      break;

    case(MID_MOUSEPAN_ENABLED):
      gschem_bottom_widget_set_middle_button_text (
          GSCHEM_BOTTOM_WIDGET (w_current->bottom_widget),
          pgettext ("mmb", "Pan"));
      break;

    default:
      gschem_bottom_widget_set_middle_button_text (
          GSCHEM_BOTTOM_WIDGET (w_current->bottom_widget),
          pgettext ("mmb", "none"));
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
  gschem_action_set_active (action_add_last_component,
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

  gschem_action_set_active (action_edit_translate,
                            w_current->event_state == OGNRSTMODE, w_current);
}


/*! \brief Check if an object has an attribute
 *  \par Function Description
 *  This functions returns TRUE if the given OBJECT has an attribute
 *  with the given name. Both attached and inherited attributes are
 *  checked.
 *
 *  \param [in] object       OBJECT to check
 *  \param [in] name         the attribute name to check for
 *  \return TRUE if the given OBJECT has an attribute with the given name
 */
/* It would probably be more efficient to look for them directly rather
 * than relying on the search functions. */
static int object_has_attribute (OBJECT *object, gchar *name)
{
  gchar *buf = o_attrib_search_attached_attribs_by_name (object, name, 0);
  int result = buf != NULL;

  g_free (buf);
  if (result)
    return result;

  buf = o_attrib_search_inherited_attribs_by_name (object, name, 0);
  result = buf != NULL;

  g_free (buf);
  return result;
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
  TOPLEVEL *toplevel = gschem_toplevel_get_toplevel (w_current);
  g_return_if_fail (w_current != NULL);
  g_return_if_fail (toplevel->page_current != NULL);

  gboolean sel_object = FALSE;          /* any object */
  gboolean sel_editable = FALSE;        /* object that can be edited via E E */
  gboolean sel_has_properties = FALSE;  /* object that can be edited via E P */
  gboolean sel_embeddable = FALSE;      /* component or picture */
  guint sel_attachable = 0;             /* component or net */

  gboolean sel_component = FALSE;       /* component */
  gboolean sel_unlocked = FALSE;        /* unlocked component */
  gboolean sel_locked = FALSE;          /* locked component */
  gboolean sel_referenced = FALSE;      /* referenced component */
  gboolean sel_slotted = FALSE;         /* slotted component */
  gboolean sel_subsheet = FALSE;        /* subsheet component */
  gboolean sel_documented = FALSE;      /* documented component */

  gboolean sel_text = FALSE;            /* text (whether attribute or not) */
  gboolean sel_floating = FALSE;        /* floating text */
  gboolean sel_attached = FALSE;        /* attached text */

  for (GList *l = geda_list_get_glist (toplevel->page_current->selection_list);
       l != NULL; l = l->next) {
    OBJECT *obj = (OBJECT *) l->data;
    sel_object = TRUE;

    if (obj->type == OBJ_ARC ||
        obj->type == OBJ_BUS ||
        obj->type == OBJ_COMPLEX ||
        obj->type == OBJ_NET ||
        obj->type == OBJ_PICTURE ||
        obj->type == OBJ_PIN ||
        obj->type == OBJ_PLACEHOLDER ||
        obj->type == OBJ_TEXT)
      sel_editable = TRUE;

    /* Net and bus objects have a color property, but it can't be
       changed in the Object Properties dock */
    if (obj->type == OBJ_ARC ||
        obj->type == OBJ_BOX ||
        obj->type == OBJ_CIRCLE ||
        obj->type == OBJ_LINE ||
        obj->type == OBJ_PATH ||
        obj->type == OBJ_PIN ||
        obj->type == OBJ_TEXT)
      sel_has_properties = TRUE;

    if (obj->type == OBJ_COMPLEX ||
        obj->type == OBJ_PICTURE)
      sel_embeddable = TRUE;

    if (obj->type == OBJ_NET ||
        obj->type == OBJ_BUS ||
        obj->type == OBJ_PIN ||
        obj->type == OBJ_COMPLEX ||
        obj->type == OBJ_PLACEHOLDER)
      sel_attachable++;

    if (obj->type == OBJ_COMPLEX) {
      sel_component = TRUE;

      if (obj->selectable)
        sel_unlocked = TRUE;
      else
        sel_locked = TRUE;

      if (!obj->complex_embedded) {
        /* Can only descend into symbol if it is referenced and it
           comes from a directory source (as opposed to a command or
           Guile function source). */
        const CLibSymbol *sym =
          s_clib_get_symbol_by_name (obj->complex_basename);
        gchar *filename = s_clib_symbol_get_filename (sym);
        if (filename != NULL)
          sel_referenced = TRUE;
        g_free (filename);
      }

      sel_slotted = object_has_attribute (obj, "slot");
      sel_subsheet = object_has_attribute (obj,"source");
      sel_documented = object_has_attribute (obj, "documentation");
    }

    if (obj->type == OBJ_TEXT) {
      sel_text = TRUE;
      if (obj->attached_to == NULL)
        sel_floating = TRUE;
      else
        sel_attached = TRUE;
    }
  }

  gschem_action_set_sensitive (action_clipboard_cut, sel_object, w_current);
  gschem_action_set_sensitive (action_clipboard_copy, sel_object, w_current);
  gschem_action_set_sensitive (action_edit_deselect, sel_object, w_current);
  gschem_action_set_sensitive (action_edit_delete, sel_object, w_current);
  gschem_action_set_sensitive (action_edit_copy, sel_object, w_current);
  gschem_action_set_sensitive (action_edit_mcopy, sel_object, w_current);
  gschem_action_set_sensitive (action_edit_move, sel_object, w_current);
  gschem_action_set_sensitive (action_edit_rotate_90, sel_object, w_current);
  gschem_action_set_sensitive (action_edit_mirror, sel_object, w_current);

  gschem_action_set_sensitive (action_edit_edit, sel_editable, w_current);
  gschem_action_set_sensitive (action_edit_text, sel_text, w_current);
  gschem_action_set_sensitive (action_edit_slot, sel_slotted, w_current);
  gschem_action_set_sensitive (action_edit_properties,
                               sel_has_properties, w_current);

  gschem_action_set_sensitive (action_edit_lock, sel_unlocked, w_current);
  gschem_action_set_sensitive (action_edit_unlock, sel_locked, w_current);
  gschem_action_set_sensitive (action_edit_update, sel_component, w_current);

  gschem_action_set_sensitive (action_edit_embed,
                               sel_embeddable, w_current);
  gschem_action_set_sensitive (action_edit_unembed,
                               sel_embeddable, w_current);

  gschem_action_set_sensitive (action_hierarchy_down_schematic,
                               sel_subsheet, w_current);
  gschem_action_set_sensitive (action_hierarchy_down_symbol,
                               sel_referenced, w_current);
  gschem_action_set_sensitive (action_hierarchy_documentation,
                               sel_documented, w_current);

  gschem_action_set_sensitive (action_attributes_attach,
                               sel_floating && sel_attachable == 1, w_current);
  gschem_action_set_sensitive (action_attributes_detach,
                               sel_attached, w_current);

  gschem_action_set_sensitive (action_attributes_show_value,
                               sel_text, w_current);
  gschem_action_set_sensitive (action_attributes_show_name,
                               sel_text, w_current);
  gschem_action_set_sensitive (action_attributes_show_both,
                               sel_text, w_current);
  gschem_action_set_sensitive (action_attributes_visibility_toggle,
                               sel_text, w_current);
  gschem_action_set_sensitive (action_attributes_overbar_toggle,
                               sel_text, w_current);
}

/*! \brief Update gschem window title
 *
 *  \par Function Description
 *  Set the filename of the current page as the window title using
 *  the gnome HID format style.
 *
 *  \param [in] w_current GschemToplevel structure
 */
void i_update_filename(GschemToplevel *w_current)
{
  PAGE *page = gschem_page_view_get_page (
    gschem_toplevel_get_current_page_view (w_current));
  if (page == NULL)
    return;
  g_return_if_fail (page->page_filename != NULL);
  g_return_if_fail (w_current->main_window != NULL);

  gchar *filename = g_path_get_basename (page->page_filename);
  gchar *title = page->is_untitled ?
    g_strdup_printf (_("%sgschem"),      page->CHANGED ? "* " : "") :
    g_strdup_printf (_("%s%s - gschem"), page->CHANGED ? "* " : "", filename);

  gtk_window_set_title (GTK_WINDOW (w_current->main_window), title);

  g_free (title);
  g_free (filename);
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
    if (gschem_dockable_get_state (w_current->compselect_dockable)
          == GSCHEM_DOCKABLE_STATE_HIDDEN)
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
