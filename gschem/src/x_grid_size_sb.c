/* gEDA - GPL Electronic Design Automation
 * gschem - gEDA Schematic Capture
 * Copyright (C) 1998-2010 Ales Hvezda
 * Copyright (C) 1998-2019 gEDA Contributors (see ChangeLog for details)
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
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
#include <math.h>

#include "gschem.h"


static gint
spin_button_input (GtkSpinButton *spin_button,
                   gdouble *new_value,
                   GschemToplevel *w_current)
{
  gchar *err = NULL;
  gdouble num = g_strtod (gtk_entry_get_text (GTK_ENTRY (spin_button)), &err);
  if (*err)
    return GTK_INPUT_ERROR;
  *new_value = round (log2 (num / 25));
  if (*new_value < 0)
    *new_value = 0;
  if (*new_value > 12)
    *new_value = 12;
  return TRUE;
}

static gint
spin_button_output (GtkSpinButton *spin_button,
                    GschemToplevel *w_current)
{
  gint value = gtk_spin_button_get_value_as_int (spin_button);
  gchar *buf = g_strdup_printf ("%.0f", 25 * exp2 (value));
  if (strcmp (buf, gtk_entry_get_text (GTK_ENTRY (spin_button))) != 0)
    gtk_entry_set_text (GTK_ENTRY (spin_button), buf);
  g_free (buf);
  return TRUE;
}

static void
spin_button_value_changed (GtkSpinButton *spin_button,
                           GschemToplevel *w_current)
{
  gint value = gtk_spin_button_get_value_as_int (spin_button);
  gschem_options_set_snap_size (w_current->options, 25 * exp2 (value));
}

static void
update_spin_button (GschemOptions *options,
                    GParamSpec *pspec,
                    gpointer user_data)
{
  gdouble value = round (log2 (gschem_options_get_snap_size (options) / 25));

  g_signal_handlers_block_matched (
    GTK_WIDGET (user_data), G_SIGNAL_MATCH_FUNC, 0, 0, NULL,
    G_CALLBACK (spin_button_value_changed), NULL);

  gtk_spin_button_set_value (GTK_SPIN_BUTTON (user_data), value);

  g_signal_handlers_unblock_matched (
    GTK_WIDGET (user_data), G_SIGNAL_MATCH_FUNC, 0, 0, NULL,
    G_CALLBACK (spin_button_value_changed), NULL);
}

/*! \brief Create grid size spinbox.
 */
GtkWidget *
x_grid_size_sb_new (GschemToplevel *w_current)
{
  GtkObject *adjustment = gtk_adjustment_new (2, 0, 12, 1, 0, 0);
  GtkWidget *spin_button = g_object_new (GTK_TYPE_SPIN_BUTTON, NULL);
  gtk_spin_button_set_adjustment (GTK_SPIN_BUTTON (spin_button),
                                  GTK_ADJUSTMENT (adjustment));
  gtk_spin_button_set_update_policy (GTK_SPIN_BUTTON (spin_button),
                                     GTK_UPDATE_IF_VALID);

  g_signal_connect (spin_button, "input",
                    G_CALLBACK (spin_button_input), w_current);
  g_signal_connect (spin_button, "output",
                    G_CALLBACK (spin_button_output), w_current);
  g_signal_connect (spin_button, "value-changed",
                    G_CALLBACK (spin_button_value_changed), w_current);
  g_signal_connect (w_current->options, "notify::snap-size",
                    G_CALLBACK (update_spin_button), spin_button);

  return spin_button;
}
