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

#ifdef HAVE_LIBFAM
#include <fcntl.h>
#include <glib-unix.h>
#include <fam.h>

struct request {
  FAMRequest fr;
  gboolean end_exist;  /* whether a FAMEndExist event has been received */

  gpointer user_data;
  void (*exists_event) (const gchar *path, unsigned int code,
                        gpointer user_data);
  void (*regular_event) (const gchar *path, unsigned int code,
                         gpointer user_data);
};

static FAMConnection *fc = NULL;
static guint tag = 0;

static gboolean fam_read (gint fd, GIOCondition condition, gpointer user_data);
#endif


void
x_fam_init ()
{
#ifdef HAVE_LIBFAM
  FAMConnection *new_fc = g_new0 (FAMConnection, 1);

  if (FAMOpen (new_fc) == -1) {
    g_free (new_fc);
    g_warning ("Can't connect to FAM daemon\n");
    return;
  }

  if (fcntl (FAMCONNECTION_GETFD (new_fc), F_SETFD, FD_CLOEXEC) == -1) {
    FAMClose (new_fc);
    g_free (new_fc);
    g_warning ("Couldn't set close-on-exec flag on FAM file descriptor\n");
    return;
  }

  fc = new_fc;
  tag = g_unix_fd_add (FAMCONNECTION_GETFD (fc), G_IO_IN, fam_read, NULL);
#endif
}

void
x_fam_free ()
{
#ifdef HAVE_LIBFAM
  if (fc == NULL)
    return;

  if (FAMClose (fc) == -1)
    g_warning ("FAMClose failed\n");

  (void) g_source_remove (tag);
  tag = 0;

  g_free (fc);
  fc = NULL;
#endif
}

gpointer
x_fam_monitor (const gchar *path,
               void (*exists_event) (const gchar *path, unsigned int code,
                                     gpointer user_data),
               void (*regular_event) (const gchar *path, unsigned int code,
                                      gpointer user_data),
               gpointer user_data)
{
#ifdef HAVE_LIBFAM
  if (fc == NULL)
    return NULL;

  struct request *req = g_new0 (struct request, 1);
  req->user_data = user_data;
  req->exists_event = exists_event;
  req->regular_event = regular_event;

  if (FAMMonitorFile (fc, path, &req->fr, req) == -1) {
    g_free (req);
    g_warning ("FAMMonitorFile failed\n");
    return NULL;
  }

  return req;
#else
  return NULL;
#endif
}

void
x_fam_unmonitor (gpointer handle)
{
#ifdef HAVE_LIBFAM
  struct request *req = handle;

  if (fc == NULL || req == NULL)
    return;

  if (FAMCancelMonitor (fc, &req->fr) == -1)
    g_warning ("FAMCancelMonitor failed\n");
#endif
}

#ifdef HAVE_LIBFAM
static gboolean
fam_read (gint fd, GIOCondition condition, gpointer user_data)
{
  if (fc == NULL)
    return FALSE;

  do {
    int ret = FAMPending (fc);
    if (ret == -1) {
      g_warning ("FAMPending failed\n");
      goto error;
    }
    if (ret == 0)
      /* no more events */
      break;

    FAMEvent fe;
    if (FAMNextEvent (fc, &fe) == -1) {
      g_warning ("FAMNextEvent failed (maybe the FAM daemon crashed?)\n");
      goto error;
    }

    struct request *req = fe.userdata;
    if (fe.code == FAMAcknowledge) {
      g_free (req);
      continue;
    }

    if (req->end_exist) {
      if (req->regular_event != NULL)
        req->regular_event (fe.filename, fe.code, req->user_data);
    } else {
      if (req->exists_event != NULL)
        req->exists_event (fe.filename, fe.code, req->user_data);
    }

    if (fe.code == FAMEndExist)
      req->end_exist = TRUE;
  } while (TRUE);

  return TRUE;

error:
  if (FAMClose (fc) == -1)
    g_warning ("FAMClose failed\n");

  g_free (fc);
  fc = NULL;

  tag = 0;
  return FALSE;
}
#endif
