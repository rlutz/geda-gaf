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
 * \file x_editlprop.c
 *
 * \brief A dialog box for editing an object's line properties.
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

#ifdef HAVE_LIBDMALLOC
#include <dmalloc.h>
#endif



static gint line_type_dialog_linetype_change (GtkWidget *widget, EditLProp *dialog);


static void
dispose (GObject *object);

static void
selection_changed (GedaList *selection, EditLProp *dialog);

static void
update_values (EditLProp *dialog);


/*! \brief get the linetype data from selected objects
 *  \par Function Description
 *  Get linetype information over all selected objects.
 *  If a object property is different to the other objects, then
 *  return -2 in that variable.
 *  \param [in]   selection the selection list
 *  \param [out]  end       OBJECT_END type
 *  \param [out]  type      OBJECT_TYPE type
 *  \param [out]  width     line width
 *  \param [out]  length    length of each line
 *  \param [out]  space     space between points and lines
 *  \returns TRUE if linetype found, FALSE otherwise
 */
static gboolean selection_get_line_type(GedaList *selection,
                                        OBJECT_END *end, OBJECT_TYPE *type,
                                        gint *width, gint *length, gint *space)
{
  GList *iter;
  OBJECT *object;
  gboolean found = FALSE;
  OBJECT_END oend;
  OBJECT_TYPE otype;
  gint owidth, olength, ospace;

  *end = -1;
  *type = -1;
  *width = -1;
  *length = -1;
  *space = -1;

  for (iter = geda_list_get_glist (selection);
       iter != NULL;
       iter = g_list_next(iter)) {
    object = (OBJECT *) iter->data;
    if (! o_get_line_options(object, &oend, &otype,
                             &owidth, &olength, &ospace))
      continue;

    if (found == FALSE) {  /* first object with linetype */
      found = TRUE;
      *end = oend;
      *type = otype;
      *width = owidth;
      *length = olength;
      *space = ospace;
    } else {
      /* indicate different values with the value -2 */
      if (*end != oend) *end = -2;
      if (*type != otype) *type = -2;
      if (*width != owidth) *width = -2;
      if (*length != olength) *length = -2;
      if (*space != ospace) *space = -2;
      if (*end != oend) *end = -2;
    }
  }

  return found;
}



/*! \brief Callback function for the linetype menu item in the line type dialog
 *  \par Function Description
 *  This Function is called when the user changes the line type selection.
 *  It sets the dash space/length entries either active or inactive.
 */
static gint line_type_dialog_linetype_change (GtkWidget *widget, EditLProp *dialog)
{
  gtk_widget_set_sensitive (dialog->space_entry,
                            x_linetypecb_get_use_space (widget));
  gtk_widget_set_sensitive (dialog->length_entry,
                            x_linetypecb_get_use_length (widget));

  return(0);
}



/*! \brief Handles the user response when apply is selected
 *
 *  \param [in] dialog The edit line properties dialog
 */
static void
dialog_response_ok (EditLProp *dialog)
{
  GSCHEM_TOPLEVEL *w_current;
  TOPLEVEL *toplevel;
  GList *selection, *iter;
  OBJECT *object;
  OBJECT_TYPE type;
  OBJECT_END end;
  gint width, length, space;
  OBJECT_TYPE otype;
  OBJECT_END oend;
  gint owidth, olength, ospace;

  g_return_if_fail (dialog != NULL);

  if (dialog->selection == NULL) {
    return;
  }

  w_current = dialog->parent.w_current;
  g_return_if_fail (w_current != NULL);
  toplevel = w_current->toplevel;
  g_return_if_fail (toplevel != NULL);

  g_return_if_fail (dialog->selection == toplevel->page_current->selection_list);

  /* get the selection */
  if (! o_select_selected(w_current))
    return;

  selection = geda_list_get_glist(dialog->selection);

  /* get the new values from the text entries of the dialog */
  type = x_linetypecb_get_index (dialog->line_type);
  width  = x_integercb_get_value (dialog->width_entry);
  length = x_integercb_get_value (dialog->length_entry);
  space  = x_integercb_get_value (dialog->space_entry);
  end = x_linecapcb_get_index (dialog->line_end);

  for (iter = selection; iter != NULL; iter = g_list_next(iter)) {
    object = (OBJECT *) iter->data;
    if (! o_get_line_options(object, &oend, &otype,
                             &owidth, &olength, &ospace))
      continue;

    otype = type == -1 ? otype : type;
    owidth = width  == -1 ? owidth : width;
    olength = length == -1 ? olength : length;
    ospace = space  == -1 ? ospace : space;
    oend = end == -1 ? oend : end;

    /* set all not required options to -1 and
       set nice parameters if not provided by the user */
    switch (otype) {
    case (TYPE_SOLID):
      olength = ospace = -1;
      break;
    case (TYPE_DOTTED):
      olength = -1;
      if (ospace < 1) ospace = 100;
      break;
    case (TYPE_DASHED):
    case (TYPE_CENTER):
    case (TYPE_PHANTOM):
      if (ospace < 1) ospace = 100;
      if (olength < 1) olength = 100;
      break;
    default:
      g_assert_not_reached();
    }

    o_set_line_options (toplevel, object,
                        oend, otype, owidth, olength, ospace);
  }

  toplevel->page_current->CHANGED = 1;
  o_undo_savestate(w_current, UNDO_ALL);
}



/*! \brief Handles user responses from the edit text dialog box

 *  \par Function Description
 *  Callback function for the edit text dialog.
 *
 *  \param [in,out] dialog The edit text dialog
 *  \param [na]     unused Unused parameter
 */
static void
dialog_response (EditLProp *dialog, gint response, gpointer unused)
{
  g_return_if_fail (dialog != NULL);

  switch(response) {
    case GTK_RESPONSE_OK:
      dialog_response_ok(dialog);
      break;
    case GTK_RESPONSE_CLOSE:
    case GTK_RESPONSE_DELETE_EVENT:
       break;
    default:
      printf("%s: dialog_response(): strange signal %d\n", __FILE__, response);
  }

  i_set_state(dialog->parent.w_current, SELECT);
  i_update_toolbar(dialog->parent.w_current);
  gtk_widget_destroy(dialog->parent.w_current->lpwindow);
  dialog->parent.w_current->lpwindow=NULL;
}



/*! \brief Initialize EditLProp class
 *
 *  \par Function Description
 *
 *  GType class initialiser for Multiattrib. We override our parent
 *  virtual class methods as needed and register our GObject properties.
 *
 *  \param [in] klasse
 */
static void
editlprop_class_init (EditLPropClass *klasse)
{
  GObjectClass *object_klasse;

  g_return_if_fail (klasse != NULL);

  object_klasse = G_OBJECT_CLASS (klasse);

  g_return_if_fail (object_klasse != NULL);

  object_klasse->dispose = dispose;
}



/*! \brief Initialize EditLProp instance
 *
 *  \param [in,out] dialog The edit text dialog
 */
static void editlprop_init(EditLProp *dialog)
{
  GtkWidget *alignment;
  GtkWidget *vbox;
  GtkWidget *label = NULL;
  GtkWidget *rlabel[5] = { NULL };
  GtkWidget *table;
  int index;

  gtk_dialog_add_button (GTK_DIALOG (dialog),
                         GTK_STOCK_CLOSE,
                         GTK_RESPONSE_CLOSE);

  gtk_dialog_add_button (GTK_DIALOG (dialog),
                         GTK_STOCK_OK,
                         GTK_RESPONSE_OK);

  /* Set the alternative button order (ok, cancel, help) for other systems */
  gtk_dialog_set_alternative_button_order(GTK_DIALOG(dialog),
                                          GTK_RESPONSE_OK,
                                          GTK_RESPONSE_CLOSE,
                                          -1);

  gtk_window_position(GTK_WINDOW (dialog),
                      GTK_WIN_POS_NONE);

  g_signal_connect (G_OBJECT (dialog),
                    "response",
                    G_CALLBACK (dialog_response),
                    NULL);

  gtk_dialog_set_default_response(GTK_DIALOG(dialog),
                                  GTK_RESPONSE_OK);

  gtk_container_border_width(GTK_CONTAINER (dialog),
                             DIALOG_BORDER_SPACING);

  vbox = GTK_DIALOG(dialog)->vbox;
  gtk_box_set_spacing(GTK_BOX(vbox), DIALOG_V_SPACING);

  label = gtk_label_new (_("<b>Line Properties</b>"));
  gtk_label_set_use_markup (GTK_LABEL (label), TRUE);
  gtk_misc_set_alignment(GTK_MISC(label),0,0);
  gtk_box_pack_start(GTK_BOX(vbox), label, FALSE, FALSE, 0);

  alignment = gtk_alignment_new(0,0,1,1);
  gtk_alignment_set_padding(GTK_ALIGNMENT(alignment), 0, 0,
                            DIALOG_INDENTATION, 0);
  gtk_box_pack_start(GTK_BOX(vbox), alignment, TRUE, TRUE, 0);

  table = gtk_table_new (5, 2, FALSE);
  gtk_table_set_row_spacings(GTK_TABLE(table), DIALOG_V_SPACING);
  gtk_table_set_col_spacings(GTK_TABLE(table), DIALOG_H_SPACING);
  gtk_container_add(GTK_CONTAINER(alignment), table);

  rlabel[0] = gtk_label_new (_("Type:"));
  rlabel[1] = gtk_label_new (_("Width:"));
  rlabel[2] = gtk_label_new (_("Dash Length:"));
  rlabel[3] = gtk_label_new (_("Dash Space:"));
  rlabel[4] = gtk_label_new (_("Cap style:"));

  for (index=0; index<5; index++) {
    gtk_misc_set_alignment (GTK_MISC(rlabel[index]), 0, 0);
    gtk_table_attach (GTK_TABLE(table),rlabel[index],0,1,index,index+1, GTK_FILL,0,0,0);
  }

  dialog->line_type = x_linetypecb_new ();
  gtk_table_attach_defaults(GTK_TABLE(table), dialog->line_type,
                            1,2,0,1);

  dialog->width_entry = x_integercb_new ();
  gtk_entry_set_activates_default (x_integercb_get_entry (dialog->width_entry), TRUE);
  gtk_table_attach_defaults(GTK_TABLE(table), dialog->width_entry,
                            1,2,1,2);

  dialog->length_entry = x_integercb_new ();
  gtk_entry_set_activates_default (x_integercb_get_entry (dialog->length_entry), TRUE);
  gtk_table_attach_defaults(GTK_TABLE(table), dialog->length_entry,
                            1,2,2,3);

  dialog->space_entry = x_integercb_new ();
  gtk_entry_set_activates_default (x_integercb_get_entry (dialog->space_entry), TRUE);
  gtk_table_attach_defaults(GTK_TABLE(table), dialog->space_entry,
                            1,2,3,4);

  dialog->line_end = x_linecapcb_new ();
  gtk_table_attach_defaults(GTK_TABLE(table), dialog->line_end,
                            1,2,4,5);

  g_signal_connect(G_OBJECT (dialog->line_type), "changed",
                   G_CALLBACK (line_type_dialog_linetype_change),
                   dialog);
}



/*! \brief Get/register EditLProp type.
 */
GType editlprop_get_type()
{
  static GType type = 0;

  if (type == 0) {
    static const GTypeInfo info = {
      sizeof(EditLPropClass),
      NULL,                                   /* base_init */
      NULL,                                   /* base_finalize */
      (GClassInitFunc) editlprop_class_init,
      NULL,                                   /* class_finalize */
      NULL,                                   /* class_data */
      sizeof(EditLProp),
      0,                                      /* n_preallocs */
      (GInstanceInitFunc) editlprop_init,
    };

    type = g_type_register_static (GSCHEM_TYPE_DIALOG, "EditLProp", &info, 0);
  }

  return type;
}



/*! \brief Set the selection this dialog manipulates
 *
 *  \param [in,out] dialog
 *  \param [in]     selection
 */
void
x_editlprop_set_selection(EditLProp *dialog, GedaList *selection)
{
  g_return_if_fail (dialog != NULL);

  if (dialog->selection != NULL) {
    g_signal_handlers_disconnect_by_func (dialog->selection,
                                          G_CALLBACK (selection_changed),
                                          dialog);

    g_object_unref (dialog->selection);
  }

  dialog->selection = selection;

  if (dialog->selection != NULL) {
    g_object_ref (dialog->selection);

    g_signal_connect (G_OBJECT (dialog->selection),
                      "changed",
                      G_CALLBACK (selection_changed),
                      dialog);
  }

  update_values (dialog);
}



/*! \brief Dispose
 *
 *  \param [in,out] object This object
 */
static void
dispose (GObject *object)
{
  EditLProp *dialog;
  EditLPropClass *klasse;
  GObjectClass *parent_klasse;

  g_return_if_fail (object != NULL);

  dialog = EDITLPROP (object);

  x_editlprop_set_selection (dialog, NULL);

  /* lastly, chain up to the parent dispose */

  klasse = EDITLPROP_GET_CLASS (object);
  g_return_if_fail (klasse != NULL);
  parent_klasse = g_type_class_peek_parent (klasse);
  g_return_if_fail (parent_klasse != NULL);
  parent_klasse->dispose (object);
}



/*! \brief Signal handler for when the selection changes
 *
 *  \par Function Description
 *  This function gets called when items are added or removed from the
 *  selection.
 *
 *  \param [in]     selection The selection that changed
 *  \param [in,out] dialog    This dialog
 */
static void
selection_changed (GedaList *selection, EditLProp *dialog)
{
  g_return_if_fail (dialog != NULL);
  g_return_if_fail (selection != NULL);

  if (dialog->selection != NULL) {
    g_return_if_fail (selection == dialog->selection);

    update_values (dialog);
  }
}



/*! \brief Update the values in the dialog box widgets.
 *
 *  \param [in,out] dialog    This dialog
 */
static void
update_values (EditLProp *dialog)
{
  g_return_if_fail (dialog != NULL);

  if (dialog->selection != NULL) {
    gboolean success;
    OBJECT_END end=END_NONE;
    OBJECT_TYPE type=TYPE_SOLID;
    gint width=1, length=-1, space=-1;

    success = selection_get_line_type (dialog->selection, &end, &type, &width, &length, &space);

    if (success) {
      x_linecapcb_set_index (dialog->line_end, end);
      x_integercb_set_value (dialog->width_entry, width);
      x_integercb_set_value (dialog->length_entry, length);
      x_integercb_set_value (dialog->space_entry, space);
      x_linetypecb_set_index (dialog->line_type, type);
    } else {
      x_linecapcb_set_index (dialog->line_end, -1);
      x_integercb_set_value (dialog->width_entry, -1);
      x_integercb_set_value (dialog->length_entry, -1);
      x_integercb_set_value (dialog->space_entry, -1);
      x_linetypecb_set_index (dialog->line_type, -1);
    }

    gtk_widget_set_sensitive (GTK_WIDGET (dialog->line_type), success);
    gtk_widget_set_sensitive (GTK_WIDGET (dialog->width_entry), success);
    gtk_widget_set_sensitive (GTK_WIDGET (dialog->line_end), success);

    /* dash length and dash space are enabled/disabled by the value in line type */

    gtk_dialog_set_response_sensitive (GTK_DIALOG (dialog), GTK_RESPONSE_OK, success);
  }
}



/*! \brief Open the dialog box to edit fill properties
 *
 *  \par Function Description
 *  This function creates or raises the modal text entry dialog
 *
 *  \param [in] w_current The gschem toplevel
 */
void
line_type_dialog (GSCHEM_TOPLEVEL *w_current)
{
  g_return_if_fail (w_current != NULL);

  if (w_current->lpwindow == NULL) {
    /* dialog not created yet */
    w_current->lpwindow = g_object_new (TYPE_EDITLPROP,
                                        /* GtkContainer */
                                        "border-width",    DIALOG_BORDER_SPACING,
                                        /* GtkWindow */
                                        "title",           _("Edit Line Width & Type"),
                                        "default-width",   320,
                                        "default-height",  350,
                                        "window-position", GTK_WIN_POS_MOUSE,
                                        "allow-grow",      TRUE,
                                        "allow-shrink",    FALSE,
                                        "modal",           TRUE,
                                        /* GtkDialog */
                                        "has-separator",   TRUE,
                                        /* GschemDialog */
                                        "settings-name",    "line-type",
                                        "gschem-toplevel",  w_current,
                                        NULL);

    x_editlprop_set_selection (EDITLPROP (w_current->lpwindow),
                               w_current->toplevel->page_current->selection_list);

    gtk_window_set_transient_for (GTK_WINDOW (w_current->lpwindow),
                                  GTK_WINDOW (w_current->main_window));

    x_integercb_set_model (EDITLPROP (w_current->lpwindow)->width_entry,
                           gschem_toplevel_get_line_width_list_store (w_current));

    x_integercb_set_model (EDITLPROP (w_current->lpwindow)->length_entry,
                           gschem_toplevel_get_dash_length_list_store (w_current));

    x_integercb_set_model (EDITLPROP (w_current->lpwindow)->space_entry,
                           gschem_toplevel_get_dash_space_list_store (w_current));

    gtk_widget_show_all (w_current->lpwindow);
  }
  else {
    /* dialog already created */
    x_editlprop_set_selection (EDITLPROP (w_current->lpwindow),
                               w_current->toplevel->page_current->selection_list);

    gtk_window_present (GTK_WINDOW (w_current->lpwindow));
  }
}