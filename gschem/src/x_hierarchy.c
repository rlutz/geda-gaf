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

/*! \file x_hierarchy.c
 * \brief Hierarchy navigation functions.
 */
#include <config.h>

#include "gschem.h"


void
x_hierarchy_down_schematic (GschemToplevel *w_current, OBJECT *object)
{
  char *attrib=NULL;
  char *current_filename=NULL;
  int count=0;
  PAGE *save_first_page=NULL;
  PAGE *parent=NULL;
  PAGE *child = NULL;
  int loaded_flag=FALSE;
  int page_control = 0;
  int pcount = 0;
  int looking_inside=FALSE;

  /* only allow going into symbols */
  if (object->type != OBJ_COMPLEX)
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
      PAGE *saved_page = gschem_toplevel_get_toplevel (w_current)->page_current;
      child = s_hierarchy_down_schematic_single(gschem_toplevel_get_toplevel (w_current),
                                                current_filename,
                                                parent,
                                                page_control,
                                                HIERARCHY_NORMAL_LOAD,
                                                &err);

      /* s_hierarchy_down_schematic_single() will not zoom the loaded page */
      if (child != NULL) {
        gtk_recent_manager_add_item (recent_manager,
                                     g_filename_to_uri (child->page_filename,
                                                        NULL, NULL));

        s_page_goto (gschem_toplevel_get_toplevel (w_current), child);
        gschem_toplevel_page_changed (w_current);
        gschem_page_view_zoom_extents (gschem_toplevel_get_current_page_view (w_current),
                                       NULL);
        o_undo_savestate_old (w_current, UNDO_ALL, NULL);
      }
      if (saved_page != NULL) {
        s_page_goto (gschem_toplevel_get_toplevel (w_current), saved_page);
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


/*! \bug may cause problems with non-directory symbols */
void
x_hierarchy_down_symbol (GschemToplevel *w_current, OBJECT *object)
{
  const CLibSymbol *sym;

  /* only allow going into symbols */
  if (object->type == OBJ_COMPLEX) {
    if (object->complex_embedded) {
      s_log_message(_("Cannot descend into embedded symbol!\n"));
      return;
    }
    s_log_message(_("Searching for symbol [%s]\n"),
		    object->complex_basename);
    sym = s_clib_get_symbol_by_name (object->complex_basename);
    if (sym == NULL)
	return;
    gchar *filename = s_clib_symbol_get_filename(sym);
    if (filename == NULL) {
	s_log_message(_("Symbol is not a real file."
			" Symbol cannot be loaded.\n"));
      g_free (filename);
	return;
    }
    g_free (filename);
    PAGE *saved_page = gschem_toplevel_get_toplevel (w_current)->page_current;
    s_hierarchy_down_symbol(gschem_toplevel_get_toplevel (w_current), sym,
			      gschem_toplevel_get_toplevel (w_current)->page_current);
    PAGE *page = gschem_toplevel_get_toplevel (w_current)->page_current;
    if (saved_page != NULL) {
      s_page_goto (gschem_toplevel_get_toplevel (w_current), saved_page);
      gschem_toplevel_page_changed (w_current);
    }
    gtk_recent_manager_add_item (recent_manager,
                                 g_filename_to_uri (page->page_filename,
                                                    NULL, NULL));

    x_window_set_current_page (w_current, page);
    /* s_hierarchy_down_symbol() will not zoom the loaded page */
    gschem_page_view_zoom_extents (gschem_toplevel_get_current_page_view (w_current),
                                   NULL);
    o_undo_savestate_old (w_current, UNDO_ALL, NULL);
  }
}


void
x_hierarchy_up (GschemToplevel *w_current)
{
  PAGE *page = NULL;
  PAGE *up_page = NULL;

  page = gschem_toplevel_get_toplevel (w_current)->page_current;

  if (page == NULL) {
    return;
  }

  up_page = s_hierarchy_find_up_page (gschem_toplevel_get_toplevel (w_current)->pages, page);
  if (up_page == NULL) {
    s_log_message(_("Cannot find any schematics above the current one!\n"));
  } else {
    if (!x_highlevel_close_page (w_current, page))
      return;
    x_window_set_current_page(w_current, up_page);
  }
}
