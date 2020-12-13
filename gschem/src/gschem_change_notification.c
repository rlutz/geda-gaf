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

/*! \file gschem_change_notification.c
 * \brief Manage a "file changed on disk" notification bar.
 *
 * The class \ref GschemChangeNotification contains the common logic
 * for the info bars which are shown at the top of the main window if
 * the current file or its back-annotation patch have been changed on
 * disk.  It creates the widget hierarchy for an info bar, shows/hides
 * the bar on file system updates, and emits the "response" signal
 * when the user either accepts or rejects the suggested action.
 *
 * Typically, it would be used as follows:
 *
 * \code
 * GschemChangeNotification *change_notification =
 *   g_object_new (GSCHEM_TYPE_CHANGE_NOTIFICATION,
 *                 "message-type", GTK_MESSAGE_QUESTION,
 *                 "markup",       _("The file has changed on disk.\n\n"
 *                                   "Do you want to perform some action?"),
 *                 "button-label", _("_Perform action"),
 *                 NULL);
 * g_signal_connect (change_notification, "response",
 *                   G_CALLBACK (handle_response), NULL);
 * gtk_box_pack_start (..., change_notification->info_bar, FALSE, FALSE, 0);
 *
 * // ...
 *
 * struct stat buf;
 * if (stat (filename, &buf) == -1)
 *   g_object_set (change_notification,
 *                 "path",            filename,
 *                 "has-known-mtime", FALSE,
 *                 NULL);
 * else
 *   g_object_set (change_notification,
 *                 "path",            filename,
 *                 "has-known-mtime", TRUE,
 *                 "known-mtime",     &buf.st_mtim,
 *                 NULL);
 * \endcode
 *
 * It is preferable to set the path and mtime information using
 * \c g_object_set as this function queues the property notifications
 * and only emits them after all properties have been set.
 */

#include <config.h>

#include <sys/types.h>
#include <sys/stat.h>

#ifdef HAVE_LIBFAM
#include <fam.h>
#endif

#include "gschem.h"
#include "../include/gschem_change_notification.h"


enum {
  PROP_GSCHEM_TOPLEVEL = 1,     /* GschemToplevel * */
  PROP_GSCHEM_PAGE,             /* PAGE *           */
  PROP_PATH,                    /* gchar *          */
  PROP_HAS_KNOWN_MTIME,         /* gboolean         */
  PROP_KNOWN_MTIME,             /* struct timespec  */
  PROP_MESSAGE_TYPE,            /* GtkMessageType   */
  PROP_MARKUP,                  /* gchar *          */
  PROP_BUTTON_STOCK_ID,         /* gchar *          */
  PROP_BUTTON_LABEL,            /* gchar *          */
  N_PROPERTIES
};

enum {
  RESPONSE,
  LAST_SIGNAL
};

static gpointer parent_class = NULL;
static GParamSpec *properties[N_PROPERTIES] = { NULL, };
static guint signals[LAST_SIGNAL] = { 0, };

static void class_init (GschemChangeNotificationClass *class);
static void instance_init (GschemChangeNotification *chnot);

static void constructed (GObject *object);
static void dispose (GObject *object);
static void set_property (GObject *object, guint property_id,
                          const GValue *value, GParamSpec *pspec);
static void get_property (GObject *object, guint property_id,
                          GValue *value, GParamSpec *pspec);
static void notify (GObject *object, GParamSpec *pspec);



static inline gboolean
timespec_ge (struct timespec *a, struct timespec *b)
{
  return a->tv_sec >= b->tv_sec ||
        (a->tv_sec == b->tv_sec && a->tv_nsec >= b->tv_nsec);
}

static GschemTimespec *
timespec_copy (const GschemTimespec *timespec)
{
  g_return_val_if_fail (timespec != NULL, NULL);

  GschemTimespec *copy = g_slice_new (GschemTimespec);
  *copy = *timespec;
  return copy;
}

static void
timespec_free (GschemTimespec *timespec)
{
  g_return_if_fail (timespec != NULL);
  g_slice_free (GschemTimespec, timespec);
}

GType
gschem_timespec_get_type (void)
{
  static GType type = 0;

  if (type == 0)
    type = g_boxed_type_register_static (
      g_intern_static_string ("GschemTimespec"),
      (GBoxedCopyFunc) timespec_copy,
      (GBoxedFreeFunc) timespec_free);

  return type;
}


/******************************************************************************/


static void
response_callback (GtkWidget *info_bar, gint response_id, gpointer user_data)
{
  g_signal_emit (GSCHEM_CHANGE_NOTIFICATION (user_data),
                 signals[RESPONSE], 0, response_id);
}


static void
update_current_mtime (GschemChangeNotification *chnot)
{
  struct stat buf;

  if (chnot->path == NULL || stat (chnot->path, &buf) == -1) {
    chnot->has_current_mtime = FALSE;
    memset (&chnot->current_mtime, 0, sizeof chnot->current_mtime);
  } else {
    chnot->has_current_mtime = TRUE;
    chnot->current_mtime = buf.st_mtim;
  }
}


static void
update_visibility (GschemChangeNotification *chnot)
{
  gtk_widget_set_visible (chnot->info_bar,
                          chnot->has_current_mtime && (
                            !chnot->has_known_mtime ||
                            !timespec_ge (&chnot->known_mtime,
                                          &chnot->current_mtime)));
}


static void
fam_event (const gchar *path, unsigned int code, gpointer user_data)
{
  GschemChangeNotification *chnot = GSCHEM_CHANGE_NOTIFICATION (user_data);

  update_current_mtime (chnot);
  update_visibility (chnot);
}


/******************************************************************************/


GType
gschem_change_notification_get_type ()
{
  static GType type = 0;

  if (type == 0) {
    static const GTypeInfo info = {
      sizeof (GschemChangeNotificationClass),
      NULL,                                     /* base_init */
      NULL,                                     /* base_finalize */
      (GClassInitFunc) class_init,
      NULL,                                     /* class_finalize */
      NULL,                                     /* class_data */
      sizeof (GschemChangeNotification),
      0,                                        /* n_preallocs */
      (GInstanceInitFunc) instance_init,
      NULL                                      /* value_table */
    };

    type = g_type_register_static (G_TYPE_OBJECT,
                                   "GschemChangeNotification",
                                   &info, 0);
  }

  return type;
}


static void
class_init (GschemChangeNotificationClass *class)
{
  G_OBJECT_CLASS (class)->constructed = constructed;
  G_OBJECT_CLASS (class)->dispose = dispose;

  G_OBJECT_CLASS (class)->set_property = set_property;
  G_OBJECT_CLASS (class)->get_property = get_property;
  G_OBJECT_CLASS (class)->notify = notify;

  parent_class = g_type_class_peek_parent (class);


  properties[PROP_GSCHEM_TOPLEVEL] =
    g_param_spec_pointer ("gschem-toplevel",
                          "",
                          "",
                          G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE);
  properties[PROP_GSCHEM_PAGE] =
    g_param_spec_pointer ("gschem-page",
                          "",
                          "",
                          G_PARAM_READWRITE);

  properties[PROP_PATH] =
    g_param_spec_string ("path",
                         "",
                         "",
                         NULL,
                         G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY);
  properties[PROP_HAS_KNOWN_MTIME] =
    g_param_spec_boolean ("has-known-mtime",
                          "",
                          "",
                          FALSE,
                          G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY);
  properties[PROP_KNOWN_MTIME] =
    g_param_spec_boxed ("known-mtime",
                        "",
                        "",
                        GSCHEM_TYPE_TIMESPEC,
                        G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY);

  properties[PROP_MESSAGE_TYPE] =
    g_param_spec_enum ("message-type",
                       "",
                       "",
                       GTK_TYPE_MESSAGE_TYPE,
                       GTK_MESSAGE_INFO,
                       G_PARAM_WRITABLE);
  properties[PROP_MARKUP] =
    g_param_spec_string ("markup",
                         "",
                         "",
                         NULL,
                         G_PARAM_WRITABLE);
  properties[PROP_BUTTON_STOCK_ID] =
    g_param_spec_string ("button-stock-id",
                         "",
                         "",
                         NULL,
                         G_PARAM_WRITABLE);
  properties[PROP_BUTTON_LABEL] =
    g_param_spec_string ("button-label",
                         "",
                         "",
                         NULL,
                         G_PARAM_WRITABLE);

  g_object_class_install_properties (G_OBJECT_CLASS (class),
                                     N_PROPERTIES, properties);

  signals[RESPONSE] =
    g_signal_new ("response",
                  G_OBJECT_CLASS_TYPE (class),
                  G_SIGNAL_RUN_LAST,
                  0, NULL, NULL,
                  g_cclosure_marshal_VOID__INT,
                  G_TYPE_NONE, 1,
                  G_TYPE_INT);
}


static void
instance_init (GschemChangeNotification *chnot)
{
}


static void
constructed (GObject *object)
{
  GschemChangeNotification *chnot = GSCHEM_CHANGE_NOTIFICATION (object);

  G_OBJECT_CLASS (parent_class)->constructed (object);

  chnot->info_bar = gtk_info_bar_new ();
  gtk_widget_set_no_show_all (chnot->info_bar, TRUE);
  g_signal_connect (chnot->info_bar, "response",
                    G_CALLBACK (response_callback), chnot);

  GtkWidget *image = gtk_image_new_from_stock (GTK_STOCK_DIALOG_QUESTION,
                                               GTK_ICON_SIZE_DIALOG);
  gtk_widget_show (image);

  chnot->label = gtk_label_new ("");
  gtk_misc_set_alignment (GTK_MISC (chnot->label), 0., .5);
  gtk_widget_show (chnot->label);

  GtkWidget *content_area =
    gtk_info_bar_get_content_area (GTK_INFO_BAR (chnot->info_bar));
  gtk_box_pack_start (GTK_BOX (content_area), image, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (content_area), chnot->label, TRUE, TRUE, 0);

  chnot->button = gtk_button_new ();
  gtk_button_set_use_underline (GTK_BUTTON (chnot->button), TRUE);
  gtk_widget_set_can_default (chnot->button, TRUE);
  gtk_widget_show (chnot->button);
  gtk_info_bar_add_action_widget (GTK_INFO_BAR (chnot->info_bar),
                                  chnot->button,
                                  GTK_RESPONSE_ACCEPT);

  gtk_info_bar_add_button (GTK_INFO_BAR (chnot->info_bar),
                           GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL);
}


static void
dispose (GObject *object)
{
  GschemChangeNotification *chnot = GSCHEM_CHANGE_NOTIFICATION (object);

  g_clear_pointer (&chnot->path, g_free);
  g_clear_pointer (&chnot->fam_handle, x_fam_unmonitor);

  G_OBJECT_CLASS (parent_class)->dispose (object);
}


static void
set_property (GObject *object,
              guint property_id,
              const GValue *value,
              GParamSpec *pspec)
{
  GschemChangeNotification *chnot = GSCHEM_CHANGE_NOTIFICATION (object);
  const gchar *path;
  gboolean has_known_mtime;

  switch (property_id) {
    case PROP_GSCHEM_TOPLEVEL:
      chnot->w_current = GSCHEM_TOPLEVEL (g_value_get_pointer (value));
      break;

    case PROP_GSCHEM_PAGE:
      chnot->page = (PAGE *) g_value_get_pointer (value);
      break;

    case PROP_PATH:
      path = g_value_get_string (value);
      if (path != NULL && *path == '\0')
        path = NULL;

      if (g_strcmp0 (path, chnot->path) != 0) {
        g_clear_pointer (&chnot->path, g_free);
        g_clear_pointer (&chnot->fam_handle, x_fam_unmonitor);

        if (path != NULL) {
          chnot->path = g_strdup (path);
          chnot->fam_handle = x_fam_monitor (path, NULL, fam_event, chnot);
        }

        update_current_mtime (chnot);
        g_object_notify_by_pspec (object, pspec);
      }
      break;

    case PROP_HAS_KNOWN_MTIME:
      has_known_mtime = g_value_get_boolean (value);
      if (has_known_mtime != chnot->has_known_mtime) {
        chnot->has_known_mtime = has_known_mtime;
        g_object_notify_by_pspec (object, pspec);
      }
      break;

    case PROP_KNOWN_MTIME:
      chnot->known_mtime = *((struct timespec *) g_value_get_boxed (value));
      g_object_notify_by_pspec (object, pspec);
      break;

    case PROP_MESSAGE_TYPE:
      gtk_info_bar_set_message_type (GTK_INFO_BAR (chnot->info_bar),
                                     g_value_get_enum (value));
      break;

    case PROP_MARKUP:
      gtk_label_set_markup (GTK_LABEL (chnot->label),
                            g_value_get_string (value));
      break;

    case PROP_BUTTON_STOCK_ID:
      gtk_button_set_image (GTK_BUTTON (chnot->button),
                            gtk_image_new_from_stock (
                              g_value_get_string (value),
                              GTK_ICON_SIZE_BUTTON));
      break;

    case PROP_BUTTON_LABEL:
      gtk_button_set_label (GTK_BUTTON (chnot->button),
                            g_value_get_string (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
  }
}


static void
get_property (GObject *object,
              guint property_id,
              GValue *value,
              GParamSpec *pspec)
{
  GschemChangeNotification *chnot = GSCHEM_CHANGE_NOTIFICATION (object);

  switch (property_id) {
    case PROP_GSCHEM_TOPLEVEL:
      g_value_set_pointer (value, chnot->w_current);
      break;

    case PROP_GSCHEM_PAGE:
      g_value_set_pointer (value, chnot->page);
      break;

    case PROP_PATH:
      g_value_set_string (value, chnot->path);
      break;

    case PROP_HAS_KNOWN_MTIME:
      g_value_set_boolean (value, chnot->has_known_mtime);
      break;

    case PROP_KNOWN_MTIME:
      g_value_set_boxed (value, &chnot->known_mtime);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
  }
}


static void
notify (GObject *object, GParamSpec *pspec)
{
  GschemChangeNotification *chnot = GSCHEM_CHANGE_NOTIFICATION (object);

  if ((pspec == properties[PROP_PATH] ||
       pspec == properties[PROP_HAS_KNOWN_MTIME] ||
       pspec == properties[PROP_KNOWN_MTIME]))
    update_visibility (chnot);

  if (G_OBJECT_CLASS (parent_class)->notify != NULL)
    G_OBJECT_CLASS (parent_class)->notify (object, pspec);
}
