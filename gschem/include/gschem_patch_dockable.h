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

#ifndef GSCHEM_PATCH_DOCKABLE_H
#define GSCHEM_PATCH_DOCKABLE_H

#define GSCHEM_TYPE_PATCH_DOCKABLE           (gschem_patch_dockable_get_type ())
#define GSCHEM_PATCH_DOCKABLE(obj)           (G_TYPE_CHECK_INSTANCE_CAST ((obj), GSCHEM_TYPE_PATCH_DOCKABLE, GschemPatchDockable))
#define GSCHEM_PATCH_DOCKABLE_CLASS(klass)   (G_TYPE_CHECK_CLASS_CAST ((klass), GSCHEM_TYPE_PATCH_DOCKABLE, GschemPatchDockableClass))
#define GSCHEM_IS_PATCH_DOCKABLE(obj)        (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GSCHEM_TYPE_PATCH_DOCKABLE))
#define GSCHEM_PATCH_DOCKABLE_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), GSCHEM_TYPE_PATCH_DOCKABLE, GschemPatchDockableClass))

typedef struct _GschemPatchDockableClass GschemPatchDockableClass;
typedef struct _GschemPatchDockable      GschemPatchDockable;

struct _GschemPatchDockableClass {
  GschemDockableClass parent_class;
};

struct _GschemPatchDockable {
  GschemDockable parent;

  GtkListStore *store;
};

/* functions are defined in prototype.h */

GType
gschem_patch_dockable_get_type (void);

#endif /* GSCHEM_PATCH_DOCKABLE_H */
