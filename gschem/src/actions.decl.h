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

/*! \file actions.decl.h
 *  \brief Generated action declarations.
 *
 * This file is preprocessed into actions.decl.x which contains
 * variable declarations for the individual actions defined in
 * actions.c.  The resulting file looks like this:
 *
 *     extern GschemAction *action_file_new;
 *     extern GschemAction *action_file_new_window;
 *     extern GschemAction *action_file_open;
 *     ...
 *
 * Source files which reference individual actions by name (e.g. in
 * order to change their sensitivity) should include this file as
 *
 *     #include "actions.decl.x"
 *
 * Don't include the un-processed file directly--this won't work.
 */

#define DEFINE_ACTION(c_id, id, icon, name, label, menu_label, tooltip, type) \
  KEEP_LINE extern GschemAction *action_ ## c_id;
#include "actions.c"
#undef DEFINE_ACTION
