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
/*!
 * \file gschem_options_dockable.c
 *
 * \brief A dockable for editing options
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

#include "../include/gschem_options_dockable.h"


static void
class_init (GschemOptionsDockableClass *klass);

GtkWidget*
create_grid_mode_widget (GschemOptionsDockable *dockable);

GtkWidget*
create_snap_mode_widget (GschemOptionsDockable *dockable);

static GtkWidget*
create_net_section (GschemOptionsDockable *dockable);

static GtkWidget*
create_snap_section (GschemOptionsDockable *dockable);

static void
dispose (GObject *object);

static GtkWidget *
create_widget (GschemDockable *parent);

static void
set_options (GschemOptionsDockable *dockable, GschemOptions *options);

static void
update_grid_mode_model (GschemOptionsDockable *dockable, GtkWidget *button);

static void
update_grid_mode_widget (GschemOptionsDockable *dockable);

static void
update_magnetic_net_mode_model (GschemOptionsDockable *dockable);

static void
update_magnetic_net_mode_widget (GschemOptionsDockable *dockable);

static void
update_net_rubber_band_mode_model (GschemOptionsDockable *dockable);

static void
update_net_rubber_band_mode_widget (GschemOptionsDockable *dockable);

static void
update_snap_mode_model (GschemOptionsDockable *dockable, GtkWidget *button);

static void
update_snap_mode_widget (GschemOptionsDockable *dockable);



/*! \brief Get/register options dockable type.
 */
GType
gschem_options_dockable_get_type ()
{
  static GType type = 0;

  if (type == 0) {
    static const GTypeInfo info = {
      sizeof(GschemOptionsDockableClass),
      NULL,                                       /* base_init */
      NULL,                                       /* base_finalize */
      (GClassInitFunc) class_init,
      NULL,                                       /* class_finalize */
      NULL,                                       /* class_data */
      sizeof(GschemOptionsDockable),
      0,                                          /* n_preallocs */
      NULL,                                       /* instance_init */
    };

    type = g_type_register_static (GSCHEM_TYPE_DOCKABLE,
                                   "GschemOptionsDockable",
                                   &info,
                                   0);
  }

  return type;
}



/*! \private
 *  \brief Initialize the options dockable class structure
 *
 *  \param [in] klass
 */
static void
class_init (GschemOptionsDockableClass *klass)
{
  GObjectClass *object_class;

  g_return_if_fail (klass != NULL);

  GSCHEM_DOCKABLE_CLASS (klass)->create_widget = create_widget;

  object_class = G_OBJECT_CLASS (klass);

  g_return_if_fail (object_class != NULL);

  object_class->dispose = dispose;
}



/*! \private
 *  \brief Create the series of buttons that make the grid mode selection widget
 *
 *  \param [in] dockable
 *  \return The grid mode widget
 */
GtkWidget*
create_grid_mode_widget (GschemOptionsDockable *dockable)
{
  GtkWidget *box;
  int index;

  g_return_val_if_fail (dockable != NULL, NULL);

  box = gtk_hbox_new (FALSE, FALSE);

  for (index=0; index<GRID_MODE_COUNT; index++) {
    dockable->grid_radio[index] = gtk_toggle_button_new ();

    gtk_box_pack_start (GTK_BOX (box),                           /* box     */
                        dockable->grid_radio[index],             /* child   */
                        FALSE,                                   /* expand  */
                        FALSE,                                   /* fill    */
                        0);                                      /* padding */

    gtk_size_group_add_widget (dockable->size_group, dockable->grid_radio[index]);

    g_signal_connect_swapped (G_OBJECT (dockable->grid_radio[index]),
                              "clicked",
                              G_CALLBACK (update_grid_mode_model),
                              dockable);
  }

  gtk_button_set_label (GTK_BUTTON (dockable->grid_radio[GRID_MODE_NONE]),
                        _("Off"));

  gtk_button_set_label (GTK_BUTTON (dockable->grid_radio[GRID_MODE_DOTS]),
                        _("Dots"));

  gtk_button_set_label (GTK_BUTTON (dockable->grid_radio[GRID_MODE_MESH]),
                        _("Mesh"));

  return box;
}



/*! \private
 *  \brief Create section with net tool settings
 *
 *  \param [in] dockable
 *  \return The net section widget
 */
static GtkWidget*
create_net_section (GschemOptionsDockable *dockable)
{
  GtkWidget *label[2];
  GtkWidget *table;
  GtkWidget *editor[2];

  /* These widgets are shown in the same order as the options menu */

  label[0] = gschem_dialog_misc_create_property_label (_("Net Rubber Band Mode:"));
  label[1] = gschem_dialog_misc_create_property_label (_("Magnetic Net Mode:"));

  /*! \todo These should become a GtkSwitch when updating to GTK 3.0 */

  editor[0] = dockable->net_rubber_band_widget = gtk_check_button_new_with_label (_("Enabled"));
  editor[1] = dockable->magnetic_net_widget = gtk_check_button_new_with_label (_("Enabled"));

  table = gschem_dialog_misc_create_property_table (label, editor, 2);

  g_signal_connect_swapped (G_OBJECT (dockable->magnetic_net_widget),
                            "toggled",
                            G_CALLBACK (update_magnetic_net_mode_model),
                            dockable);

  g_signal_connect_swapped (G_OBJECT (dockable->net_rubber_band_widget),
                            "toggled",
                            G_CALLBACK (update_net_rubber_band_mode_model),
                            dockable);

  return gschem_dialog_misc_create_section_widget (_("<b>Net Options</b>"), table);
}


/*! \private
 *  \brief Create section with snap and grid settings
 *
 *  \param [in] dockable
 *  \return The snap section widget
 */
static GtkWidget*
create_snap_section (GschemOptionsDockable *dockable)
{
  GtkWidget *label[3];
  GtkWidget *table;
  GtkWidget *editor[3];

  label[0] = gschem_dialog_misc_create_property_label (_("Grid Mode:"));
  label[1] = gschem_dialog_misc_create_property_label (_("Snap Mode:"));
  label[2] = gschem_dialog_misc_create_property_label (_("Grid Spacing:"));

  editor[0] = create_grid_mode_widget (dockable);
  editor[1] = create_snap_mode_widget (dockable);
  editor[2] = x_grid_size_sb_new (GSCHEM_DOCKABLE (dockable)->w_current);

  table = gschem_dialog_misc_create_property_table (label, editor, 3);

  return gschem_dialog_misc_create_section_widget (_("<b>Grid Options</b>"), table);
}



/*! \private
 *  \brief Create the series of buttons that make the snap mode selection widget
 *
 *  \param [in] dockable
 *  \return The snap mode widget
 */
GtkWidget*
create_snap_mode_widget (GschemOptionsDockable *dockable)
{
  GtkWidget *box;
  int index;

  g_return_val_if_fail (dockable != NULL, NULL);

  box = gtk_hbox_new (FALSE, FALSE);

  for (index=0; index<SNAP_STATE_COUNT; index++) {
    dockable->snap_radio[index] = gtk_toggle_button_new ();

    gtk_box_pack_start (GTK_BOX (box),                           /* box     */
                        dockable->snap_radio[index],             /* child   */
                        FALSE,                                   /* expand  */
                        FALSE,                                   /* fill    */
                        0);                                      /* padding */

    gtk_size_group_add_widget (dockable->size_group, dockable->snap_radio[index]);

    g_signal_connect_swapped (G_OBJECT (dockable->snap_radio[index]),
                              "clicked",
                              G_CALLBACK (update_snap_mode_model),
                              dockable);
  }

  gtk_button_set_label (GTK_BUTTON (dockable->snap_radio[SNAP_OFF]),
                        _("Off"));

  gtk_button_set_label (GTK_BUTTON (dockable->snap_radio[SNAP_GRID]),
                        _("Grid"));

  gtk_button_set_label (GTK_BUTTON (dockable->snap_radio[SNAP_RESNAP]),
                        _("Resnap"));

  return box;
}


/*! \private
 *  \brief Dispose
 *
 *  \param [in] object The options dockable to dispose
 */
static void
dispose (GObject *object)
{
  GschemOptionsDockable *dockable;
  GschemOptionsDockableClass *klass;
  GObjectClass *parent_class;

  g_return_if_fail (object != NULL);

  dockable = GSCHEM_OPTIONS_DOCKABLE (object);

  set_options (dockable, NULL);

  //g_slist_foreach (dockable->bindings, (GFunc) g_object_unref, NULL);
  //g_slist_free (dockable->bindings);
  //dockable->bindings = NULL;

  /* lastly, chain up to the parent dispose */

  klass = GSCHEM_OPTIONS_DOCKABLE_GET_CLASS (object);
  g_return_if_fail (klass != NULL);
  parent_class = g_type_class_peek_parent (klass);
  g_return_if_fail (parent_class != NULL);
  parent_class->dispose (object);
}


/*! \private
 *  \brief Create widgets and set up property change notifications
 *
 *  \param [in,out] parent This dockable
 */
static GtkWidget *
create_widget (GschemDockable *parent)
{
  GschemOptionsDockable *dockable = GSCHEM_OPTIONS_DOCKABLE (parent);
  GtkWidget *vbox;

  dockable->size_group = gtk_size_group_new (GTK_SIZE_GROUP_BOTH);

  vbox = gtk_vbox_new (FALSE, DIALOG_V_SPACING);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), DIALOG_BORDER_SPACING);

  gtk_box_pack_start (GTK_BOX (vbox),                          /* box     */
                      create_snap_section (dockable),          /* child   */
                      FALSE,                                   /* expand  */
                      FALSE,                                   /* fill    */
                      0);                                      /* padding */

  gtk_box_pack_start (GTK_BOX (vbox),                          /* box     */
                      create_net_section (dockable),           /* child   */
                      FALSE,                                   /* expand  */
                      FALSE,                                   /* fill    */
                      0);                                      /* padding */

  set_options (dockable, parent->w_current->options);

  gtk_widget_show_all (vbox);
  return vbox;
}



/*! \private
 *  \brief Set the options that this dockable manipulates
 *
 *  \param [in,out] dockable This dockable
 *  \param [in]     options  The options to manipulate
 */
static void
set_options (GschemOptionsDockable *dockable, GschemOptions *options)
{
  if (dockable->options != NULL) {
    g_signal_handlers_disconnect_by_func (dockable->options,
                                          G_CALLBACK (update_snap_mode_widget),
                                          dockable);

    g_signal_handlers_disconnect_by_func (dockable->options,
                                          G_CALLBACK (update_net_rubber_band_mode_widget),
                                          dockable);

    g_signal_handlers_disconnect_by_func (dockable->options,
                                          G_CALLBACK (update_magnetic_net_mode_widget),
                                          dockable);

    g_signal_handlers_disconnect_by_func (dockable->options,
                                          G_CALLBACK (update_grid_mode_widget),
                                          dockable);

    g_object_unref (dockable->options);
  }

  dockable->options = options;

  if (dockable->options != NULL) {
    g_object_ref (dockable->options);

    g_signal_connect_swapped (dockable->options,
                              "notify::grid-mode",
                              G_CALLBACK (update_grid_mode_widget),
                              dockable);

    g_signal_connect_swapped (dockable->options,
                              "notify::magnetic-net-mode",
                              G_CALLBACK (update_magnetic_net_mode_widget),
                              dockable);

    g_signal_connect_swapped (dockable->options,
                              "notify::net-rubber-band-mode",
                              G_CALLBACK (update_net_rubber_band_mode_widget),
                              dockable);

    g_signal_connect_swapped (dockable->options,
                              "notify::snap-mode",
                              G_CALLBACK (update_snap_mode_widget),
                              dockable);
  }

  update_grid_mode_widget (dockable);
  update_magnetic_net_mode_widget (dockable);
  update_net_rubber_band_mode_widget (dockable);
  update_snap_mode_widget (dockable);
}


/*! \private
 *  \brief Update the grid mode in the model
 *
 *  \param [in,out] dockable This dockable
 */
static void
update_grid_mode_model (GschemOptionsDockable *dockable, GtkWidget *button)
{
  g_return_if_fail (dockable != NULL);

  if (dockable->options != NULL) {
    int index;

    for (index = 0; index < GRID_MODE_COUNT; index++) {
      if (dockable->grid_radio[index] == button) {
        gschem_options_set_grid_mode (dockable->options, index);
        break;
      }
    }
  }
}



/*! \private
 *  \brief Update the grid mode widget with the current value
 *
 *  \param [in,out] dockable This dockable
 */
static void
update_grid_mode_widget (GschemOptionsDockable *dockable)
{
  g_return_if_fail (dockable != NULL);

  if (dockable->options != NULL) {
    GRID_MODE grid_mode;
    int index;

    grid_mode = gschem_options_get_grid_mode (dockable->options);

    for (index=0; index<GRID_MODE_COUNT; index++) {
      g_signal_handlers_block_by_func (G_OBJECT (dockable->grid_radio[index]),
                                       G_CALLBACK (update_grid_mode_model),
                                       dockable);
    }

    for (index=0; index<GRID_MODE_COUNT; index++) {
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (dockable->grid_radio[index]),
                                    (grid_mode == index));
    }

    for (index=0; index<GRID_MODE_COUNT; index++) {
      g_signal_handlers_unblock_by_func (G_OBJECT (dockable->grid_radio[index]),
                                         G_CALLBACK (update_grid_mode_model),
                                         dockable);
    }
  }
}



/*! \private
 *  \brief Update the magnetic net mode in the model
 *
 *  \param [in,out] dockable This dockable
 */
static void
update_magnetic_net_mode_model (GschemOptionsDockable *dockable)
{
  GschemToplevel *w_current;

  g_return_if_fail (dockable != NULL);

  g_object_get (dockable, "gschem-toplevel", &w_current, NULL);

  g_return_if_fail (w_current != NULL);

  gschem_options_set_magnetic_net_mode (w_current->options,
                                        gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (dockable->magnetic_net_widget)));
}



/*! \private
 *  \brief Update the net rubber band mode widget with the current value
 *
 *  \param [in,out] dockable This dockable
 */
static void
update_magnetic_net_mode_widget (GschemOptionsDockable *dockable)
{
  g_return_if_fail (dockable != NULL);

  if (dockable->options != NULL) {
    GschemToplevel *w_current;

    g_object_get (dockable, "gschem-toplevel", &w_current, NULL);

    g_return_if_fail (w_current != NULL);

    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (dockable->magnetic_net_widget),
                                  gschem_options_get_magnetic_net_mode (w_current->options));
  }
}


/*! \private
 *  \brief Update the net rubber band mode in the model
 *
 *  \param [in,out] dockable This dockable
 */
static void
update_net_rubber_band_mode_model (GschemOptionsDockable *dockable)
{
  GschemToplevel *w_current;

  g_return_if_fail (dockable != NULL);

  g_object_get (dockable, "gschem-toplevel", &w_current, NULL);

  g_return_if_fail (w_current != NULL);

  gschem_options_set_net_rubber_band_mode (w_current->options,
                                           gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (dockable->net_rubber_band_widget)));
}



/*! \private
 *  \brief Update the net rubber band mode widget with the current value
 *
 *  \param [in,out] dockable This dockable
 */
static void
update_net_rubber_band_mode_widget (GschemOptionsDockable *dockable)
{
  g_return_if_fail (dockable != NULL);

  if (dockable->options != NULL) {
    GschemToplevel *w_current;

    g_object_get (dockable, "gschem-toplevel", &w_current, NULL);

    g_return_if_fail (w_current != NULL);

    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (dockable->net_rubber_band_widget),
                                  gschem_options_get_net_rubber_band_mode (w_current->options));
  }
}



/*! \private
 *  \brief Update the snap mode in the model
 *
 *  \param [in,out] dockable This dockable
 */
static void
update_snap_mode_model (GschemOptionsDockable *dockable, GtkWidget *button)
{
  g_return_if_fail (dockable != NULL);

  if (dockable->options != NULL) {
    int index;

    for (index = 0; index < SNAP_STATE_COUNT; index++) {
      if (dockable->snap_radio[index] == button) {
        gschem_options_set_snap_mode (dockable->options, index);
        break;
      }
    }
  }
}



/*! \private
 *  \brief Update the snap mode widget with the current value
 *
 *  \param [in,out] dockable This dockable
 */
static void
update_snap_mode_widget (GschemOptionsDockable *dockable)
{
  g_return_if_fail (dockable != NULL);

  if (dockable->options != NULL) {
    int index;
    SNAP_STATE snap_mode;

    snap_mode = gschem_options_get_snap_mode (dockable->options);

    for (index=0; index<SNAP_STATE_COUNT; index++) {
      g_signal_handlers_block_by_func (G_OBJECT (dockable->snap_radio[index]),
                                       G_CALLBACK (update_snap_mode_model),
                                       dockable);
    }

    for (index=0; index<SNAP_STATE_COUNT; index++) {
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (dockable->snap_radio[index]),
                                    (snap_mode == index));
  }

    for (index=0; index<SNAP_STATE_COUNT; index++) {
      g_signal_handlers_unblock_by_func (G_OBJECT (dockable->snap_radio[index]),
                                         G_CALLBACK (update_snap_mode_model),
                                         dockable);
    }
  }
}
