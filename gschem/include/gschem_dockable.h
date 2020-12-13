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

#ifndef GSCHEM_DOCKABLE_H
#define GSCHEM_DOCKABLE_H

#define GSCHEM_TYPE_DOCKABLE_STATE     (gschem_dockable_state_get_type ())

#define GSCHEM_TYPE_DOCKABLE           (gschem_dockable_get_type ())
#define GSCHEM_DOCKABLE(obj)           (G_TYPE_CHECK_INSTANCE_CAST ((obj), GSCHEM_TYPE_DOCKABLE, GschemDockable))
#define GSCHEM_DOCKABLE_CLASS(klass)   (G_TYPE_CHECK_CLASS_CAST ((klass), GSCHEM_TYPE_DOCKABLE, GschemDockableClass))
#define GSCHEM_IS_DOCKABLE(obj)        (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GSCHEM_TYPE_DOCKABLE))
#define GSCHEM_DOCKABLE_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), GSCHEM_TYPE_DOCKABLE, GschemDockableClass))

typedef enum {
  GSCHEM_DOCKABLE_STATE_HIDDEN,
  GSCHEM_DOCKABLE_STATE_DIALOG,
  GSCHEM_DOCKABLE_STATE_WINDOW,
  GSCHEM_DOCKABLE_STATE_DOCKED_LEFT,
  GSCHEM_DOCKABLE_STATE_DOCKED_BOTTOM,
  GSCHEM_DOCKABLE_STATE_DOCKED_RIGHT
} GschemDockableState;

typedef struct _GschemDockableClass GschemDockableClass;
typedef struct _GschemDockable      GschemDockable;

struct _GschemDockableClass {
  GObjectClass parent_class;

  GtkWidget *(*create_widget) (GschemDockable *dockable);
  void (*post_present) (GschemDockable *dockable);
  void (*cancel) (GschemDockable *dockable);

  void (*save_internal_geometry)    (GschemDockable *dockable,
                                     EdaConfig *cfg,
                                     gchar *group_name);
  void (*restore_internal_geometry) (GschemDockable *dockable,
                                     EdaConfig *cfg,
                                     gchar *group_name);
};

struct _GschemDockable {
  GObject parent_instance;

  GschemToplevel *w_current;

  gchar *title;
  gchar *settings_name, *group_name;
  gboolean cancellable;
  gchar *help_page;

  GschemDockableState initial_state;
  gint initial_width, initial_height;
  GtkWindowPosition initial_position;

  GtkWidget *widget, *window, *vbox, *action_area;
  GtkWidget *hide_button, *cancel_button, *help_button;

  GtkWidget *dock_left_item, *dock_bottom_item, *dock_right_item;
  GtkWidget *menu, *detach_item, *move_left_item, *move_bottom_item,
                   *move_right_item, *close_item;
};

GType gschem_dockable_state_get_type (void);
GType gschem_dockable_get_type (void);

void gschem_dockable_hide (GschemDockable *dockable);
void gschem_dockable_detach (GschemDockable *dockable,
                             gboolean show_action_area);
void gschem_dockable_attach_to_notebook (GschemDockable *dockable,
                                         GtkWidget *notebook);

GschemDockableState gschem_dockable_get_state (GschemDockable *dockable);
void gschem_dockable_set_state (GschemDockable *dockable,
                                GschemDockableState state);

void gschem_dockable_present (GschemDockable *dockable);


void gschem_dockable_initialize_toplevel (GschemToplevel *w_current);
void gschem_dockable_cleanup_toplevel (GschemToplevel *w_current);

#endif /* GSCHEM_DOCKABLE_H */
