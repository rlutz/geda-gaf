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
 * \file gschem_find_patch_widget.c
 *
 * \brief A widget for finding text
 */

#include <config.h>

#include <stdio.h>
#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif
#ifdef HAVE_STRING_H
#include <string.h>
#endif
#ifdef HAVE_MATH_H
#include <math.h>
#endif

#include "gschem.h"



/*! \brief The columns in the GtkListStore
 */
enum
{
    COLUMN_NAME,
    COLUMN_INDEX,
    COLUMN_COUNT
};


enum
{
  PROP_0,
  PROP_DESCEND,
  PROP_FIND_TEXT_STRING,
  PROP_FIND_TYPE
};




static void
activate_entry (GtkWidget *entry, GschemFindPatchWidget *widget);

static void
click_cancel (GtkWidget *button, GschemFindPatchWidget *widget);

static void
click_find (GtkWidget *entry, GschemFindPatchWidget *widget);

static GtkListStore*
create_find_type_store ();

static void
dispose (GObject *object);

static void
finalize (GObject *object);

static void
get_property (GObject *object, guint param_id, GValue *value, GParamSpec *pspec);

static void
gschem_find_patch_widget_class_init (GschemFindPatchWidgetClass *klass);

static void
gschem_find_patch_widget_init (GschemFindPatchWidget *view);

static void
set_property (GObject *object, guint param_id, const GValue *value, GParamSpec *pspec);

static void
notify_entry_patch (GtkWidget *entry, GParamSpec *pspec, GschemFindPatchWidget *widget);



static GObjectClass *gschem_find_patch_widget_parent_class = NULL;



/* Callback for when the user presses enter in the entry widget
 */
static void
activate_entry (GtkWidget *entry, GschemFindPatchWidget *widget)
{
  g_return_if_fail (widget != NULL);

  if (gtk_entry_get_text_length (GTK_ENTRY (widget->entry)) > 0) {
    gtk_info_bar_response (GTK_INFO_BAR (widget), GTK_RESPONSE_OK);
  }
  else {
    gtk_info_bar_response (GTK_INFO_BAR (widget), GTK_RESPONSE_CANCEL);
  }
}



/* Callback for when the user clicks the cancel button
 */
static void
click_cancel (GtkWidget *button, GschemFindPatchWidget *widget)
{
  gtk_info_bar_response (GTK_INFO_BAR (widget), GTK_RESPONSE_CANCEL);
}



/* Callback for when the user clicks the find button
 */
static void
click_find (GtkWidget *entry, GschemFindPatchWidget *widget)
{
  g_return_if_fail (widget != NULL);

  if (gtk_entry_get_text_length (GTK_ENTRY (widget->entry)) > 0) {
    gtk_info_bar_response (GTK_INFO_BAR (widget), GTK_RESPONSE_OK);
  }
}



/*! \brief Dispose of the object
 */
static void
dispose (GObject *object)
{
  /* lastly, chain up to the parent dispose */

  g_return_if_fail (gschem_find_patch_widget_parent_class != NULL);
  gschem_find_patch_widget_parent_class->dispose (object);
}



/*! \brief Finalize object
 */
static void
finalize (GObject *object)
{
  /* lastly, chain up to the parent finalize */

  g_return_if_fail (gschem_find_patch_widget_parent_class != NULL);
  gschem_find_patch_widget_parent_class->finalize (object);
}


/*! \brief Create the text find dialog
 *  \par Function Description
 *  This function creates the text find dialog.
 */
void find_patch_dialog (GschemToplevel *w_current)
{
  g_return_if_fail (w_current != NULL);
  g_return_if_fail (w_current->toplevel != NULL);

  /* Unlike with text find, do not change the search text (file name) here:
     it will retrain its previous value. It's likely that the same
     file name is to be used over and over (the name of the project),
     but with multi-page schematics it's hard to figure what that name
     would be. */

  gtk_widget_show (GTK_WIDGET (w_current->find_patch_widget));
  gtk_widget_grab_focus (gschem_find_patch_widget_get_entry (GSCHEM_FIND_PATCH_WIDGET (w_current->find_patch_widget)));
  gtk_editable_select_region (GTK_EDITABLE (gschem_find_patch_widget_get_entry (GSCHEM_FIND_PATCH_WIDGET (w_current->find_patch_widget))), 0, -1);
}


/*! \brief Get a property
 *
 *  \param [in]     object
 *  \param [in]     param_id
 *  \param [in,out] value
 *  \param [in]     pspec
 */
static void
get_property (GObject *object, guint param_id, GValue *value, GParamSpec *pspec)
{
  GschemFindPatchWidget *widget = GSCHEM_FIND_PATCH_WIDGET (object);

  switch (param_id) {
    case PROP_DESCEND:
      g_value_set_boolean (value, gschem_find_patch_widget_get_descend (widget));
      break;

    case PROP_FIND_TEXT_STRING:
      g_value_set_string (value, gschem_find_patch_widget_get_find_patch_string (widget));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
  }
}



/*! \brief Initialize GschemFindPatchWidget class
 *
 *  \param [in] klass The class for the GschemFindPatchWidget
 */
static void
gschem_find_patch_widget_class_init (GschemFindPatchWidgetClass *klass)
{
  gschem_find_patch_widget_parent_class = G_OBJECT_CLASS (g_type_class_peek_parent (klass));

  G_OBJECT_CLASS (klass)->dispose  = dispose;
  G_OBJECT_CLASS (klass)->finalize = finalize;

  G_OBJECT_CLASS (klass)->get_property = get_property;
  G_OBJECT_CLASS (klass)->set_property = set_property;

  g_object_class_install_property (G_OBJECT_CLASS (klass),
                                   PROP_DESCEND,
                                   g_param_spec_boolean ("descend",
                                                         "Descend",
                                                         "Descend",
                                                         FALSE,
                                                         G_PARAM_READWRITE | G_PARAM_CONSTRUCT));

  g_object_class_install_property (G_OBJECT_CLASS (klass),
                                   PROP_FIND_TYPE,
                                   g_param_spec_int ("find-type",
                                                     "Find Type",
                                                     "Find Type",
                                                     0,
                                                     2,
                                                     FIND_TYPE_SUBSTRING,
                                                     G_PARAM_READWRITE | G_PARAM_CONSTRUCT));

  g_object_class_install_property (G_OBJECT_CLASS (klass),
                                   PROP_FIND_TEXT_STRING,
                                   g_param_spec_string ("find-text-string",
                                                        "Find Patch String",
                                                        "Find Patch String",
                                                        "",
                                                        G_PARAM_READWRITE | G_PARAM_CONSTRUCT));
}



/*! \brief Get the descend property
 *
 *  \param [in] widget This GschemFindPatchWidget
 *  \return The descend property
 */
int
gschem_find_patch_widget_get_descend (GschemFindPatchWidget *widget)
{
  g_return_val_if_fail (widget != NULL, FALSE);

  return gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (widget->descend_button));
}



/*! \brief Get the entry
 *
 *  \param [in] widget This GschemFindPatchWidget
 *  \return The entry
 */
GtkWidget*
gschem_find_patch_widget_get_entry (GschemFindPatchWidget *widget)
{
  g_return_val_if_fail (widget != NULL, NULL);

  return widget->entry;
}



/*! \brief Get the find text string
 *
 *  \param [in] widget This GschemFindPatchWidget
 *  \return The find text string
 */
const char*
gschem_find_patch_widget_get_find_patch_string (GschemFindPatchWidget *widget)
{
  g_return_val_if_fail (widget != NULL, NULL);

  return gtk_entry_get_text (GTK_ENTRY (widget->entry));
}



/*! \brief Get/register GschemFindPatchWidget type.
 */
GType
gschem_find_patch_widget_get_type ()
{
  static GType type = 0;

  if (type == 0) {
    static const GTypeInfo info = {
      sizeof(GschemFindPatchWidgetClass),
      NULL,                                                    /* base_init */
      NULL,                                                    /* base_finalize */
      (GClassInitFunc) gschem_find_patch_widget_class_init,
      NULL,                                                    /* class_finalize */
      NULL,                                                    /* class_data */
      sizeof(GschemFindPatchWidget),
      0,                                                       /* n_preallocs */
      (GInstanceInitFunc) gschem_find_patch_widget_init,
    };

    type = g_type_register_static (GTK_TYPE_INFO_BAR, "GschemFindPatchWidget", &info, 0);
  }

  return type;
}



/*! \brief Initialize GschemFindPatchWidget instance
 *
 *  \param [in,out] view the GschemFindPatchWidget
 */
static void
gschem_find_patch_widget_init (GschemFindPatchWidget *widget)
{
  GtkWidget *action = gtk_info_bar_get_action_area (GTK_INFO_BAR (widget));
  GtkWidget *button_box;
  GtkWidget *cancel_button;
  GtkWidget *content = gtk_info_bar_get_content_area (GTK_INFO_BAR (widget));

  g_return_if_fail (widget != NULL);

  gtk_widget_set_no_show_all (GTK_WIDGET (widget), TRUE);

  widget->find_type_model = GTK_TREE_MODEL (create_find_type_store ());

  widget->label = gtk_label_new ("Patch file name to load:");
  gtk_widget_set_visible (widget->label, TRUE);
  gtk_box_pack_start (GTK_BOX (content), widget->label, TRUE, TRUE, 0);

  widget->entry = gtk_entry_new ();
  gtk_widget_set_visible (widget->entry, TRUE);
  gtk_box_pack_start (GTK_BOX (content), widget->entry, TRUE, TRUE, 0);

  widget->descend_button = gtk_check_button_new_with_label(_("descend into hierarchy"));
  gtk_widget_set_visible (widget->descend_button, TRUE);
  gtk_box_pack_start (GTK_BOX (content), widget->descend_button, FALSE, FALSE, 0);

  button_box = gtk_hbutton_box_new ();
  gtk_widget_set_visible (button_box, TRUE);
  gtk_box_pack_start (GTK_BOX (content), button_box, FALSE, FALSE, 0);

  widget->find_button = gtk_button_new_with_label (_("Find"));
  gtk_widget_set_sensitive (widget->find_button, FALSE);
  gtk_widget_set_visible (widget->find_button, TRUE);
  gtk_box_pack_start (GTK_BOX (button_box), widget->find_button, FALSE, FALSE, 0);

  cancel_button = gtk_button_new_from_stock (GTK_STOCK_CANCEL);
  gtk_widget_set_visible (cancel_button, TRUE);
  gtk_box_pack_start (GTK_BOX (button_box), cancel_button, FALSE, FALSE, 0);

  gtk_widget_set_no_show_all (action, TRUE);
  gtk_widget_set_visible (action, FALSE);

  g_signal_connect (G_OBJECT (widget->entry),
                    "activate",
                    G_CALLBACK (activate_entry),
                    widget);

  g_signal_connect (G_OBJECT (cancel_button),
                    "clicked",
                    G_CALLBACK (click_cancel),
                    widget);

  g_signal_connect (G_OBJECT (widget->find_button),
                    "clicked",
                    G_CALLBACK (click_find),
                    widget);

  g_signal_connect (G_OBJECT (widget->entry),
                    "notify::text",
                    G_CALLBACK (notify_entry_patch),
                    widget);
}



/*! \brief Set descend
 *
 *  \param [in,out] view This GschemFindPatchWidget
 *  \param [in]     text The label text
 */
void
gschem_find_patch_widget_set_descend (GschemFindPatchWidget *widget, int descend)
{
  g_return_if_fail (widget != NULL);

  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (widget->descend_button), descend);

  g_object_notify (G_OBJECT (widget), "descend");
}



/*! \brief Set the type of find to perform
 *
 *  \param [in,out] view This GschemFindPatchWidget
 *  \param [in]     text The label text
 */
void
gschem_find_patch_widget_set_find_type (GschemFindPatchWidget *widget, int type)
{
  g_return_if_fail (widget != NULL);

  if (type >= 0) {
    GtkTreeIter iter;
    gboolean success;
    GValue value = {0};

    success = gtk_tree_model_get_iter_first (GTK_TREE_MODEL (widget->find_type_model), &iter);
    while (success) {
      gtk_tree_model_get_value (GTK_TREE_MODEL (widget->find_type_model), &iter, COLUMN_INDEX, &value);
      if (g_value_get_int (&value) == type) {
        g_value_unset (&value);
        break;
      }
      g_value_unset (&value);
      success = gtk_tree_model_iter_next (GTK_TREE_MODEL(widget->find_type_model), &iter);
    }
  }

  g_object_notify (G_OBJECT (widget), "find-type");
}



/*! \brief Set the find text string
 *
 *  \param [in,out] view This GschemFindPatchWidget
 *  \param [in]     str  The find text string
 */
void
gschem_find_patch_widget_set_find_text_string (GschemFindPatchWidget *widget, const char *str)
{
  g_return_if_fail (widget != NULL);

  gtk_entry_set_text (GTK_ENTRY (widget->entry), str);

  g_object_notify (G_OBJECT (widget), "find-text-string");
}


static GtkListStore*
create_find_type_store ()
{
  GtkTreeIter iter;
  GtkListStore *store;

  store = gtk_list_store_new (COLUMN_COUNT,
                              G_TYPE_STRING,
                              G_TYPE_INT);

  gtk_list_store_append (store, &iter);
  gtk_list_store_set (store, &iter,
    COLUMN_NAME,      _("Find Patch:"),
    COLUMN_INDEX,     FIND_TYPE_SUBSTRING,
    -1
    );

  gtk_list_store_append (store, &iter);
  gtk_list_store_set (store, &iter,
    COLUMN_NAME,      _("Find Pattern:"),
    COLUMN_INDEX,     FIND_TYPE_PATTERN,
    -1
    );

  gtk_list_store_append (store, &iter);
  gtk_list_store_set (store, &iter,
    COLUMN_NAME,      _("Find Regex:"),
    COLUMN_INDEX,     FIND_TYPE_REGEX,
    -1
    );

  return store;
}


/*! \brief Update the sensitivity of the find button
 */
static void
notify_entry_patch (GtkWidget *entry, GParamSpec *pspec, GschemFindPatchWidget *widget)
{
  g_return_if_fail (widget != NULL);

  gtk_widget_set_sensitive (widget->find_button,
                            (gtk_entry_get_text_length (GTK_ENTRY (widget->entry)) > 0));
}



/*! \brief Set a gobject property
 */
static void
set_property (GObject *object, guint param_id, const GValue *value, GParamSpec *pspec)
{
  GschemFindPatchWidget *widget = GSCHEM_FIND_PATCH_WIDGET (object);

  switch (param_id) {
    case PROP_DESCEND:
      gschem_find_patch_widget_set_descend (widget, g_value_get_boolean (value));
      break;

    case PROP_FIND_TEXT_STRING:
      gschem_find_patch_widget_set_find_text_string (widget, g_value_get_string (value));
      break;

    case PROP_FIND_TYPE:
      gschem_find_patch_widget_set_find_type (widget, g_value_get_int (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
  }
}
