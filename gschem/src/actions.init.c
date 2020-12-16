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
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

/*! \file actions.init.c
 *  \brief Generated action initializations.
 *
 * This file is preprocessed into actions.init.x which contains a call
 * to \ref gschem_action_register for each individual action defined
 * in actions.c.  The resulting file looks like this:
 *
 *     action_file_new = gschem_action_register (
 *                         "file-new",
 *                         "gtk-new",
 *                         _("New File"),
 *                         _("New"),
 *                         _("_New"),
 *                         NULL,
 *                         GSCHEM_ACTION_TYPE_ACTUATE,
 *                         action_callback_file_new);
 *     ...
 *
 * actions.init.x should only be included once in the definition of
 * \ref init_module_gschem_core_builtins.
 */

#define DEFINE_ACTION(c_id, id, icon, name, label, menu_label, tooltip, type) \
  KEEP_LINE                                                                   \
  action_ ## c_id =                                                           \
    gschem_action_register (id,                                               \
                            icon,                                             \
                            name,                                             \
                            label,                                            \
                            menu_label,                                       \
                            tooltip,                                          \
                            GSCHEM_ACTION_TYPE_ ## type,                      \
                            action_callback_ ## c_id);
#include "actions.c"
#undef DEFINE_ACTION
