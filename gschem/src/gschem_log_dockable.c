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
/*!
 * \file gschem_log_dockable.c
 *
 * \brief GType class and functions to support the gschem log window.
 */

#include <config.h>

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#ifdef HAVE_FCNTL_H
#include <fcntl.h>
#endif
#ifdef HAVE_STRING_H
#include <string.h>
#endif

#include "gschem.h"

#include "../include/gschem_log_dockable.h"


static gpointer parent_class = NULL;

static void
apply_tag_cb (GtkTextBuffer *buffer, GtkTextTag *tag,
              GtkTextIter *start, GtkTextIter *end,
              GschemLogDockable *dockable);

static void
class_init (GschemLogDockableClass *class);

static GtkTextBuffer*
create_text_buffer();

static GtkWidget *
create_widget (GschemDockable *parent);

static void
dispose (GObject *object);

static void
x_log_message (const gchar *log_domain, GLogLevelFlags log_level, const gchar *message);

static void
log_message (GschemLogDockableClass *klass, const gchar *message, const gchar *style);


/*!
 *  \brief Get the Log class type
 *
 *  \par Function Description
 *
 * On first call, registers the Log class with the GType dynamic type system.
 * On subsequent calls, returns the saved value from first execution.
 * \returns the type identifier for the Log class
 */
GType
gschem_log_dockable_get_type ()
{
  static GType type = 0;

  if (type == 0) {
    static const GTypeInfo info = {
      sizeof(GschemLogDockableClass),
      NULL,                                 /* base_init */
      NULL,                                 /* base_finalize */
      (GClassInitFunc) class_init,
      NULL,                                 /* class_finalize */
      NULL,                                 /* class_data */
      sizeof(GschemLogDockable),
      0,                                    /* n_preallocs */
      NULL,                                 /* instance_init */
    };

    type = g_type_register_static (GSCHEM_TYPE_DOCKABLE,
                                   "GschemLogDockable",
                                   &info,
                                   0);
  }

  return type;
}


/*! \brief Add a message to the status log
 *
 *  \param [in] log_domain
 *  \param [in] log_level The severity of the message
 *  \param [in] message   The message to be displayed
 */
static void
x_log_message (const gchar *log_domain, GLogLevelFlags log_level, const gchar *message)
{
  GschemLogDockableClass *klass = GSCHEM_LOG_DOCKABLE_CLASS (g_type_class_peek_static (GSCHEM_TYPE_LOG_DOCKABLE));
  gchar *style;

  if (log_level & (G_LOG_LEVEL_CRITICAL | G_LOG_LEVEL_ERROR)) {
    style = "critical";
  } else if (log_level & G_LOG_LEVEL_WARNING) {
    style = "warning";
  } else {
    style = "message";
  }

  log_message (klass, message, style);
}


/*!
 *  \brief Add a message to the log window
 *
 *  \par Function Description
 *  \param [in] log The log instance
 *  \param [in] message The message to be logged
 *  \param [in] style   The style to use in the text rendering
 */
static void
log_message (GschemLogDockableClass *klass, const gchar *message, const gchar *style)
{
  GtkTextIter iter;

  g_return_if_fail (klass != NULL);
  g_return_if_fail (klass->buffer != NULL);

  gtk_text_buffer_get_end_iter (klass->buffer, &iter);
  /* Apply the "plain" tag before the level-specific tag in order to
   * reset the formatting */

  if (g_utf8_validate (message, -1, NULL)) {
    gtk_text_buffer_insert_with_tags_by_name (klass->buffer, &iter, message, -1,
                                              "plain", style, NULL);
  } else {
    /* If UTF-8 wasn't valid (due to a system locale encoded filename or
     * other string being included by mistake), log a warning, and print
     * the original message to stderr, where it may be partly intelligible */
    gtk_text_buffer_insert_with_tags_by_name (klass->buffer, &iter,
      _("** Invalid UTF-8 in log message. See stderr or gschem.log.\n"),
                                              -1, "plain", style, NULL);
    fprintf (stderr, "%s", message);
  }
}


/*! \brief callback for tags being applied to the text buffer
 *
 *  Applying tags to the buffer causes all text view widgets to scroll
 *  to the bottom.  Additionally, for high priority tags, the log
 *  dockable is presented to the user.
 *
 *  \param [in] buffer the text buffer triggering the event
 *  \param [in] tag the applied tag
 *  \param [in] start the start of the range the tag is applied to
 *  \param [in] end the end of the range the tag is applied to
 *  \param [in] dockable the dockable to scroll to the bottom
 */
static void
apply_tag_cb (GtkTextBuffer *buffer, GtkTextTag *tag,
              GtkTextIter *start, GtkTextIter *end,
              GschemLogDockable *dockable)
{
  gchar *tag_name;

  g_return_if_fail (buffer != NULL);
  g_return_if_fail (dockable != NULL);
  g_return_if_fail (dockable->viewer != NULL);

  g_object_get (tag, "name", &tag_name, NULL);
  if ((strcmp (tag_name, "critical") == 0 ||
       strcmp (tag_name, "warning") == 0))
    gschem_dockable_present (GSCHEM_DOCKABLE (dockable));
  g_free (tag_name);

  if (gtk_widget_get_realized (GTK_WIDGET (dockable->viewer)))
    gtk_text_view_scroll_to_iter (dockable->viewer, end, 0.0, TRUE, 0.0, 1.0);
}


/*! \brief initialize class
 */
static void
class_init (GschemLogDockableClass *klass)
{
  gchar *contents;
/*   GObjectClass *gobject_class = G_OBJECT_CLASS (klass); */

  klass->buffer = create_text_buffer ();

  /* make it read the content of the current log file */
  /* and add its contents to the dialog */
  contents = s_log_read ();

  /* s_log_read can return NULL if the log file cannot be written to */
  if (contents != NULL) {
    log_message (klass, contents, "old");
    g_free (contents);

    x_log_update_func = x_log_message;
  }

  GSCHEM_DOCKABLE_CLASS (klass)->create_widget = create_widget;
  G_OBJECT_CLASS (klass)->dispose = dispose;

  parent_class = g_type_class_peek_parent (klass);
}


/*! \brief create the text buffer for storing the status log contents
 *
 *  \return a GtkTextBuffer for storing the status log
 */
static GtkTextBuffer*
create_text_buffer()
{
  GtkTextBuffer *buffer = gtk_text_buffer_new (NULL);

  /* Add some tags for highlighting log messages to the buffer */
  gtk_text_buffer_create_tag (buffer,
                              "plain",
                              "foreground", "black",
                              "foreground-set", TRUE,
                              "weight", PANGO_WEIGHT_NORMAL,
                              "weight-set", TRUE,
                              NULL);

  /* The default "message" style is plain */
  gtk_text_buffer_create_tag (buffer, "message", NULL);

  /* "old" messages are in dark grey */
  gtk_text_buffer_create_tag (buffer,
                              "old",
                              "foreground", "#404040",
                              "foreground-set", TRUE,
                              NULL);

  /* "warning" messages are printed in red */
  gtk_text_buffer_create_tag (buffer,
                              "warning",
                              "foreground", "red",
                              "foreground-set", TRUE,
                              NULL);

  /* "critical" messages are bold red */
  gtk_text_buffer_create_tag (buffer,
                              "critical",
                              "foreground", "red",
                              "foreground-set", TRUE,
                              "weight", PANGO_WEIGHT_BOLD,
                              "weight-set", TRUE,
                              NULL);

  return buffer;
}


/*! \brief create widgets
 *
 *  \param [in] parent an instance of the dockable
 */
static GtkWidget *
create_widget (GschemDockable *parent)
{
  GschemLogDockable *dockable = GSCHEM_LOG_DOCKABLE (parent);
  GtkTextIter iter;
  GschemLogDockableClass *klass = GSCHEM_LOG_DOCKABLE_GET_CLASS (dockable);
  GtkWidget *scrolled;

  g_return_val_if_fail (klass != NULL, NULL);
  g_return_val_if_fail (klass->buffer != NULL, NULL);
  g_return_val_if_fail (dockable != NULL, NULL);

  scrolled = gtk_scrolled_window_new (NULL, NULL);

  dockable->viewer = GTK_TEXT_VIEW (g_object_new (GTK_TYPE_TEXT_VIEW,
                                                  /* GtkTextView */
                                                  "buffer",   klass->buffer,
                                                  "editable", FALSE,
                                                  NULL));

  gtk_container_add (GTK_CONTAINER (scrolled), GTK_WIDGET (dockable->viewer));

  g_signal_connect (klass->buffer,
                    "apply-tag",
                    G_CALLBACK (&apply_tag_cb),
                    dockable);

  gtk_text_buffer_get_end_iter (klass->buffer, &iter);

  gtk_widget_show_all (scrolled);
  return scrolled;
}


static void
dispose (GObject *object)
{
  g_signal_handlers_disconnect_by_data (
    GSCHEM_LOG_DOCKABLE_GET_CLASS (object)->buffer, object);

  G_OBJECT_CLASS (parent_class)->dispose (object);
}
