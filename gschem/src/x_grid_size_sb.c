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

/*! \file x_grid_size_sb.c
 * \brief Spinbox for grid size selection.
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

static void
update_spin_button_internal (GtkSpinButton *spin_button, gdouble value);


static gboolean
is_grid_step (gint v)
{
  return /* base 10 */
         v == 5 || v == 10 || v == 20 || v == 40 || v == 80 || v == 160 ||
         v == 320 || v == 640 || v == 1280 || v == 2560 || v == 5120 ||
         v == 10240 || v == 20480 || v == 40960 || v == 81920 ||

         /* base 100 (default) */
         v == 25 || v == 50 || v == 100 || v == 200 || v == 400 || v == 800 ||
         v == 1600 || v == 3200 || v == 6400 || v == 12800 || v == 25600 ||
         v == 51200 ||

         /* base 1000 */
         v == 125 || v == 250 || v == 500 || v == 1000 || v == 2000 ||
         v == 4000 || v == 8000 || v == 16000 || v == 32000 || v == 64000;
}

static void
spin_button_value_changed (GtkSpinButton *spin_button,
                           GschemToplevel *w_current)
{
  gint value = gtk_spin_button_get_value_as_int (spin_button);
  gint prev_value = gschem_options_get_snap_size (w_current->options);

  /* Gtk doesn't allow handling the increment/decrement events
     directly, so we have to observe the new value and guess whether
     they are the result of pressing up/down on the previous value. */
  if (!is_grid_step (value) && is_grid_step (prev_value)) {
    if (value == prev_value - 1) {
      if (prev_value % 2 == 0 && is_grid_step (prev_value / 2))
        value = prev_value / 2;
      else
        value = prev_value;
    } else if (value == prev_value + 1) {
      if (is_grid_step (prev_value * 2))
        value = prev_value * 2;
      else
        value = prev_value;
    }
  }

  update_spin_button_internal (spin_button, value);
  gschem_options_set_snap_size (w_current->options, value);
}

static void
update_spin_button (GschemOptions *options,
                    GParamSpec *pspec,
                    gpointer user_data)
{
  update_spin_button_internal (GTK_SPIN_BUTTON (user_data),
                               gschem_options_get_snap_size (options));
}

static void
update_spin_button_internal (GtkSpinButton *spin_button,
                             gdouble value)
{
  g_signal_handlers_block_matched (
    spin_button, G_SIGNAL_MATCH_FUNC, 0, 0, NULL,
    G_CALLBACK (spin_button_value_changed), NULL);

  gtk_spin_button_set_value (spin_button, value);

  g_signal_handlers_unblock_matched (
    spin_button, G_SIGNAL_MATCH_FUNC, 0, 0, NULL,
    G_CALLBACK (spin_button_value_changed), NULL);
}

/*! \brief Create grid size spinbox.
 */
GtkWidget *
x_grid_size_sb_new (GschemToplevel *w_current)
{
  GtkObject *adjustment = gtk_adjustment_new (1, MINIMUM_SNAP_SIZE,
                                                 MAXIMUM_SNAP_SIZE, 1, 1, 0);
  GtkWidget *spin_button = g_object_new (GTK_TYPE_SPIN_BUTTON, NULL);
  gtk_spin_button_set_adjustment (GTK_SPIN_BUTTON (spin_button),
                                  GTK_ADJUSTMENT (adjustment));
  gtk_spin_button_set_value (GTK_SPIN_BUTTON (spin_button),
                             gschem_options_get_snap_size (w_current->options));
  gtk_entry_set_width_chars (GTK_ENTRY (spin_button), 5);

  g_signal_connect (spin_button, "value-changed",
                    G_CALLBACK (spin_button_value_changed), w_current);
  g_signal_connect (w_current->options, "notify::snap-size",
                    G_CALLBACK (update_spin_button), spin_button);

  return spin_button;
}
