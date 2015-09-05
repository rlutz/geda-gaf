/* gEDA - GPL Electronic Design Automation
 * gschem - gEDA Schematic Capture
 * Copyright (C) 1998-2010 Ales Hvezda
 * Copyright (C) 1998-2010 gEDA Contributors (see ChangeLog for details)
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
 * \file gschem_find_patch_widget.h
 *
 * \brief A widget for finding text
 */

#define GSCHEM_TYPE_FIND_PATCH_WIDGET           (gschem_find_patch_widget_get_type())
#define GSCHEM_FIND_PATCH_WIDGET(obj)           (G_TYPE_CHECK_INSTANCE_CAST ((obj), GSCHEM_TYPE_FIND_PATCH_WIDGET, GschemFindPatchWidget))
#define GSCHEM_FIND_PATCH_WIDGET_CLASS(klass)   (G_TYPE_CHECK_CLASS_CAST ((klass),  GSCHEM_TYPE_FIND_PATCH_WIDGET, GschemFindPatchWidgetClass))
#define GSCHEM_IS_FIND_PATCH_WIDGET(obj)        (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GSCHEM_TYPE_FIND_PATCH_WIDGET))
#define GSCHEM_FIND_PATCH_WIDGET_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), GSCHEM_TYPE_FIND_PATCH_WIDGET, GschemFindPatchWidgetClass))

typedef struct _GschemFindPatchWidgetClass GschemFindPatchWidgetClass;
typedef struct _GschemFindPatchWidget GschemFindPatchWidget;

struct _GschemFindPatchWidgetClass
{
  GtkInfoBarClass parent_class;
};

struct _GschemFindPatchWidget
{
  GtkInfoBar parent;

  GtkTreeModel *find_type_model;

  GtkWidget *label;
  GtkWidget *descend_button;
  GtkWidget *entry;
  GtkWidget *find_button;
};



int
gschem_find_patch_widget_get_descend (GschemFindPatchWidget *widget);

GtkWidget*
gschem_find_patch_widget_get_entry (GschemFindPatchWidget *widget);

const char*
gschem_find_patch_widget_get_find_patch_string (GschemFindPatchWidget *widget);

GType
gschem_find_patch_widget_get_type ();

void
gschem_find_patch_widget_set_descend (GschemFindPatchWidget *widget, int descend);

void
gschem_find_patch_widget_set_find_patch_string (GschemFindPatchWidget *widget, const char *str);

void
gschem_find_patch_widget_set_find_type (GschemFindPatchWidget *widget, int type);
