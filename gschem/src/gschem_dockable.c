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

#include <config.h>
#include "gschem.h"
#include <gdk/gdkkeysyms.h>


enum {
  PROP_GSCHEM_TOPLEVEL = 1,
  PROP_TITLE,
  PROP_SETTINGS_NAME,
  PROP_CANCELLABLE,
  PROP_HELP_PAGE,
  PROP_INITIAL_STATE,
  PROP_INITIAL_WIDTH,
  PROP_INITIAL_HEIGHT,
  PROP_INITIAL_WIN_POS,
  PROP_STATE
};

static gpointer parent_class = NULL;

static void class_init (GschemDockableClass *class);
static void instance_init (GschemDockable *dockable);
static void constructed (GObject *object);
static void dispose (GObject *object);
static void finalize (GObject *object);
static void set_property (GObject *object, guint property_id,
                          const GValue *value, GParamSpec *pspec);
static void get_property (GObject *object, guint property_id,
                          GValue *value, GParamSpec *pspec);

static GtkWidget *real_create_widget (GschemDockable *dockable);
static void real_post_present (GschemDockable *dockable);
static void real_cancel (GschemDockable *dockable);
static void real_save_internal_geometry (GschemDockable *dockable,
                                         EdaConfig *cfg, gchar *group_name);
static void real_restore_internal_geometry (GschemDockable *dockable,
                                            EdaConfig *cfg, gchar *group_name);

static const gchar *get_notebook_group_name (
  GschemToplevel *w_current, GtkNotebook *notebook);
static GschemDockable *get_dockable_by_widget (
  GschemToplevel *w_current, GtkWidget *widget);
static GschemDockable *get_dockable_by_settings_name (
  GschemToplevel *w_current, const gchar *settings_name);

static void save_internal_geometry (GschemDockable *dockable);
static void save_window_geometry (GschemDockable *dockable);
static void save_state (GschemDockable *dockable);
static void save_page_order (GschemToplevel *w_current,
                             GtkNotebook *notebook);
static void save_current_page (GschemToplevel *w_current,
                               GtkNotebook *notebook,
                               GschemDockable *dockable);
static void restore_internal_geometry (GschemDockable *dockable);
static void restore_window_geometry (GschemDockable *dockable);
static void restore_state (GschemDockable *dockable);
static void restore_page_order (GschemToplevel *w_current,
                                GtkNotebook *notebook);
static void restore_current_page (GschemToplevel *w_current,
                                  GtkNotebook *notebook);
static void restore_detached_dockables (GschemToplevel *w_current);

static void create_widget (GschemDockable *dockable);
static void create_window (GschemDockable *dockable);
static void create_menu (GschemDockable *dockable);
static void update_menu_items (GschemDockable *dockable);
static void present_in_notebook (GschemDockable *dockable);

static void connect_notebook_signals (GschemToplevel *w_current,
                                      GtkWidget *notebook);

static gboolean
callback_after_window_key_press_event (GtkWidget *widget,
                                       GdkEventKey *event,
                                       GschemDockable *dockable);
static gboolean
callback_window_delete_event (GtkWidget *widget,
                              GdkEvent *event,
                              GschemDockable *dockable);
static void
callback_hide_button_clicked (GtkWidget *button,
                              GschemDockable *dockable);
static void
callback_cancel_button_clicked (GtkWidget *button,
                                GschemDockable *dockable);
static void
callback_help_button_clicked (GtkWidget *button,
                              GschemDockable *dockable);
static void
callback_dock_item_toggled (GtkWidget *check_menu_item,
                            GschemDockable *dockable);
static void
callback_menu_item_activate (GtkWidget *menu_item,
                             GschemDockable *dockable);
static gboolean
callback_event_box_button_press_event (GtkWidget *widget,
                                       GdkEventButton *event,
                                       GschemDockable *dockable);

static gboolean
callback_main_window_focus_in_event (GtkWidget *widget,
                                     GdkEvent *event,
                                     GschemToplevel *w_current);
static void
callback_notebook_page_added (GtkWidget *notebook,
                              GtkWidget *child,
                              guint page_num,
                              GschemToplevel *w_current);
static void
callback_notebook_page_added_removed_reordered (GtkWidget *notebook,
                                                GtkWidget *child,
                                                guint page_num,
                                                GschemToplevel *w_current);
static void
callback_notebook_switch_page (GtkWidget *notebook,
                               GtkWidget *arg1,
                               guint arg2,
                               GschemToplevel *w_current);
static GtkNotebook *
callback_notebook_create_window (GtkWidget *notebook,
                                 GtkWidget *page,
                                 gint x, gint y,
                                 GschemToplevel *w_current);
static gboolean
callback_notebook_key_press_event (GtkWidget *notebook,
                                   GdkEventKey *event,
                                   GschemToplevel *w_current);
static gboolean
callback_notebook_button_press_event (GtkWidget *notebook,
                                      GdkEventButton *event,
                                      GschemToplevel *w_current);


GType
gschem_dockable_state_get_type ()
{
  static GType type = 0;

  if (type == 0) {
    static const GEnumValue values[] = {
      { GSCHEM_DOCKABLE_STATE_HIDDEN,
        "GSCHEM_DOCKABLE_STATE_HIDDEN", "hidden" },
      { GSCHEM_DOCKABLE_STATE_DIALOG,
        "GSCHEM_DOCKABLE_STATE_DIALOG", "dialog" },
      { GSCHEM_DOCKABLE_STATE_WINDOW,
        "GSCHEM_DOCKABLE_STATE_WINDOW", "window" },
      { GSCHEM_DOCKABLE_STATE_DOCKED_LEFT,
        "GSCHEM_DOCKABLE_STATE_DOCKED_LEFT", "docked-left" },
      { GSCHEM_DOCKABLE_STATE_DOCKED_BOTTOM,
        "GSCHEM_DOCKABLE_STATE_DOCKED_BOTTOM", "docked-bottom" },
      { GSCHEM_DOCKABLE_STATE_DOCKED_RIGHT,
        "GSCHEM_DOCKABLE_STATE_DOCKED_RIGHT", "docked-right" },
      { 0, NULL, NULL }
    };

    type = g_enum_register_static ("GschemDockableState", values);
  }

  return type;
}


GType
gschem_dockable_get_type ()
{
  static GType type = 0;

  if (type == 0) {
    static const GTypeInfo info = {
      sizeof (GschemDockableClass),
      NULL,                             /* base_init */
      NULL,                             /* base_finalize */
      (GClassInitFunc) class_init,
      NULL,                             /* class_finalize */
      NULL,                             /* class_data */
      sizeof (GschemDockable),
      0,                                /* n_preallocs */
      (GInstanceInitFunc) instance_init,
      NULL                              /* value_table */
    };

    type = g_type_register_static (G_TYPE_OBJECT,
                                   "GschemDockable",
                                   &info,
                                   G_TYPE_FLAG_ABSTRACT);
  }

  return type;
}


static void
class_init (GschemDockableClass *class)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (class);

  class->create_widget             = real_create_widget;
  class->post_present              = real_post_present;
  class->cancel                    = real_cancel;
  class->save_internal_geometry    = real_save_internal_geometry;
  class->restore_internal_geometry = real_restore_internal_geometry;

  gobject_class->constructed  = constructed;
  gobject_class->dispose      = dispose;
  gobject_class->finalize     = finalize;
  gobject_class->set_property = set_property;
  gobject_class->get_property = get_property;

  parent_class = g_type_class_peek_parent (class);

  g_object_class_install_property (
    gobject_class, PROP_GSCHEM_TOPLEVEL,
    g_param_spec_pointer ("gschem-toplevel",
                          "",
                          "",
                          G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE));

  g_object_class_install_property (
    gobject_class, PROP_TITLE,
    g_param_spec_string ("title",
                         "",
                         "",
                         NULL,
                         G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE));
  g_object_class_install_property (
    gobject_class, PROP_SETTINGS_NAME,
    g_param_spec_string ("settings-name",
                         "",
                         "",
                         NULL,
                         G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE));
  g_object_class_install_property (
    gobject_class, PROP_CANCELLABLE,
    g_param_spec_boolean ("cancellable",
                          "",
                          "",
                          FALSE,
                          G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE));
  g_object_class_install_property (
    gobject_class, PROP_HELP_PAGE,
    g_param_spec_string ("help-page",
                         "",
                         "",
                         NULL,
                         G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE));

  g_object_class_install_property (
    gobject_class, PROP_INITIAL_STATE,
    g_param_spec_enum ("initial-state",
                       "",
                       "",
                       GSCHEM_TYPE_DOCKABLE_STATE,
                       GSCHEM_DOCKABLE_STATE_WINDOW,
                       G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE));
  g_object_class_install_property (
    gobject_class, PROP_INITIAL_WIDTH,
    g_param_spec_int ("initial-width",
                      "",
                      "",
                      -1,
                      G_MAXINT,
                      -1,
                      G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE));
  g_object_class_install_property (
    gobject_class, PROP_INITIAL_HEIGHT,
    g_param_spec_int ("initial-height",
                      "",
                      "",
                      -1,
                      G_MAXINT,
                      -1,
                      G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE));
  g_object_class_install_property (
    gobject_class, PROP_INITIAL_WIN_POS,
    g_param_spec_enum ("initial-window-position",
                       "",
                       "",
                       GTK_TYPE_WINDOW_POSITION,
                       GTK_WIN_POS_NONE,
                       G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE));

  g_object_class_install_property (
    gobject_class, PROP_STATE,
    g_param_spec_enum ("state",
                       "",
                       "",
                       GSCHEM_TYPE_DOCKABLE_STATE,
                       GSCHEM_DOCKABLE_STATE_HIDDEN,
                       G_PARAM_READWRITE));
}


static void
instance_init (GschemDockable *dockable)
{
  dockable->w_current = NULL;

  dockable->title = NULL;
  dockable->settings_name = NULL;
  dockable->group_name = NULL;
  dockable->cancellable = FALSE;
  dockable->help_page = NULL;

  dockable->initial_state = GSCHEM_DOCKABLE_STATE_HIDDEN;
  dockable->initial_width = -1;
  dockable->initial_height = -1;
  dockable->initial_position = GTK_WIN_POS_NONE;

  dockable->widget = NULL;
  dockable->window = NULL;
  dockable->vbox = NULL;
  dockable->action_area = NULL;
  dockable->hide_button = NULL;
  dockable->cancel_button = NULL;
  dockable->help_button = NULL;

  dockable->dock_left_item = NULL;
  dockable->dock_bottom_item = NULL;
  dockable->dock_right_item = NULL;

  dockable->menu = NULL;
  dockable->detach_item = NULL;
  dockable->move_left_item = NULL;
  dockable->move_bottom_item = NULL;
  dockable->move_right_item = NULL;
  dockable->close_item = NULL;
}


static void
constructed (GObject *object)
{
  G_OBJECT_CLASS (parent_class)->constructed (object);

  GschemDockable *dockable = GSCHEM_DOCKABLE (object);
  g_return_if_fail (dockable->w_current != NULL);

  dockable->w_current->dockables = g_list_append (
    dockable->w_current->dockables, dockable);

  dockable->dock_left_item =
    gtk_check_menu_item_new_with_label (dockable->title);
  gtk_widget_show (dockable->dock_left_item);
  dockable->dock_bottom_item =
    gtk_check_menu_item_new_with_label (dockable->title);
  gtk_widget_show (dockable->dock_bottom_item);
  dockable->dock_right_item =
    gtk_check_menu_item_new_with_label (dockable->title);
  gtk_widget_show (dockable->dock_right_item);

  restore_state (dockable);

  g_signal_connect (dockable->dock_left_item, "toggled",
                    G_CALLBACK (callback_dock_item_toggled), dockable);
  g_signal_connect (dockable->dock_bottom_item, "toggled",
                    G_CALLBACK (callback_dock_item_toggled), dockable);
  g_signal_connect (dockable->dock_right_item, "toggled",
                    G_CALLBACK (callback_dock_item_toggled), dockable);
}


static void
dispose (GObject *object)
{
  GschemDockable *dockable = GSCHEM_DOCKABLE (object);

  g_clear_object (&dockable->widget);
  g_clear_object (&dockable->menu);

  G_OBJECT_CLASS (parent_class)->dispose (object);
}


static void
finalize (GObject *object)
{
  GschemDockable *dockable = GSCHEM_DOCKABLE (object);

  g_free (dockable->title);
  g_free (dockable->settings_name);
  g_free (dockable->group_name);
  g_free (dockable->help_page);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}


static void
set_property (GObject *object, guint property_id,
              const GValue *value, GParamSpec *pspec)
{
  GschemDockable *dockable = GSCHEM_DOCKABLE (object);

  switch (property_id) {
    /* Properties specified as G_PARAM_CONSTRUCT[_ONLY] are set during
     * construction--the instance isn't fully initialized at that time.
     * Don't do any setup logic here. */

    case PROP_GSCHEM_TOPLEVEL:
      dockable->w_current = GSCHEM_TOPLEVEL (g_value_get_pointer (value));
      break;

    case PROP_TITLE:
      g_free (dockable->title);
      dockable->title = g_strdup (g_value_get_string (value));
      break;
    case PROP_SETTINGS_NAME:
      g_free (dockable->settings_name);
      g_free (dockable->group_name);
      dockable->settings_name = g_strdup (g_value_get_string (value));
      dockable->group_name = g_strdup_printf ("gschem.dialog-geometry.%s",
                                              dockable->settings_name);
      break;
    case PROP_CANCELLABLE:
      dockable->cancellable = g_value_get_boolean (value);
      break;
    case PROP_HELP_PAGE:
      g_free (dockable->help_page);
      dockable->help_page = g_strdup (g_value_get_string (value));
      break;

    case PROP_INITIAL_STATE:
      dockable->initial_state = g_value_get_enum (value);
      break;
    case PROP_INITIAL_WIDTH:
      dockable->initial_width = g_value_get_int (value);
      break;
    case PROP_INITIAL_HEIGHT:
      dockable->initial_height = g_value_get_int (value);
      break;
    case PROP_INITIAL_WIN_POS:
      dockable->initial_position = g_value_get_enum (value);
      break;

    /* Properties not specified as G_PARAM_CONSTRUCT[_ONLY] will only
     * be set after the object has been fully initialized. */

    case PROP_STATE:
      gschem_dockable_set_state (dockable, g_value_get_enum (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
  }
}


static void
get_property (GObject *object, guint property_id,
              GValue *value, GParamSpec *pspec)
{
  GschemDockable *dockable = GSCHEM_DOCKABLE (object);

  switch (property_id) {
    case PROP_GSCHEM_TOPLEVEL:
      g_value_set_pointer (value, dockable->w_current);
      break;
    case PROP_TITLE:
      g_value_set_string (value, dockable->title);
      break;
    case PROP_SETTINGS_NAME:
      g_value_set_string (value, dockable->settings_name);
      break;
    case PROP_CANCELLABLE:
      g_value_set_boolean (value, dockable->cancellable);
      break;
    case PROP_HELP_PAGE:
      g_value_set_string (value, dockable->help_page);
      break;
    case PROP_INITIAL_STATE:
      g_value_set_enum (value, dockable->initial_state);
      break;
    case PROP_INITIAL_WIDTH:
      g_value_set_int (value, dockable->initial_width);
      break;
    case PROP_INITIAL_HEIGHT:
      g_value_set_int (value, dockable->initial_height);
      break;
    case PROP_INITIAL_WIN_POS:
      g_value_set_enum (value, dockable->initial_position);
      break;
    case PROP_STATE:
      g_value_set_enum (value, gschem_dockable_get_state (dockable));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
  }
}


static GtkWidget *
real_create_widget (GschemDockable *dockable)
{
  g_assert_not_reached ();
}


static void
real_post_present (GschemDockable *dockable)
{
  /* default action: none */
}


static void
real_cancel (GschemDockable *dockable)
{
  /* default action: ignore */
}


static void
real_save_internal_geometry (GschemDockable *dockable,
                             EdaConfig *cfg, gchar *group_name)
{
  /* override if dockable has internal geometry */
}


static void
real_restore_internal_geometry (GschemDockable *dockable,
                                EdaConfig *cfg, gchar *group_name)
{
  /* override if dockable has internal geometry */
}


/******************************************************************************/


static const gchar *
get_notebook_group_name (GschemToplevel *w_current, GtkNotebook *notebook)
{
  if (GTK_WIDGET (notebook) == w_current->left_notebook)
    return "gschem.dock-geometry.left";
  if (GTK_WIDGET (notebook) == w_current->bottom_notebook)
    return "gschem.dock-geometry.bottom";
  if (GTK_WIDGET (notebook) == w_current->right_notebook)
    return "gschem.dock-geometry.right";
  g_assert_not_reached ();
}


static GschemDockable *
get_dockable_by_widget (GschemToplevel *w_current, GtkWidget *widget)
{
  g_return_val_if_fail (widget != NULL, NULL);

  GValue value = G_VALUE_INIT;
  g_value_init (&value, G_TYPE_POINTER);
  g_object_get_property (G_OBJECT (widget), "user-data", &value);
  return GSCHEM_DOCKABLE (g_value_get_pointer (&value));
}


static GschemDockable *
get_dockable_by_settings_name (GschemToplevel *w_current,
                               const gchar *settings_name)
{
  g_return_val_if_fail (settings_name != NULL, NULL);

  for (GList *l = w_current->dockables; l != NULL; l = l->next) {
    GschemDockable *dockable = GSCHEM_DOCKABLE (l->data);
    if (strcmp (settings_name, dockable->settings_name) == 0)
      return dockable;
  }

  return NULL;  /* unknown dockable name */
}


/*! \brief Save current internal geometry of a dockable */
static void
save_internal_geometry (GschemDockable *dockable)
{
  EdaConfig *cfg = eda_config_get_user_context ();
  g_return_if_fail (cfg != NULL);
  g_return_if_fail (dockable->widget != NULL);

  GSCHEM_DOCKABLE_GET_CLASS (dockable)
    ->save_internal_geometry (dockable, cfg, dockable->group_name);
}


/*! \brief Save current window position and size of a detached dockable */
static void
save_window_geometry (GschemDockable *dockable)
{
  EdaConfig *cfg = eda_config_get_user_context ();
  g_return_if_fail (cfg != NULL);
  g_return_if_fail (dockable->window != NULL);

  gint x, y, width, height;
  gtk_window_get_position (GTK_WINDOW (dockable->window), &x, &y);
  gtk_window_get_size (GTK_WINDOW (dockable->window), &width, &height);

  eda_config_set_int (cfg, dockable->group_name, "x", x);
  eda_config_set_int (cfg, dockable->group_name, "y", y);
  eda_config_set_int (cfg, dockable->group_name, "width", width);
  eda_config_set_int (cfg, dockable->group_name, "height", height);
}


/*! \brief Save current state of a dockable */
static void
save_state (GschemDockable *dockable)
{
  EdaConfig *cfg = eda_config_get_user_context ();
  g_return_if_fail (cfg != NULL);

  GschemDockableState state = gschem_dockable_get_state (dockable);
  const char *value =
    state == GSCHEM_DOCKABLE_STATE_WINDOW        ? "detached" :
    state == GSCHEM_DOCKABLE_STATE_DOCKED_LEFT   ? "docked-left" :
    state == GSCHEM_DOCKABLE_STATE_DOCKED_BOTTOM ? "docked-bottom" :
    state == GSCHEM_DOCKABLE_STATE_DOCKED_RIGHT  ? "docked-right" : "hidden";

  eda_config_set_string (cfg, dockable->group_name, "state", value);
}


static void
save_page_order (GschemToplevel *w_current,
                 GtkNotebook *notebook)
{
  EdaConfig *cfg = eda_config_get_user_context ();
  g_return_if_fail (cfg != NULL);

  gint page_count = gtk_notebook_get_n_pages (notebook);

  /* write new page order to configuration */
  const gchar **list = g_new (const gchar *, page_count);

  for (gint page_num = 0; page_num < page_count; page_num++) {
    GtkWidget *widget = gtk_notebook_get_nth_page (notebook, page_num);
    GschemDockable *dockable = get_dockable_by_widget (w_current, widget);
    g_return_if_fail (dockable != NULL);
    list[page_num] = dockable->settings_name;
  }

  eda_config_set_string_list (
    cfg, get_notebook_group_name (w_current, notebook),
    "page-order", list, page_count);

  g_free (list);

  /* clear current page key if notebook is empty */
  if (page_count == 0)
    eda_config_set_string (
      cfg, get_notebook_group_name (w_current, notebook),
      "current-page", "");
}


static void
save_current_page (GschemToplevel *w_current,
                   GtkNotebook *notebook,
                   GschemDockable *dockable)
{
  EdaConfig *cfg = eda_config_get_user_context ();
  g_return_if_fail (cfg != NULL);

  eda_config_set_string (
    cfg, get_notebook_group_name (w_current, notebook),
    "current-page", dockable->settings_name);
}


/*! \brief Restore last internal geometry of a dockable */
static void
restore_internal_geometry (GschemDockable *dockable)
{
  EdaConfig *cfg = eda_config_get_user_context ();
  g_return_if_fail (cfg != NULL);
  g_return_if_fail (dockable->widget != NULL);

  GSCHEM_DOCKABLE_GET_CLASS (dockable)
    ->restore_internal_geometry (dockable, cfg, dockable->group_name);
}


/*! \brief Restore last window position and size of a detached dockable */
static void
restore_window_geometry (GschemDockable *dockable)
{
  EdaConfig *cfg = eda_config_get_user_context ();
  g_return_if_fail (cfg != NULL);
  g_return_if_fail (dockable->window != NULL);

  if (!eda_config_has_group (cfg, dockable->group_name))
    gtk_window_set_position (GTK_WINDOW (dockable->window),
                             dockable->initial_position);
  else {
    gint x, y, width, height;
    x      = eda_config_get_int (cfg, dockable->group_name, "x", NULL);
    y      = eda_config_get_int (cfg, dockable->group_name, "y", NULL);
    width  = eda_config_get_int (cfg, dockable->group_name, "width",  NULL);
    height = eda_config_get_int (cfg, dockable->group_name, "height", NULL);

    gtk_window_move (GTK_WINDOW (dockable->window), x, y);
    if (width > 0 && height > 0)
      gtk_window_resize (GTK_WINDOW (dockable->window), width, height);
  }
}


/*! \brief Restore last state of a dockable (if it was docked) */
static void
restore_state (GschemDockable *dockable)
{
  EdaConfig *cfg = eda_config_get_user_context ();
  g_return_if_fail (cfg != NULL);

  gchar *state = eda_config_get_string (
    cfg, dockable->group_name, "state", NULL);

  if (state == NULL) {
    if (dockable->initial_state != GSCHEM_DOCKABLE_STATE_WINDOW)
      gschem_dockable_set_state (dockable, dockable->initial_state);
  } else if (strcmp (state, "detached") == 0)
    /* detached docks are restored later */;
  else if (strcmp (state, "docked-left") == 0)
    gschem_dockable_set_state (dockable, GSCHEM_DOCKABLE_STATE_DOCKED_LEFT);
  else if (strcmp (state, "docked-bottom") == 0)
    gschem_dockable_set_state (dockable, GSCHEM_DOCKABLE_STATE_DOCKED_BOTTOM);
  else if (strcmp (state, "docked-right") == 0)
    gschem_dockable_set_state (dockable, GSCHEM_DOCKABLE_STATE_DOCKED_RIGHT);

  g_free (state);
}


static void
restore_page_order (GschemToplevel *w_current,
                    GtkNotebook *notebook)
{
  EdaConfig *cfg = eda_config_get_user_context ();
  g_return_if_fail (cfg != NULL);

  gchar **list;
  gsize page_count = 0;
  gint page_num;

  list = eda_config_get_string_list (
    cfg, get_notebook_group_name (w_current, notebook),
    "page-order", &page_count, NULL);

  for (page_num = 0; page_num < page_count; page_num++) {
    GschemDockable *dockable =
      get_dockable_by_settings_name (w_current, list[page_num]);
    if (dockable != NULL && dockable->widget != NULL)
      gtk_notebook_reorder_child (notebook, dockable->widget, page_num);
  }

  g_strfreev (list);
}


static void
restore_current_page (GschemToplevel *w_current,
                      GtkNotebook *notebook)
{
  EdaConfig *cfg = eda_config_get_user_context ();
  g_return_if_fail (cfg != NULL);

  gchar *current_page = eda_config_get_string (
    cfg, get_notebook_group_name (w_current, notebook),
    "current-page", NULL);

  GschemDockable *dockable = NULL;
  if (current_page != NULL && *current_page != '\0')
    dockable = get_dockable_by_settings_name (w_current, current_page);

  g_free (current_page);
  if (dockable == NULL || dockable->widget == NULL)
    return;

  gint page_num = gtk_notebook_page_num (notebook, dockable->widget);
  if (page_num == -1)
    return;

  /* non-dockable tabs aren't necessarily visible at this point */
  gtk_widget_show (dockable->widget);

  gtk_notebook_set_current_page (notebook, page_num);
}


static void
restore_detached_dockables (GschemToplevel *w_current)
{
  EdaConfig *cfg = eda_config_get_user_context ();
  g_return_if_fail (cfg != NULL);

  for (GList *l = w_current->dockables; l != NULL; l = l->next) {
    GschemDockable *dockable = GSCHEM_DOCKABLE (l->data);
    gchar *state = eda_config_get_string (
      cfg, dockable->group_name, "state", NULL);

    if (state != NULL ? strcmp (state, "detached") == 0 :
          dockable->initial_state == GSCHEM_DOCKABLE_STATE_WINDOW)
      gschem_dockable_detach (dockable, FALSE);

    g_free (state);
  }

  /* open up log window on startup */
  if (w_current->log_window == MAP_ON_STARTUP)
    switch (gschem_dockable_get_state (w_current->log_dockable)) {
      case GSCHEM_DOCKABLE_STATE_HIDDEN:
        /* For whatever reason, this only works if the action area is
         * hidden while showing the log window.  Otherwise, scrolling
         * down to the bottom of the log will cause drawing the main
         * window toolbar icons to break [sic]; but only if the log
         * window height is about the length of the log or smaller.
         *
         * I suspect this may somehow be connected to widget size
         * allocation, but I'm giving up on debugging this now and
         * just accept that it may flicker a bit. */

        gschem_dockable_detach (w_current->log_dockable, FALSE);
        gtk_widget_show (w_current->log_dockable->action_area);
        break;

      case GSCHEM_DOCKABLE_STATE_DIALOG:
      case GSCHEM_DOCKABLE_STATE_WINDOW:
        gtk_window_present (GTK_WINDOW (w_current->log_dockable->window));
        break;

      case GSCHEM_DOCKABLE_STATE_DOCKED_LEFT:
      case GSCHEM_DOCKABLE_STATE_DOCKED_BOTTOM:
      case GSCHEM_DOCKABLE_STATE_DOCKED_RIGHT:
        present_in_notebook (w_current->log_dockable);
        break;
    }
  else if (w_current->log_dockable->widget == NULL)
    /* create the widget so the log window can auto-show if necessary */
    create_widget (w_current->log_dockable);
}


/******************************************************************************/


static void
create_widget (GschemDockable *dockable)
{
  g_return_if_fail (dockable->widget == NULL);

  dockable->widget =
    GSCHEM_DOCKABLE_GET_CLASS (dockable)->create_widget (dockable);
  g_object_ref_sink (dockable->widget);

  /* GtkNotebook doesn't allow storing a pointer along with a widget,
   * so store the dockable in the user-data property of the widget */

  GValue value = G_VALUE_INIT;
  g_value_init (&value, G_TYPE_POINTER);
  g_value_set_pointer (&value, dockable);
  g_object_set_property (G_OBJECT (dockable->widget), "user-data", &value);
}


static void
create_window (GschemDockable *dockable)
{
  g_return_if_fail (dockable->window == NULL);

  dockable->window = g_object_new (GTK_TYPE_WINDOW,
                                   "title",          dockable->title,
                                   "default-width",  dockable->initial_width,
                                   "default-height", dockable->initial_height,
                                   NULL);

  gtk_window_set_transient_for (GTK_WINDOW (dockable->window),
                                GTK_WINDOW (dockable->w_current->main_window));
  gtk_window_set_type_hint (GTK_WINDOW (dockable->window),
                            GDK_WINDOW_TYPE_HINT_DIALOG);

  g_signal_connect_after (dockable->window,
                          "key-press-event",
                          G_CALLBACK (callback_after_window_key_press_event),
                          dockable);
  g_signal_connect (dockable->window,
                    "delete-event",
                    G_CALLBACK (callback_window_delete_event),
                    dockable);


  /* create containers */

  dockable->vbox = g_object_new (GTK_TYPE_VBOX,
                                 "border-width", 2,
                                 "spacing",      0,
                                 NULL);
  gtk_container_add (GTK_CONTAINER (dockable->window), dockable->vbox);
  gtk_widget_show (dockable->vbox);

  dockable->action_area = g_object_new (GTK_TYPE_HBUTTON_BOX,
                                        "border-width", 5,
                                        "spacing",      6,
                                        "layout-style", GTK_BUTTONBOX_END,
                                        NULL);
  gtk_box_pack_end (GTK_BOX (dockable->vbox),
                    dockable->action_area,
                    FALSE, TRUE, 0);
  gtk_widget_show (dockable->action_area);


  /* add buttons */

  if (dockable->cancellable) {
    dockable->cancel_button = gtk_button_new_from_stock (GTK_STOCK_CANCEL);
    gtk_widget_set_can_default (dockable->cancel_button, TRUE);
    gtk_widget_show (dockable->cancel_button);
    g_signal_connect (dockable->cancel_button, "clicked",
                      G_CALLBACK (callback_cancel_button_clicked), dockable);
    gtk_box_pack_end (GTK_BOX (dockable->action_area),
                      dockable->cancel_button,
                      FALSE, TRUE, 0);
  }

  dockable->hide_button = gtk_button_new_from_stock (
    dockable->cancellable ? GTK_STOCK_OK : GTK_STOCK_CLOSE);
  gtk_widget_set_can_default (dockable->hide_button, TRUE);
  gtk_widget_show (dockable->hide_button);
  g_signal_connect (dockable->hide_button, "clicked",
                    G_CALLBACK (callback_hide_button_clicked), dockable);
  gtk_box_pack_end (GTK_BOX (dockable->action_area),
                    dockable->hide_button,
                    FALSE, TRUE, 0);

  if (dockable->help_page != NULL) {
    dockable->help_button = gtk_button_new_from_stock (GTK_STOCK_HELP);
    gtk_widget_set_can_default (dockable->help_button, TRUE);
    gtk_widget_show (dockable->help_button);
    g_signal_connect (dockable->help_button, "clicked",
                      G_CALLBACK (callback_help_button_clicked), dockable);
    gtk_box_pack_end (GTK_BOX (dockable->action_area),
                      dockable->help_button,
                      FALSE, TRUE, 0);
    gtk_button_box_set_child_secondary (GTK_BUTTON_BOX (dockable->action_area),
                                        dockable->help_button, TRUE);
  }

  GdkScreen *screen = gtk_widget_get_screen (dockable->window);
  if (gtk_alternative_dialog_button_order (screen)) {
    gtk_box_reorder_child (GTK_BOX (dockable->action_area),
                           dockable->hide_button, 0);
    if (dockable->cancel_button != NULL)
      gtk_box_reorder_child (GTK_BOX (dockable->action_area),
                             dockable->cancel_button, 1);
    if (dockable->help_button != NULL)
      gtk_box_reorder_child (GTK_BOX (dockable->action_area),
                             dockable->help_button, 2);
  }

  gtk_widget_grab_default (dockable->hide_button);
}


static void
create_menu (GschemDockable *dockable)
{
  g_return_if_fail (dockable->menu == NULL);

  dockable->menu = gtk_menu_new ();
  g_object_ref_sink (dockable->menu);

  dockable->detach_item = gtk_menu_item_new_with_mnemonic (_("_Detach"));
  g_signal_connect (dockable->detach_item, "activate",
                    G_CALLBACK (callback_menu_item_activate), dockable);
  gtk_menu_shell_append (GTK_MENU_SHELL (dockable->menu),
                         dockable->detach_item);

  gtk_menu_shell_append (GTK_MENU_SHELL (dockable->menu),
                         gtk_separator_menu_item_new ());

  dockable->move_left_item =
    gtk_menu_item_new_with_mnemonic (_("Move to _left dock"));
  g_signal_connect (dockable->move_left_item, "activate",
                    G_CALLBACK (callback_menu_item_activate), dockable);
  gtk_menu_shell_append (GTK_MENU_SHELL (dockable->menu),
                         dockable->move_left_item);

  dockable->move_bottom_item =
    gtk_menu_item_new_with_mnemonic (_("Move to _bottom dock"));
  g_signal_connect (dockable->move_bottom_item, "activate",
                    G_CALLBACK (callback_menu_item_activate), dockable);
  gtk_menu_shell_append (GTK_MENU_SHELL (dockable->menu),
                         dockable->move_bottom_item);

  dockable->move_right_item =
    gtk_menu_item_new_with_mnemonic (_("Move to _right dock"));
  g_signal_connect (dockable->move_right_item, "activate",
                    G_CALLBACK (callback_menu_item_activate), dockable);
  gtk_menu_shell_append (GTK_MENU_SHELL (dockable->menu),
                         dockable->move_right_item);

  gtk_menu_shell_append (GTK_MENU_SHELL (dockable->menu),
                         gtk_separator_menu_item_new ());

  dockable->close_item = gtk_image_menu_item_new_from_stock (GTK_STOCK_CLOSE,
                                                             NULL);
  g_signal_connect (dockable->close_item, "activate",
                    G_CALLBACK (callback_menu_item_activate), dockable);
  gtk_menu_shell_append (GTK_MENU_SHELL (dockable->menu),
                         dockable->close_item);

  gtk_widget_show_all (dockable->menu);
}


static void
update_menu_items (GschemDockable *dockable)
{
  GschemDockableState state = gschem_dockable_get_state (dockable);

  gtk_check_menu_item_set_active (
    GTK_CHECK_MENU_ITEM (dockable->dock_left_item),
    state == GSCHEM_DOCKABLE_STATE_DOCKED_LEFT);
  gtk_check_menu_item_set_active (
    GTK_CHECK_MENU_ITEM (dockable->dock_bottom_item),
    state == GSCHEM_DOCKABLE_STATE_DOCKED_BOTTOM);
  gtk_check_menu_item_set_active (
    GTK_CHECK_MENU_ITEM (dockable->dock_right_item),
    state == GSCHEM_DOCKABLE_STATE_DOCKED_RIGHT);
}


static void
present_in_notebook (GschemDockable *dockable)
{
  GtkWidget *parent;
  gint i;

  g_return_if_fail (dockable->widget != NULL);

  parent = gtk_widget_get_parent (dockable->widget);
  g_return_if_fail (GTK_IS_NOTEBOOK (parent));

  i = gtk_notebook_page_num (GTK_NOTEBOOK (parent), dockable->widget);
  g_return_if_fail (i != -1);

  gtk_notebook_set_current_page (GTK_NOTEBOOK (parent), i);

  x_window_present (dockable->w_current);
  gtk_widget_grab_focus (dockable->widget);
}


void
gschem_dockable_hide (GschemDockable *dockable)
{
  g_return_if_fail (GSCHEM_IS_DOCKABLE (dockable));

  GtkWidget *parent = dockable->widget == NULL ? NULL :
                        gtk_widget_get_parent (dockable->widget);

  if (parent == NULL ||
      (dockable->window != NULL && parent == dockable->vbox &&
       !gtk_widget_get_visible (dockable->window)))
    /* already hidden, no action required */
    return;

  if (dockable->window != NULL && parent == dockable->vbox) {
    /* hide window */
    save_internal_geometry (dockable);
    save_window_geometry (dockable);
    gtk_widget_hide (dockable->window);
  } else {
    /* remove from notebook */
    save_internal_geometry (dockable);
    gtk_container_remove (GTK_CONTAINER (parent), dockable->widget);
  }

  update_menu_items (dockable);
  save_state (dockable);
}


void
gschem_dockable_detach (GschemDockable *dockable, gboolean show_action_area)
{
  g_return_if_fail (GSCHEM_IS_DOCKABLE (dockable));

  if (dockable->widget == NULL)
    create_widget (dockable);

  if (dockable->window == NULL)
    create_window (dockable);

  GtkWidget *parent = gtk_widget_get_parent (dockable->widget);

  if (parent == NULL) {
    /* freshly created or was removed from notebook */

    gtk_box_pack_start (GTK_BOX (dockable->vbox),
                        dockable->widget,
                        TRUE, TRUE, 0);

    if (show_action_area)
      gtk_widget_show (dockable->action_area);
    else
      gtk_widget_hide (dockable->action_area);

    restore_window_geometry (dockable);
    restore_internal_geometry (dockable);

    /* detached docks are only restored after the main window is visible */
    g_assert (gtk_widget_get_visible (dockable->w_current->main_window));

    gtk_window_present (GTK_WINDOW (dockable->window));
    save_state (dockable);

  } else if (dockable->window != NULL && parent == dockable->vbox &&
             !gtk_widget_get_visible (dockable->window)) {
    /* already in window, but window is currently hidden */

    if (show_action_area)
      gtk_widget_show (dockable->action_area);
    else
      gtk_widget_hide (dockable->action_area);

    restore_window_geometry (dockable);
    restore_internal_geometry (dockable);

    gtk_window_present (GTK_WINDOW (dockable->window));
    save_state (dockable);

  } else if (dockable->window != NULL && parent == dockable->vbox) {
    /* already in window, and window is currently visible
       just show/hide action area as appropriate */

    if (show_action_area && !gtk_widget_get_visible (dockable->action_area)) {
      gtk_widget_show (dockable->action_area);
      save_state (dockable);
    }
    if (!show_action_area && gtk_widget_get_visible (dockable->action_area)) {
      gtk_widget_hide (dockable->action_area);
      save_state (dockable);
    }

  } else {
    /* currently docked */

    g_assert (GTK_IS_NOTEBOOK (parent));
    save_internal_geometry (dockable);
    gtk_container_remove (GTK_CONTAINER (parent), dockable->widget);

    gtk_box_pack_start (GTK_BOX (dockable->vbox),
                        dockable->widget,
                        TRUE, TRUE, 0);

    if (show_action_area)
      gtk_widget_show (dockable->action_area);
    else
      gtk_widget_hide (dockable->action_area);

    restore_window_geometry (dockable);
    restore_internal_geometry (dockable);

    update_menu_items (dockable);

    gtk_window_present (GTK_WINDOW (dockable->window));
    save_state (dockable);
  }
}


void
gschem_dockable_attach_to_notebook (GschemDockable *dockable,
                                    GtkWidget *notebook)
{
  g_return_if_fail (GSCHEM_IS_DOCKABLE (dockable));
  g_return_if_fail (notebook == dockable->w_current->left_notebook ||
                    notebook == dockable->w_current->bottom_notebook ||
                    notebook == dockable->w_current->right_notebook);
  g_return_if_fail (GTK_IS_NOTEBOOK (notebook));

  if (dockable->widget == NULL)
    create_widget (dockable);

  GtkWidget *parent = gtk_widget_get_parent (dockable->widget);

  if (parent == NULL) {
    /* hidden and not attached to window */
  } else if (dockable->window != NULL && parent == dockable->vbox) {
    /* attached to window (hidden or visible) */
    if (gtk_widget_get_visible (dockable->window)) {
      save_internal_geometry (dockable);
      save_window_geometry (dockable);
      gtk_widget_hide (dockable->window);
    }
    gtk_container_remove (GTK_CONTAINER (parent), dockable->widget);
  } else {
    if (parent == notebook)
      /* already in requested notebook, nothing to do */
      return;

    g_assert (GTK_IS_NOTEBOOK (parent));
    save_internal_geometry (dockable);
    gtk_container_remove (GTK_CONTAINER (parent), dockable->widget);
  }

  g_assert (gtk_widget_get_parent (dockable->widget) == NULL);

  /* add widget to notebook */

  GtkWidget *event_box = gtk_event_box_new ();
  gtk_event_box_set_visible_window (GTK_EVENT_BOX (event_box), FALSE);
  g_signal_connect (event_box,
                    "button-press-event",
                    G_CALLBACK (callback_event_box_button_press_event),
                    dockable);

  GtkWidget *label = gtk_label_new (dockable->title);
  gtk_container_add (GTK_CONTAINER (event_box), label);
  gtk_widget_show (label);

  gtk_notebook_append_page (GTK_NOTEBOOK (notebook),
                            dockable->widget,
                            event_box);
  gtk_notebook_set_tab_reorderable (GTK_NOTEBOOK (notebook),
                                    dockable->widget, TRUE);
  gtk_notebook_set_tab_detachable (GTK_NOTEBOOK (notebook),
                                   dockable->widget, TRUE);

  restore_internal_geometry (dockable);
  update_menu_items (dockable);

  /* don't present when restoring state on startup */
  if (gtk_widget_get_visible (dockable->w_current->main_window)) {
    present_in_notebook (dockable);
    save_state (dockable);
  }
}


void
gschem_dockable_set_state (GschemDockable *dockable, GschemDockableState state)
{
  g_return_if_fail (GSCHEM_IS_DOCKABLE (dockable));

  switch (state) {
    case GSCHEM_DOCKABLE_STATE_HIDDEN:
      gschem_dockable_hide (dockable);
      break;
    case GSCHEM_DOCKABLE_STATE_DIALOG:
      gschem_dockable_detach (dockable, TRUE);
      break;
    case GSCHEM_DOCKABLE_STATE_WINDOW:
      gschem_dockable_detach (dockable, FALSE);
      break;
    case GSCHEM_DOCKABLE_STATE_DOCKED_LEFT:
      gschem_dockable_attach_to_notebook (
        dockable, dockable->w_current->left_notebook);
      break;
    case GSCHEM_DOCKABLE_STATE_DOCKED_BOTTOM:
      gschem_dockable_attach_to_notebook (
        dockable, dockable->w_current->bottom_notebook);
      break;
    case GSCHEM_DOCKABLE_STATE_DOCKED_RIGHT:
      gschem_dockable_attach_to_notebook (
        dockable, dockable->w_current->right_notebook);
      break;
    default:
      g_assert_not_reached ();
  }
}


GschemDockableState
gschem_dockable_get_state (GschemDockable *dockable)
{
  g_return_val_if_fail (GSCHEM_IS_DOCKABLE (dockable),
                        GSCHEM_DOCKABLE_STATE_HIDDEN);

  if (dockable->widget == NULL)
    return GSCHEM_DOCKABLE_STATE_HIDDEN;

  GtkWidget *parent = gtk_widget_get_parent (dockable->widget);
  if (parent == NULL)
    return GSCHEM_DOCKABLE_STATE_HIDDEN;

  if (dockable->window != NULL && parent == dockable->vbox) {
    if (!gtk_widget_get_visible (dockable->window))
      return GSCHEM_DOCKABLE_STATE_HIDDEN;
    if (!gtk_widget_get_visible (dockable->action_area))
      return GSCHEM_DOCKABLE_STATE_WINDOW;
    return GSCHEM_DOCKABLE_STATE_DIALOG;
  }

  if (parent == dockable->w_current->left_notebook)
    return GSCHEM_DOCKABLE_STATE_DOCKED_LEFT;
  if (parent == dockable->w_current->bottom_notebook)
    return GSCHEM_DOCKABLE_STATE_DOCKED_BOTTOM;
  if (parent == dockable->w_current->right_notebook)
    return GSCHEM_DOCKABLE_STATE_DOCKED_RIGHT;

  g_assert_not_reached ();
}


void
gschem_dockable_present (GschemDockable *dockable)
{
  g_return_if_fail (GSCHEM_IS_DOCKABLE (dockable));

  switch (gschem_dockable_get_state (dockable)) {
    case GSCHEM_DOCKABLE_STATE_HIDDEN:
      gschem_dockable_detach (dockable, TRUE);
      break;
    case GSCHEM_DOCKABLE_STATE_DIALOG:
    case GSCHEM_DOCKABLE_STATE_WINDOW:
      gtk_window_present (GTK_WINDOW (dockable->window));
      break;
    case GSCHEM_DOCKABLE_STATE_DOCKED_LEFT:
    case GSCHEM_DOCKABLE_STATE_DOCKED_BOTTOM:
    case GSCHEM_DOCKABLE_STATE_DOCKED_RIGHT:
      present_in_notebook (dockable);
      break;
    default:
      g_assert_not_reached ();
  }

  GSCHEM_DOCKABLE_GET_CLASS (dockable)->post_present (dockable);
}


/******************************************************************************/


static void
connect_notebook_signals (GschemToplevel *w_current, GtkWidget *notebook)
{
  g_signal_connect (
    notebook, "create-window",
    G_CALLBACK (callback_notebook_create_window), w_current);
  g_signal_connect (
    notebook, "page-added",
    G_CALLBACK (callback_notebook_page_added), w_current);
  g_signal_connect (
    notebook, "page-added",
    G_CALLBACK (callback_notebook_page_added_removed_reordered), w_current);
  g_signal_connect (
    notebook, "page-removed",
    G_CALLBACK (callback_notebook_page_added_removed_reordered), w_current);
  g_signal_connect (
    notebook, "page-reordered",
    G_CALLBACK (callback_notebook_page_added_removed_reordered), w_current);
  g_signal_connect (
    notebook, "switch-page",
    G_CALLBACK (callback_notebook_switch_page), w_current);
  g_signal_connect (
    notebook, "key-press-event",
    G_CALLBACK (callback_notebook_key_press_event), w_current);
  g_signal_connect (
    notebook, "button-press-event",
    G_CALLBACK (callback_notebook_button_press_event), w_current);
}


void
gschem_dockable_initialize_toplevel (GschemToplevel *w_current)
{
  /* restore saved page order and last opened page */
  restore_page_order (w_current, GTK_NOTEBOOK (w_current->left_notebook));
  restore_current_page (w_current, GTK_NOTEBOOK (w_current->left_notebook));

  restore_page_order (w_current, GTK_NOTEBOOK (w_current->bottom_notebook));
  restore_current_page (w_current, GTK_NOTEBOOK (w_current->bottom_notebook));

  restore_page_order (w_current, GTK_NOTEBOOK (w_current->right_notebook));
  restore_current_page (w_current, GTK_NOTEBOOK (w_current->right_notebook));

  /* connect signals */
  connect_notebook_signals (w_current, w_current->left_notebook);
  connect_notebook_signals (w_current, w_current->bottom_notebook);
  connect_notebook_signals (w_current, w_current->right_notebook);

  g_signal_connect (
    G_OBJECT (w_current->main_window), "focus-in-event",
    G_CALLBACK (callback_main_window_focus_in_event), w_current);

  /* populate docking area menus */
  for (GList *l = w_current->dockables; l != NULL; l = l->next) {
    gtk_menu_shell_append (GTK_MENU_SHELL (w_current->left_docking_area_menu),
                           GSCHEM_DOCKABLE (l->data)->dock_left_item);
    gtk_menu_shell_append (GTK_MENU_SHELL (w_current->bottom_docking_area_menu),
                           GSCHEM_DOCKABLE (l->data)->dock_bottom_item);
    gtk_menu_shell_append (GTK_MENU_SHELL (w_current->right_docking_area_menu),
                           GSCHEM_DOCKABLE (l->data)->dock_right_item);
  }
}


void
gschem_dockable_cleanup_toplevel (GschemToplevel *w_current)
{
  /* disconnect notebook signals */
  g_signal_handlers_disconnect_by_data (w_current->left_notebook, w_current);
  g_signal_handlers_disconnect_by_data (w_current->bottom_notebook, w_current);
  g_signal_handlers_disconnect_by_data (w_current->right_notebook, w_current);

  /* destroy dock windows */
  for (GList *l = w_current->dockables; l != NULL; l = l->next) {
    GschemDockable *dockable = GSCHEM_DOCKABLE (l->data);

    /* if the dockable is visible, save geometry */
    switch (gschem_dockable_get_state (dockable)) {
      case GSCHEM_DOCKABLE_STATE_HIDDEN:
        /* nothing to do */
        break;
      case GSCHEM_DOCKABLE_STATE_DIALOG:
      case GSCHEM_DOCKABLE_STATE_WINDOW:
        save_internal_geometry (dockable);
        save_window_geometry (dockable);
        break;
      case GSCHEM_DOCKABLE_STATE_DOCKED_LEFT:
      case GSCHEM_DOCKABLE_STATE_DOCKED_BOTTOM:
      case GSCHEM_DOCKABLE_STATE_DOCKED_RIGHT:
        save_internal_geometry (dockable);
        break;
    }

    if (dockable->window != NULL) {
      gtk_widget_destroy (dockable->window);
      dockable->window = NULL;
    }

    if (dockable->widget != NULL) {
      gtk_widget_destroy (dockable->widget);
      dockable->widget = NULL;
    }
  }
}


/******************************************************************************/


static gboolean
callback_after_window_key_press_event (GtkWidget *widget,
                                       GdkEventKey *event,
                                       GschemDockable *dockable)
{
  if ((event->keyval == GDK_KEY_Return ||
       event->keyval == GDK_KEY_ISO_Enter ||
       event->keyval == GDK_KEY_KP_Enter) &&
      (event->state & gtk_accelerator_get_default_mod_mask ()) == 0) {
    if (gschem_dockable_get_state (dockable) == GSCHEM_DOCKABLE_STATE_DIALOG)
      gschem_dockable_hide (dockable);
    else {
      gtk_widget_grab_focus (dockable->w_current->drawing_area);
      x_window_present (dockable->w_current);
    }
    return TRUE;
  }

  if (event->keyval == GDK_KEY_Escape &&
      (event->state & gtk_accelerator_get_default_mod_mask ()) == 0) {
    if (gschem_dockable_get_state (dockable) == GSCHEM_DOCKABLE_STATE_DIALOG)
      gschem_dockable_hide (dockable);
    else {
      gtk_widget_grab_focus (dockable->w_current->drawing_area);
      x_window_present (dockable->w_current);
    }

    if (dockable->cancellable)
      GSCHEM_DOCKABLE_GET_CLASS (dockable)->cancel (dockable);
    return TRUE;
  }

  return FALSE;
}


static gboolean
callback_window_delete_event (GtkWidget *widget,
                              GdkEvent *event,
                              GschemDockable *dockable)
{
  GschemDockableState state = gschem_dockable_get_state (dockable);
  g_return_val_if_fail (state == GSCHEM_DOCKABLE_STATE_DIALOG ||
                        state == GSCHEM_DOCKABLE_STATE_WINDOW, FALSE);

  gschem_dockable_hide (dockable);

  if (state == GSCHEM_DOCKABLE_STATE_DIALOG && dockable->cancellable)
    GSCHEM_DOCKABLE_GET_CLASS (dockable)->cancel (dockable);

  return TRUE;
}


static void
callback_hide_button_clicked (GtkWidget *button,
                              GschemDockable *dockable)
{
  gschem_dockable_hide (dockable);
}


static void
callback_cancel_button_clicked (GtkWidget *button,
                                GschemDockable *dockable)
{
  gschem_dockable_hide (dockable);

  if (dockable->cancellable)
    GSCHEM_DOCKABLE_GET_CLASS (dockable)->cancel (dockable);
}


static void
callback_help_button_clicked (GtkWidget *button,
                              GschemDockable *dockable)
{
  g_return_if_fail (dockable->help_page != NULL);

  scm_dynwind_begin (0);
  g_dynwind_window (dockable->w_current);
  g_scm_eval_protected (scm_list_2 (
                          scm_variable_ref (
                            scm_c_public_variable ("gschem gschemdoc",
                                                   "show-wiki")),
                          scm_from_utf8_string (dockable->help_page)),
                        SCM_UNDEFINED);
  scm_dynwind_end ();
}


static void
callback_dock_item_toggled (GtkWidget *check_menu_item,
                            GschemDockable *dockable)
{
  gboolean active = gtk_check_menu_item_get_active (
                      GTK_CHECK_MENU_ITEM (check_menu_item));

  GschemDockableState current_state = gschem_dockable_get_state (dockable);
  GschemDockableState target_state;

  if (check_menu_item == dockable->dock_left_item)
    target_state = GSCHEM_DOCKABLE_STATE_DOCKED_LEFT;
  else if (check_menu_item == dockable->dock_bottom_item)
    target_state = GSCHEM_DOCKABLE_STATE_DOCKED_BOTTOM;
  else if (check_menu_item == dockable->dock_right_item)
    target_state = GSCHEM_DOCKABLE_STATE_DOCKED_RIGHT;
  else
    g_assert_not_reached ();

  if (active && current_state != target_state)
    gschem_dockable_set_state (dockable, target_state);
  else if (!active && current_state == target_state)
    gschem_dockable_hide (dockable);
}


static void
callback_menu_item_activate (GtkWidget *menu_item,
                             GschemDockable *dockable)
{
  if (menu_item == dockable->detach_item)
    gschem_dockable_set_state (dockable, GSCHEM_DOCKABLE_STATE_WINDOW);
  else if (menu_item == dockable->move_left_item)
    gschem_dockable_set_state (dockable, GSCHEM_DOCKABLE_STATE_DOCKED_LEFT);
  else if (menu_item == dockable->move_bottom_item)
    gschem_dockable_set_state (dockable, GSCHEM_DOCKABLE_STATE_DOCKED_BOTTOM);
  else if (menu_item == dockable->move_right_item)
    gschem_dockable_set_state (dockable, GSCHEM_DOCKABLE_STATE_DOCKED_RIGHT);
  else if (menu_item == dockable->close_item)
    gschem_dockable_set_state (dockable, GSCHEM_DOCKABLE_STATE_HIDDEN);
  else
    g_assert_not_reached ();
}


static gboolean
callback_event_box_button_press_event (GtkWidget *widget,
                                       GdkEventButton *event,
                                       GschemDockable *dockable)
{
  if (event->button == 2 && event->type == GDK_BUTTON_PRESS) {
    gschem_dockable_hide (dockable);
    return TRUE;
  }

  if (event->button == 3 && event->type == GDK_BUTTON_PRESS) {
    if (dockable->menu == NULL)
      create_menu (dockable);

    GschemDockableState state = gschem_dockable_get_state (dockable);
    gtk_widget_set_sensitive (dockable->detach_item,
                              state != GSCHEM_DOCKABLE_STATE_WINDOW);
    gtk_widget_set_sensitive (dockable->move_left_item,
                              state != GSCHEM_DOCKABLE_STATE_DOCKED_LEFT);
    gtk_widget_set_sensitive (dockable->move_bottom_item,
                              state != GSCHEM_DOCKABLE_STATE_DOCKED_BOTTOM);
    gtk_widget_set_sensitive (dockable->move_right_item,
                              state != GSCHEM_DOCKABLE_STATE_DOCKED_RIGHT);
    gtk_widget_set_sensitive (dockable->close_item,
                              state != GSCHEM_DOCKABLE_STATE_HIDDEN);

    gtk_menu_popup (GTK_MENU (dockable->menu), NULL, NULL, NULL, NULL,
                    event->button, event->time);
    return TRUE;
  }

  return FALSE;
}


static gboolean
callback_main_window_focus_in_event (GtkWidget *widget,
                                     GdkEvent *event,
                                     GschemToplevel *w_current)
{
  static gboolean initialized = FALSE;

  for (GList *l = w_current->dockables; l != NULL; l = l->next) {
    GschemDockable *dockable = GSCHEM_DOCKABLE (l->data);
    if (gschem_dockable_get_state (dockable) == GSCHEM_DOCKABLE_STATE_DIALOG)
      gschem_dockable_set_state (dockable, GSCHEM_DOCKABLE_STATE_WINDOW);
  }

  if (!initialized) {
    initialized = TRUE;
    restore_detached_dockables (w_current);
  }

  return FALSE;
}


static void
callback_notebook_page_added (GtkWidget *notebook,
                              GtkWidget *child,
                              guint page_num,
                              GschemToplevel *w_current)
{
  GschemDockable *dockable = get_dockable_by_widget (w_current, child);
  g_return_if_fail (dockable != NULL);

  update_menu_items (dockable);
  save_state (dockable);
}


static void
callback_notebook_page_added_removed_reordered (GtkWidget *notebook,
                                                GtkWidget *child,
                                                guint page_num,
                                                GschemToplevel *w_current)
{
  save_page_order (w_current, GTK_NOTEBOOK (notebook));

  /* show or hide notebook as appropriate */
  if (gtk_notebook_get_n_pages (GTK_NOTEBOOK (notebook)) == 0)
    gtk_widget_hide (notebook);
  else
    gtk_widget_show (notebook);
}


static void
callback_notebook_switch_page (GtkWidget *notebook,
                               GtkWidget *arg1,
                               guint arg2,
                               GschemToplevel *w_current)
{
  GschemDockable *dockable = get_dockable_by_widget (w_current, arg1);
  g_return_if_fail (dockable != NULL);

  save_current_page (w_current, GTK_NOTEBOOK (notebook), dockable);
}


static GtkNotebook *
callback_notebook_create_window (GtkWidget *notebook,
                                 GtkWidget *page,
                                 gint x, gint y,
                                 GschemToplevel *w_current)
{
  GschemDockable *dockable = get_dockable_by_widget (w_current, page);
  g_return_val_if_fail (dockable != NULL, NULL);

  /* This is kind of a hack: Write the desired window position to the
     configuration so it is restored when detaching */

  EdaConfig *cfg = eda_config_get_user_context ();
  if (cfg != NULL) {
    eda_config_set_int (cfg, dockable->group_name, "x", x);
    eda_config_set_int (cfg, dockable->group_name, "y", y);
  }

  gschem_dockable_detach (dockable, FALSE);
  return NULL;
}


static gboolean
callback_notebook_key_press_event (GtkWidget *notebook,
                                   GdkEventKey *event,
                                   GschemToplevel *w_current)
{
  gint page_num = gtk_notebook_get_current_page (GTK_NOTEBOOK (notebook));
  GtkWidget *widget = gtk_notebook_get_nth_page (GTK_NOTEBOOK (notebook),
                                                 page_num);
  GschemDockable *dockable = get_dockable_by_widget (w_current, widget);
  g_return_val_if_fail (dockable != NULL, FALSE);

  if ((event->keyval == GDK_KEY_Return ||
       event->keyval == GDK_KEY_ISO_Enter ||
       event->keyval == GDK_KEY_KP_Enter) &&
      (event->state & gtk_accelerator_get_default_mod_mask ()) == 0) {
    gtk_widget_grab_focus (w_current->drawing_area);
    return TRUE;
  }

  if (event->keyval == GDK_KEY_Escape &&
      (event->state & gtk_accelerator_get_default_mod_mask ()) == 0) {
    gtk_widget_grab_focus (w_current->drawing_area);
    if (GSCHEM_DOCKABLE_GET_CLASS (dockable)->cancel != NULL)
      GSCHEM_DOCKABLE_GET_CLASS (dockable)->cancel (dockable);
    return TRUE;
  }

  return FALSE;
}


static gboolean
callback_notebook_button_press_event (GtkWidget *notebook,
                                      GdkEventButton *event,
                                      GschemToplevel *w_current)
{
  if (event->button != 3 || event->type != GDK_BUTTON_PRESS)
    return FALSE;

  gpointer data;
  gdk_window_get_user_data (event->window, &data);
  if (data != notebook)
    return FALSE;

  GtkWidget *menu = notebook == w_current->left_notebook ?
                      w_current->left_docking_area_menu :
                    notebook == w_current->bottom_notebook ?
                      w_current->bottom_docking_area_menu :
                    notebook == w_current->right_notebook ?
                      w_current->right_docking_area_menu : NULL;
  g_return_val_if_fail (menu != NULL, FALSE);
  gtk_menu_popup (GTK_MENU (menu), NULL, NULL, NULL, NULL,
                  event->button, event->time);

  return TRUE;
}
