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
 * \file gschem_text_properties_dockable.c
 *
 * \brief A dockable for editing text properties
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
#include <gdk/gdkkeysyms.h>

#include "../include/gschem_text_properties_dockable.h"


static void
adjust_focus (GschemDockable *parent);

static void
class_init (GschemTextPropertiesDockableClass *klass);

static GtkWidget*
create_text_content_section (GschemTextPropertiesDockable *widget);

static GtkWidget*
create_text_property_section (GschemTextPropertiesDockable *widget);

static void
dispose (GObject *object);

static GtkWidget *
create_widget (GschemDockable *parent);

static void
set_selection_adapter (GschemTextPropertiesDockable *widget, GschemSelectionAdapter *adapter);

static void
update_text_alignment_model (GschemTextPropertiesDockable *widget);

static void
update_text_alignment_widget (GschemTextPropertiesDockable *widget);

static void
update_text_color_model (GschemTextPropertiesDockable *widget);

static void
update_text_color_widget (GschemTextPropertiesDockable *widget);

static void
update_text_content_model (GschemTextPropertiesDockable *widget);

static void
update_text_content_widget (GschemTextPropertiesDockable *widget);

static void
update_text_rotation_model (GschemTextPropertiesDockable *widget);

static void
update_text_rotation_widget (GschemTextPropertiesDockable *widget);



/*! \brief Adjust widget focus for the convienence of the user
 *
 *  If selecting one item, this function selects all the text content and gives
 *  focus to the text view. If multiple items are selected, this function gives
 *  focus to the color combo box.
 *
 *  \param [in] widget This text properties widget
 */
static void
adjust_focus (GschemDockable *parent)
{
  GschemTextPropertiesDockable *widget =
    GSCHEM_TEXT_PROPERTIES_DOCKABLE (parent);

  g_return_if_fail (widget != NULL);
  g_return_if_fail (widget->text_view != NULL);
  g_return_if_fail (widget->colorcb != NULL);

  if (gtk_widget_is_sensitive (widget->text_view)) {
    select_all_text_in_textview (GTK_TEXT_VIEW (widget->text_view));
    gtk_widget_grab_focus (widget->text_view);
  }
  else {
    gtk_widget_grab_focus (widget->colorcb);
  }
}



/*! \brief Get/register text properties widget type.
 */
GType
gschem_text_properties_dockable_get_type ()
{
  static GType type = 0;

  if (type == 0) {
    static const GTypeInfo info = {
      sizeof(GschemTextPropertiesDockableClass),
      NULL,                                       /* base_init */
      NULL,                                       /* base_finalize */
      (GClassInitFunc) class_init,
      NULL,                                       /* class_finalize */
      NULL,                                       /* class_data */
      sizeof(GschemTextPropertiesDockable),
      0,                                          /* n_preallocs */
      NULL,                                       /* instance_init */
    };

    type = g_type_register_static (GSCHEM_TYPE_DOCKABLE,
                                   "GschemTextPropertiesDockable",
                                   &info,
                                   0);
  }

  return type;
}



/*! \private
 *  \brief Initialize the text properties widget class structure
 *
 *  \param [in] klass
 */
static void
class_init (GschemTextPropertiesDockableClass *klass)
{
  GObjectClass *object_class;

  g_return_if_fail (klass != NULL);

  GSCHEM_DOCKABLE_CLASS (klass)->create_widget = create_widget;
  GSCHEM_DOCKABLE_CLASS (klass)->post_present = adjust_focus;

  object_class = G_OBJECT_CLASS (klass);

  g_return_if_fail (object_class != NULL);

  object_class->dispose = dispose;
}



/*! \private
 *  \brief Create the text content section widget
 *
 *  \param [in] widget
 *  \return The new text content section widget
 */
static GtkWidget*
create_text_content_section (GschemTextPropertiesDockable *widget)
{
  GtkWidget *bbox = gtk_hbutton_box_new ();
  GtkWidget *scrolled = gtk_scrolled_window_new (NULL, NULL);
  GtkWidget *vbox = gtk_vbox_new (FALSE, 0);

  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled),
                                  GTK_POLICY_AUTOMATIC,
                                  GTK_POLICY_AUTOMATIC);

  widget->text_view = gtk_text_view_new ();

  gtk_text_view_set_editable (GTK_TEXT_VIEW (widget->text_view), TRUE);

  /*! \bug FIXME: Set tab's width in the textview widget. */
  /* See first the code in text_input_dialog and get it working before adding
   * it here.
   */

  gtk_container_add (GTK_CONTAINER (scrolled), widget->text_view);

  gtk_button_box_set_layout (GTK_BUTTON_BOX (bbox), GTK_BUTTONBOX_END);

  widget->apply_button = gtk_button_new_from_stock (GTK_STOCK_APPLY);

  g_signal_connect_swapped (G_OBJECT (widget->apply_button),
                            "clicked",
                            G_CALLBACK (update_text_content_model),
                            widget);

  gtk_box_pack_start (GTK_BOX (bbox),                          /* box     */
                      widget->apply_button,                    /* child   */
                      TRUE,                                    /* expand  */
                      TRUE,                                    /* fill    */
                      0);                                      /* padding */

  gtk_box_set_spacing (GTK_BOX (vbox), DIALOG_V_SPACING);

  gtk_box_pack_start (GTK_BOX (vbox),                          /* box     */
                      scrolled,                                /* child   */
                      TRUE,                                    /* expand  */
                      TRUE,                                    /* fill    */
                      0);                                      /* padding */

  gtk_box_pack_start (GTK_BOX (vbox),                          /* box     */
                      bbox,                                    /* child   */
                      FALSE,                                   /* expand  */
                      TRUE,                                    /* fill    */
                      0);                                      /* padding */

  return gschem_dialog_misc_create_section_widget (_("<b>Text Content</b>"), vbox);
}



/*! \private
 *  \brief Create the text property section widget
 *
 *  \param [in] widget
 *  \return The new text property section widget
 */
static GtkWidget*
create_text_property_section (GschemTextPropertiesDockable *widget)
{
  GtkWidget *label[4];
  GtkWidget *table;
  GtkWidget *editor[4];

  label[0] = gschem_dialog_misc_create_property_label(_("Color:"));
  label[1] = gschem_dialog_misc_create_property_label(_("Size:"));
  label[2] = gschem_dialog_misc_create_property_label(_("Alignment:"));
  label[3] = gschem_dialog_misc_create_property_label(_("Rotation:"));

  editor[0] = widget->colorcb = x_colorcb_new ();
  editor[1] = widget->textsizecb = gschem_integer_combo_box_new ();
  editor[2] = widget->aligncb = gschem_alignment_combo_new();
  editor[3] = widget->rotatecb = gschem_rotation_combo_new();

  table = gschem_dialog_misc_create_property_table (label, editor, 4);

  widget->bindings = g_slist_append (widget->bindings,
                                     gschem_binding_integer_new ("text-size",
                                                                 editor[1]));

  g_signal_connect_swapped (G_OBJECT (widget->colorcb),
                            "changed",
                            G_CALLBACK (update_text_color_model),
                            widget);

  g_signal_connect_swapped (G_OBJECT (widget->aligncb),
                            "changed",
                            G_CALLBACK (update_text_alignment_model),
                            widget);

  g_signal_connect_swapped (G_OBJECT (widget->rotatecb),
                            "changed",
                            G_CALLBACK (update_text_rotation_model),
                            widget);

  return gschem_dialog_misc_create_section_widget (_("<b>Text Properties</b>"), table);
}



/*! \private
 *  \brief Dispose
 *
 *  \param [in] object The text edit widget to dispose
 */
static void
dispose (GObject *object)
{
  GschemTextPropertiesDockable *widget;
  GschemTextPropertiesDockableClass *klass;
  GObjectClass *parent_class;

  g_return_if_fail (object != NULL);

  widget = GSCHEM_TEXT_PROPERTIES_DOCKABLE (object);

  set_selection_adapter (widget, NULL);

  g_slist_foreach (widget->bindings, (GFunc) g_object_unref, NULL);
  g_slist_free (widget->bindings);
  widget->bindings = NULL;

  /* lastly, chain up to the parent dispose */

  klass = GSCHEM_TEXT_PROPERTIES_DOCKABLE_GET_CLASS (object);
  g_return_if_fail (klass != NULL);
  parent_class = g_type_class_peek_parent (klass);
  g_return_if_fail (parent_class != NULL);
  parent_class->dispose (object);
}



/*! \private
 *  \brief Create text property widgets and set up property change
 *         notifications
 *
 *  \param [in,out] parent This dockable
 */
static GtkWidget *
create_widget (GschemDockable *parent)
{
  GschemTextPropertiesDockable *widget =
    GSCHEM_TEXT_PROPERTIES_DOCKABLE (parent);
  GschemToplevel *w_current = parent->w_current;

  GtkWidget *scrolled;
  GtkWidget *vbox;
  GtkWidget *viewport;

  scrolled = gtk_scrolled_window_new (NULL, NULL);

  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled),
                                  GTK_POLICY_NEVER,
                                  GTK_POLICY_AUTOMATIC);

  gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (scrolled),
                                       GTK_SHADOW_NONE);

  viewport = gtk_viewport_new (NULL, NULL);
  gtk_container_add (GTK_CONTAINER (scrolled), viewport);

  gtk_viewport_set_shadow_type (GTK_VIEWPORT (viewport),
                                GTK_SHADOW_NONE);

  vbox = gtk_vbox_new (FALSE, DIALOG_V_SPACING);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), DIALOG_BORDER_SPACING);
  gtk_container_add (GTK_CONTAINER (viewport), vbox);

  gtk_box_pack_start (GTK_BOX (vbox),                          /* box     */
                      create_text_content_section (widget),    /* child   */
                      TRUE,                                    /* expand  */
                      TRUE,                                    /* fill    */
                      0);                                      /* padding */

  gtk_box_pack_start (GTK_BOX (vbox),                          /* box     */
                      create_text_property_section (widget),   /* child   */
                      FALSE,                                   /* expand  */
                      FALSE,                                   /* fill    */
                      0);                                      /* padding */


    gschem_integer_combo_box_set_model (widget->textsizecb,
                                        gschem_toplevel_get_text_size_list_store (w_current));

    set_selection_adapter (widget,
                           gschem_toplevel_get_selection_adapter (w_current));


  gtk_widget_show_all (scrolled);
  return scrolled;
}



/*! \private
 *  \brief Set the selection that this widget manipulates
 *
 *  \param [in,out] widget    This widget
 *  \param [in]     selection The selection to manipulate
 */
static void
set_selection_adapter (GschemTextPropertiesDockable *widget, GschemSelectionAdapter *adapter)
{
  g_return_if_fail (widget != NULL);

  if (widget->adapter != NULL) {
    g_signal_handlers_disconnect_by_func (widget->adapter,
                                          G_CALLBACK (update_text_content_widget),
                                          widget);

    g_signal_handlers_disconnect_by_func (widget->adapter,
                                          G_CALLBACK (update_text_rotation_widget),
                                          widget);

    g_signal_handlers_disconnect_by_func (widget->adapter,
                                          G_CALLBACK (update_text_color_widget),
                                          widget);

    g_signal_handlers_disconnect_by_func (widget->adapter,
                                          G_CALLBACK (update_text_alignment_widget),
                                          widget);

    g_object_unref (widget->adapter);
  }

  widget->adapter = adapter;

  g_slist_foreach (widget->bindings,
                   (GFunc) gschem_binding_set_model_object,
                   adapter);

  if (widget->adapter != NULL) {
    g_object_ref (widget->adapter);

    g_signal_connect_swapped (widget->adapter,
                              "notify::text-alignment",
                              G_CALLBACK (update_text_alignment_widget),
                              widget);

    g_signal_connect_swapped (widget->adapter,
                              "notify::text-color",
                              G_CALLBACK (update_text_color_widget),
                              widget);

    g_signal_connect_swapped (widget->adapter,
                              "notify::text-rotation",
                              G_CALLBACK (update_text_rotation_widget),
                              widget);

    g_signal_connect_swapped (widget->adapter,
                              "notify::text-string",
                              G_CALLBACK (update_text_content_widget),
                              widget);

    update_text_alignment_widget (widget);
    update_text_color_widget (widget);
    update_text_rotation_widget (widget);
    update_text_content_widget (widget);
  }
}



/*! \private
 *  \brief Update the text alignment in the model
 *
 *  \param [in,out] widget This widget
 */
static void
update_text_alignment_model (GschemTextPropertiesDockable *widget)
{
  g_return_if_fail (widget != NULL);
  g_return_if_fail (widget->aligncb != NULL);

  if (widget->adapter != NULL) {
    int alignment = gschem_alignment_combo_get_align (widget->aligncb);

    if (alignment >= 0) {
      gschem_selection_adapter_set_text_alignment (widget->adapter, alignment);
    }
  }
}



/*! \private
 *  \brief Update the value in the text alignment widget
 *
 *  \param [in,out] widget This widget
 */
static void
update_text_alignment_widget (GschemTextPropertiesDockable *widget)
{
  g_return_if_fail (widget != NULL);
  g_return_if_fail (widget->aligncb != NULL);

  if (widget->adapter != NULL) {
    int alignment = gschem_selection_adapter_get_text_alignment (widget->adapter);

    g_signal_handlers_block_by_func (G_OBJECT (widget->aligncb),
                                     G_CALLBACK (update_text_alignment_model),
                                     widget);

    gschem_alignment_combo_set_align (widget->aligncb, alignment);

    g_signal_handlers_unblock_by_func (G_OBJECT (widget->aligncb),
                                       G_CALLBACK (update_text_alignment_model),
                                       widget);

    gtk_widget_set_sensitive (GTK_WIDGET (widget->aligncb), (alignment != NO_SELECTION));
  }
}



/*! \private
 *  \brief Update the text color value in the model
 *
 *  \param [in,out] widget This widget
 */
static void
update_text_color_model (GschemTextPropertiesDockable *widget)
{
  g_return_if_fail (widget != NULL);
  g_return_if_fail (widget->colorcb != NULL);

  if (widget->adapter != NULL) {
    int color = x_colorcb_get_index (widget->colorcb);

    if (color >= 0) {
      gschem_selection_adapter_set_text_color (widget->adapter, color);
    }
  }
}



/*! \private
 *  \brief Update the value in the object color widget
 *
 *  \param [in,out] widget This widget
 */
static void
update_text_color_widget (GschemTextPropertiesDockable *widget)
{
  g_return_if_fail (widget != NULL);
  g_return_if_fail (widget->colorcb != NULL);

  if (widget->adapter != NULL) {
    int color = gschem_selection_adapter_get_text_color (widget->adapter);

    g_signal_handlers_block_by_func (G_OBJECT (widget->colorcb),
                                     G_CALLBACK (update_text_color_model),
                                     widget);

    x_colorcb_set_index(widget->colorcb, color);

    g_signal_handlers_unblock_by_func (G_OBJECT (widget->colorcb),
                                       G_CALLBACK (update_text_color_model),
                                       widget);

    gtk_widget_set_sensitive (GTK_WIDGET (widget->colorcb), (color != NO_SELECTION));
  }
}



/*! \private
 *  \brief Update the text string in the model
 *
 *  \param [in,out] widget This widget
 */
static void
update_text_content_model (GschemTextPropertiesDockable *widget)
{
  g_return_if_fail (widget != NULL);
  g_return_if_fail (widget->text_view != NULL);

  if (widget->adapter != NULL) {
    char *string;
    GtkTextBuffer *buffer;
    GtkTextIter start;
    GtkTextIter end;

    buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (widget->text_view));
    gtk_text_buffer_get_bounds (buffer, &start, &end);
    string =  gtk_text_iter_get_text (&start, &end);

    if (string != NULL) {
      gschem_selection_adapter_set_text_string (widget->adapter, string, widget->parent.w_current);
    }
  }
}



/*! \private
 *  \brief Update the value in the object color widget
 *
 *  \param [in,out] widget This widget
 */
static void
update_text_content_widget (GschemTextPropertiesDockable *widget)
{
  g_return_if_fail (widget != NULL);
  g_return_if_fail (widget->text_view != NULL);

  if (widget->adapter != NULL) {
    GtkTextBuffer *buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (widget->text_view));
    const char *string = gschem_selection_adapter_get_text_string (widget->adapter);

    g_signal_handlers_block_by_func (G_OBJECT (widget->text_view),
                                     G_CALLBACK (update_text_content_model),
                                     widget);

    if (string != NULL) {
      gtk_text_buffer_set_text (buffer, string, -1);
    }
    else {
      GtkTextIter start;
      GtkTextIter end;

      gtk_text_buffer_get_bounds (buffer, &start, &end);
      gtk_text_buffer_delete (buffer, &start, &end);
    }

    g_signal_handlers_unblock_by_func (G_OBJECT (widget->text_view),
                                       G_CALLBACK (update_text_content_model),
                                       widget);

    gtk_widget_set_sensitive (GTK_WIDGET (widget->text_view), (string != NULL));
    gtk_widget_set_sensitive (GTK_WIDGET (widget->apply_button), (string != NULL));
  }
}



/*! \private
 *  \brief Update the text rotation value in the model
 *
 *  \param [in,out] widget This widget
 */
static void
update_text_rotation_model (GschemTextPropertiesDockable *widget)
{
  g_return_if_fail (widget != NULL);
  g_return_if_fail (widget->rotatecb != NULL);

  if (widget->adapter != NULL) {
    int angle = gschem_rotation_combo_get_angle (widget->rotatecb);

    if (angle >= 0) {
      gschem_selection_adapter_set_text_rotation (widget->adapter, angle);
    }
  }
}



/*! \private
 *  \brief Update the value in the text rotation widget
 *
 *  \param [in,out] widget This widget
 */
static void
update_text_rotation_widget (GschemTextPropertiesDockable *widget)
{
  g_return_if_fail (widget != NULL);
  g_return_if_fail (widget->rotatecb != NULL);

  if (widget->adapter != NULL) {
    int angle = gschem_selection_adapter_get_text_rotation (widget->adapter);

    g_signal_handlers_block_by_func (G_OBJECT (widget->rotatecb),
                                     G_CALLBACK (update_text_rotation_model),
                                     widget);

    gschem_rotation_combo_set_angle (widget->rotatecb, angle);

    g_signal_handlers_unblock_by_func (G_OBJECT (widget->rotatecb),
                                       G_CALLBACK (update_text_rotation_model),
                                       widget);

    gtk_widget_set_sensitive (GTK_WIDGET (widget->rotatecb), (angle != NO_SELECTION));
  }
}
