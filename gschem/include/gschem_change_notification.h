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

#ifndef GSCHEM_CHANGE_NOTIFICATION_H
#define GSCHEM_CHANGE_NOTIFICATION_H

#define GSCHEM_TYPE_TIMESPEC (gschem_timespec_get_type ())
#define GSCHEM_TIMESPEC(obj) ((GschemTimespec *) (obj))

#define GSCHEM_TYPE_CHANGE_NOTIFICATION           (gschem_change_notification_get_type ())
#define GSCHEM_CHANGE_NOTIFICATION(obj)           (G_TYPE_CHECK_INSTANCE_CAST ((obj), GSCHEM_TYPE_CHANGE_NOTIFICATION, GschemChangeNotification))
#define GSCHEM_CHANGE_NOTIFICATION_CLASS(klass)   (G_TYPE_CHECK_CLASS_CAST ((klass), GSCHEM_TYPE_CHANGE_NOTIFICATION, GschemChangeNotificationClass))
#define GSCHEM_IS_CHANGE_NOTIFICATION(obj)        (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GSCHEM_TYPE_CHANGE_NOTIFICATION))
#define GSCHEM_CHANGE_NOTIFICATION_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), GSCHEM_TYPE_CHANGE_NOTIFICATION, GschemChangeNotificationClass))

typedef struct timespec GschemTimespec;

typedef struct _GschemChangeNotificationClass GschemChangeNotificationClass;
typedef struct _GschemChangeNotification      GschemChangeNotification;

struct _GschemChangeNotificationClass {
  GObjectClass parent_class;
};

struct _GschemChangeNotification {
  GObject parent_instance;

  /* convenience properties--not internally used */
  GschemToplevel *w_current;
  PAGE *page;

  gchar *path;
  gpointer fam_handle;
  gboolean has_known_mtime, has_current_mtime;
  struct timespec known_mtime, current_mtime;

  GtkWidget *info_bar, *label, *button;
};

GType gschem_timespec_get_type (void);
GType gschem_change_notification_get_type (void);

#endif /* GSCHEM_CHANGE_NOTIFICATION_H */
