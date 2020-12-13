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
#include <sys/stat.h>
#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif
#ifdef HAVE_ASSERT_H
#include <assert.h>
#endif
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include "gschem.h"

/*! \brief */
struct gsubr_t {
  char* name;
  int req;
  int opt;
  int rst;
  SCM (*fnc)();
};

/*! \brief */
static struct gsubr_t gschem_funcs[] = {
  /* rc file */
  { "gschem-version",           1, 0, 0, g_rc_gschem_version },

  { "display-color-map",        0, 1, 0, g_rc_display_color_map },
  { "display-outline-color-map", 0, 1, 0, g_rc_display_outline_color_map },

  { "net-direction-mode",        1, 0, 0, g_rc_net_direction_mode },
  { "net-selection-mode",        1, 0, 0, g_rc_net_selection_mode },
  { "zoom-with-pan",             1, 0, 0, g_rc_zoom_with_pan },
  { "action-feedback-mode",      1, 0, 0, g_rc_action_feedback_mode },
  { "scrollbars",                1, 0, 0, g_rc_scrollbars },
  { "embed-components",          1, 0, 0, g_rc_embed_components },
  { "logging",                   1, 0, 0, g_rc_logging },
  { "text-size",                 1, 0, 0, g_rc_text_size },
  { "snap-size",                 1, 0, 0, g_rc_snap_size },

  { "text-caps-style",           1, 0, 0, g_rc_text_caps_style },
  { "logging-destination",       1, 0, 0, g_rc_logging_destination },

  { "attribute-name",            1, 0, 0, g_rc_attribute_name },

  { "image-color",               1, 0, 0, g_rc_image_color },
  { "image-size",                2, 0, 0, g_rc_image_size },
  { "log-window",                1, 0, 0, g_rc_log_window },
  { "log-window-type",           1, 0, 0, g_rc_log_window_type },
  { "third-button",              1, 0, 0, g_rc_third_button },
  { "third-button-cancel",       1, 0, 0, g_rc_third_button_cancel },
  { "middle-button",             1, 0, 0, g_rc_middle_button },
  { "scroll-wheel",              1, 0, 0, g_rc_scroll_wheel },
  { "net-consolidate",           1, 0, 0, g_rc_net_consolidate },
  { "file-preview",              1, 0, 0, g_rc_file_preview },
  { "enforce-hierarchy",         1, 0, 0, g_rc_enforce_hierarchy },
  { "fast-mousepan",             1, 0, 0, g_rc_fast_mousepan },
  { "raise-dialog-boxes-on-expose", 1, 0, 0, g_rc_raise_dialog_boxes_on_expose },
  { "continue-component-place",  1, 0, 0, g_rc_continue_component_place },
  { "undo-levels",               1, 0, 0, g_rc_undo_levels },
  { "undo-control",              1, 0, 0, g_rc_undo_control },
  { "undo-type",                 1, 0, 0, g_rc_undo_type },
  { "undo-panzoom",              1, 0, 0, g_rc_undo_panzoom },

  { "draw-grips",                1, 0, 0, g_rc_draw_grips },
  { "netconn-rubberband",        1, 0, 0, g_rc_netconn_rubberband },
  { "magnetic-net-mode",         1, 0, 0, g_rc_magnetic_net_mode },
  { "window-size",               2, 0, 0, g_rc_window_size },
  { "warp-cursor",               1, 0, 0, g_rc_warp_cursor },
  { "toolbars",                  1, 0, 0, g_rc_toolbars },
  { "handleboxes",               1, 0, 0, g_rc_handleboxes },
  { "bus-ripper-size",           1, 0, 0, g_rc_bus_ripper_size },
  { "bus-ripper-type",           1, 0, 0, g_rc_bus_ripper_type },
  { "bus-ripper-rotation",       1, 0, 0, g_rc_bus_ripper_rotation },
  { "force-boundingbox",         1, 0, 0, g_rc_force_boundingbox },
  { "grid-mode",                 1, 0, 0, g_rc_grid_mode },
  { "dots-grid-dot-size",        1, 0, 0, g_rc_dots_grid_dot_size },
  { "dots-grid-mode",            1, 0, 0, g_rc_dots_grid_mode },
  { "dots-grid-fixed-threshold", 1, 0, 0, g_rc_dots_grid_fixed_threshold },
  { "mesh-grid-display-threshold", 1, 0, 0, g_rc_mesh_grid_display_threshold },
  { "add-attribute-offset",      1, 0, 0, g_rc_add_attribute_offset },
  { "mousepan-gain",             1, 0, 0, g_rc_mousepan_gain },
  { "keyboardpan-gain",          1, 0, 0, g_rc_keyboardpan_gain },
  { "select-slack-pixels",       1, 0, 0, g_rc_select_slack_pixels },
  { "zoom-gain",                 1, 0, 0, g_rc_zoom_gain },
  { "scrollpan-steps",           1, 0, 0, g_rc_scrollpan_steps },

  /* backup functions */
  { "auto-save-interval",        1, 0, 0, g_rc_auto_save_interval },

  /* general guile functions */
  { "gschem-pdf",                1, 0, 0, g_funcs_pdf },
  { "gschem-image",              1, 0, 0, g_funcs_image },
  { "gschem-use-rc-values",      0, 0, 0, g_funcs_use_rc_values },
  { "gschem-exit",               0, 0, 0, g_funcs_exit },
  { "gschem-log",                1, 0, 0, g_funcs_log },
  { "gschem-msg",                1, 0, 0, g_funcs_msg },
  { "gschem-confirm",            1, 0, 0, g_funcs_confirm },
  { "gschem-filesel",            2, 0, 1, g_funcs_filesel },

  { NULL,                        0, 0, 0, NULL } };

/*! \brief Define a hook.
 * \par Function Description
 * Creates a Guile new hook with \a n_args arguments, and binds it to
 * the variable \a name, returning the newly created hook.
 *
 * \param n_args Number of arguments the hook should take.
 * \param name   Name of variable to bind the hook to.
 *
 * \return the newly-created hook.
 */
static SCM
create_hook (const char *name, int n_args)
{
  SCM hook = scm_make_hook (scm_from_int (n_args));
  scm_c_define (name, hook);
  return scm_permanent_object (hook);
}

/*! \brief Register function with Scheme.
 *  \par Function Description
 *  Creates <B>subr</B> objects to make <B>g_rc_*</B> functions that are defined *  #g_rc.c and #g_funcs.c visible to Scheme.
 */
void g_register_funcs (void)
{
  struct gsubr_t *tmp = gschem_funcs;

  while (tmp->name != NULL) {
    scm_c_define_gsubr (tmp->name, tmp->req, tmp->opt, tmp->rst, tmp->fnc);
    tmp++;
  }

  /* Hook stuff */
  complex_place_list_changed_hook = create_hook ("complex-place-list-changed-hook", 1);
}
