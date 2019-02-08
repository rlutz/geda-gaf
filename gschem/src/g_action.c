/* gEDA - GPL Electronic Design Automation
 * gschem - gEDA Schematic Capture
 * Copyright (C) 2013 Peter Brett <peter@peter-b.co.uk>
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111 USA
 */

/*! \file g_action.c
 * \brief Functions relating to working with gschem actions.
 */

#include <config.h>

#include "gschem.h"

/*! \brief Get the action position.
 * \par Function Description
 * Retrieves the current action position and stores it in \a x and \a
 * y, optionally snapping it to the grid if \a snap is true.  This
 * should be interpreted as the position that the user was pointing
 * with the mouse pointer when the current action was invoked.  If
 * there is no valid world position for the current action, returns
 * FALSE without modifying the output variables.
 *
 * This should be used by actions implemented in C to figure out where
 * on the schematic the user wants them to apply the action.
 *
 * See also the (gschem action) Scheme module.
 *
 * \param w_current    Current gschem toplevel structure.
 * \param x            Location to store x coordinate.
 * \param y            Location to store y coordinate.
 *
 * \return TRUE if current action position is set, FALSE otherwise.
 */
gboolean
g_action_get_position (gboolean snap, int *x, int *y)
{
  SCM s_action_position_proc;
  SCM s_point;
  GschemToplevel *w_current = g_current_window ();

  g_assert (w_current);

  /* Get the action-position procedure */
  s_action_position_proc =
	  scm_variable_ref (scm_c_public_variable ("gschem action",
	                                           "action-position"));

  /* Retrieve the action position */
  s_point = scm_call_0 (s_action_position_proc);

  if (scm_is_false (s_point)) return FALSE;

  if (x) {
    *x = scm_to_int (scm_car (s_point));
    if (snap) {
      *x = snap_grid (w_current, *x);
    }
  }
  if (y) {
    *y = scm_to_int (scm_cdr (s_point));
    if (snap) {
      *y = snap_grid (w_current, *y);
    }
  }

  return TRUE;
}
