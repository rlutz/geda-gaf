/* gEDA - GPL Electronic Design Automation
 * gschem - gEDA Schematic Capture
 * Copyright (C) 2013 Ales Hvezda
 * Copyright (C) 2013-2020 gEDA Contributors (see ChangeLog for details)
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
 * \file gschem_options_dockable.h
 *
 * \brief A dockable for editing options
 */

#define GSCHEM_TYPE_OPTIONS_DOCKABLE           (gschem_options_dockable_get_type())
#define GSCHEM_OPTIONS_DOCKABLE(obj)           (G_TYPE_CHECK_INSTANCE_CAST ((obj), GSCHEM_TYPE_OPTIONS_DOCKABLE, GschemOptionsDockable))
#define GSCHEM_OPTIONS_DOCKABLE_CLASS(klass)   (G_TYPE_CHECK_CLASS_CAST ((klass),  GSCHEM_TYPE_OPTIONS_DOCKABLE, GschemOptionsDockableClass))
#define GSCHEM_IS_OPTIONS_DOCKABLE(obj)        (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GSCHEM_TYPE_OPTIONS_DOCKABLE))
#define GSCHEM_OPTIONS_DOCKABLE_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj),  GSCHEM_TYPE_OPTIONS_DOCKABLE, GschemOptionsDockableClass))

typedef struct _GschemOptionsDockableClass GschemOptionsDockableClass;
typedef struct _GschemOptionsDockable GschemOptionsDockable;

struct _GschemOptionsDockableClass {
  GschemDockableClass parent_class;
};

struct _GschemOptionsDockable {
  GschemDockable parent;

  GschemOptions *options;

  GtkSizeGroup *size_group;

  GtkWidget *magnetic_net_widget;
  GtkWidget *net_rubber_band_widget;

  GtkWidget *grid_radio[GRID_MODE_COUNT];
  GtkWidget *snap_radio[SNAP_STATE_COUNT];
};

GType
gschem_options_dockable_get_type ();
