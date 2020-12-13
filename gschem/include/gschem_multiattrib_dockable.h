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


#ifndef GSCHEM_MULTIATTRIB_DOCKABLE_H
#define GSCHEM_MULTIATTRIB_DOCKABLE_H


/*
 * GschemMultiattribDockable
 */

#define GSCHEM_TYPE_MULTIATTRIB_DOCKABLE         (gschem_multiattrib_dockable_get_type())
#define GSCHEM_MULTIATTRIB_DOCKABLE(obj)         (G_TYPE_CHECK_INSTANCE_CAST ((obj), GSCHEM_TYPE_MULTIATTRIB_DOCKABLE, GschemMultiattribDockable))
#define GSCHEM_MULTIATTRIB_DOCKABLE_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass), GSCHEM_TYPE_MULTIATTRIB_DOCKABLE, GschemMultiattribDockableClass))
#define GSCHEM_IS_MULTIATTRIB_DOCKABLE(obj)      (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GSCHEM_TYPE_MULTIATTRIB_DOCKABLE))


typedef struct _GschemMultiattribDockableClass GschemMultiattribDockableClass;
typedef struct _GschemMultiattribDockable      GschemMultiattribDockable;


struct _GschemMultiattribDockableClass {
  GschemDockableClass parent_class;

};

struct _GschemMultiattribDockable {
  GschemDockable parent;

  GedaList *object_list;
  int       total_num_in_list;
  int       num_complex_in_list;
  int       num_pins_in_list;
  int       num_nets_in_list;
  int       num_buses_in_list;
  int       num_lone_attribs_in_list;

  GtkWidget      *status_label;
  GtkTreeView    *treeview;
  GtkTreeModel   *store;

  GtkTreeViewColumn *column_visible, *column_show_name, *column_show_value,
                    *column_name, *column_value;

  GtkWidget      *show_inherited;
  GtkCombo       *combo_name;
  GtkTextView    *textview_value;
  GtkCheckButton *button_visible;
  GtkOptionMenu  *optionmenu_shownv;
  GtkWidget      *list_frame;
  GtkWidget      *add_frame;

  GdkColor       value_normal_text_color;   /* Workaround for lameness in GtkTextView */
  GdkColor       insensitive_text_color;
  GdkColor       not_identical_value_text_color;
  GdkColor       not_present_in_all_text_color;

  gulong object_list_changed_id;
};


GType gschem_multiattrib_dockable_get_type (void);


/*
 * CellTextView
 */

#define TYPE_CELL_TEXT_VIEW         (celltextview_get_type())
#define CELL_TEXT_VIEW(obj)         (G_TYPE_CHECK_INSTANCE_CAST ((obj), TYPE_CELL_TEXT_VIEW, CellTextView))
#define CELL_TEXT_VIEW_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass), TYPE_CELL_TEXT_VIEW, CellTextViewClass))
#define IS_CELL_TEXT_VIEW(obj)      (G_TYPE_CHECK_INSTANCE_TYPE ((obj), TYPE_CELL_TEXT_VIEW))


typedef struct _CellTextViewClass CellTextViewClass;
typedef struct _CellTextView      CellTextView;


struct _CellTextViewClass {
  GtkTextViewClass parent_class;

};

struct _CellTextView {
  GtkTextView parent_instance;

  gboolean editing_canceled;
};


GType celltextview_get_type (void);


/*
 * CellRendererMultiLineText
 */

#define TYPE_CELL_RENDERER_MULTI_LINE_TEXT         (cellrenderermultilinetext_get_type())
#define CELL_RENDERER_MULTI_LINE_TEXT(obj)         (G_TYPE_CHECK_INSTANCE_CAST ((obj), TYPE_CELL_RENDERER_MULTI_LINE_TEXT, CellRendererMultiLineText))
#define CELL_RENDERER_MULTI_LINE_TEXT_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass), TYPE_CELL_RENDERER_MULTI_LINE_TEXT, CellRendererMultiLineText))
#define IS_CELL_RENDERER_MULTI_LINE_TEXT(obj)      (G_TYPE_CHECK_INSTANCE_TYPE ((obj), TYPE_CELL_RENDERER_MULTI_LINE_TEXT))


typedef struct _CellRendererMultiLineTextClass CellRendererMultiLineTextClass;
typedef struct _CellRendererMultiLineText      CellRendererMultiLineText;


struct _CellRendererMultiLineTextClass {
  GtkCellRendererTextClass parent_class;

};

struct _CellRendererMultiLineText {
  GtkCellRendererText parent_instance;

  /*< private >*/
  guint focus_out_id;

};


GType cellrenderermultilinetext_get_type (void);


#endif /* GSCHEM_MULTIATTRIB_DOCKABLE_H */
