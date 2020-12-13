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



#define GSCHEM_TYPE_PAGESEL_DOCKABLE         (gschem_pagesel_dockable_get_type())
#define GSCHEM_PAGESEL_DOCKABLE(obj)         (G_TYPE_CHECK_INSTANCE_CAST ((obj), GSCHEM_TYPE_PAGESEL_DOCKABLE, GschemPageselDockable))
#define GSCHEM_PAGESEL_DOCKABLE_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass), GSCHEM_TYPE_PAGESEL_DOCKABLE, GschemPageselDockableClass))
#define GSCHEM_IS_PAGESEL_DOCKABLE(obj)      (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GSCHEM_TYPE_PAGESEL_DOCKABLE))


typedef struct _GschemPageselDockableClass GschemPageselDockableClass;
typedef struct _GschemPageselDockable      GschemPageselDockable;


struct _GschemPageselDockableClass {
  GschemDialogClass parent_class;
};

struct _GschemPageselDockable {
  GschemDialog parent_instance;

  GtkTreeView *treeview;
};


GType gschem_pagesel_dockable_get_type (void);
