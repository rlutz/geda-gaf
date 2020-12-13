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

/*! \file x_controlfd.c
 * \brief Remote-control gschem via a file descriptor.
 *
 * This feature allows project managers like Igor2's "genxproj" to
 * issue certain actions in gschem.  It is implemented as a plain text
 * protocol and can be enabled via the command-line options
 * `--control-fd=stdin' or `--control-fd=FD'.
 *
 * See \ref help_string for a list of commands.
 */

#include <config.h>

#include "gschem.h"

#include <glib-unix.h>

static const char *help_string =
  "The following commands are supported:\n"
  "\n"
  "    visit FILE\n"
  "    save FILE\n"
  "    save-all\n"
  "    revert FILE\n"
  "    close FILE\n"
  "    patch-filename FILE PATCHFILE\n"
  "    import-patch FILE [PATCHFILE]\n"
  "    quit\n"
  "    help\n"
  "\n"
  "File paths must be absolute, except for patch filenames which are resolved\n"
  "relative to the main file.  Arguments are separated by spaces; spaces and\n"
  "backslashes inside arguments must be escaped with a backslash.\n";

static int control_fd = -1;
static guint tag = 0;
static FILE *stream = NULL;

static gboolean can_read (gint fd, GIOCondition condition, gpointer user_data);


void
x_controlfd_parsearg (char *optarg)
{
  long int val;
  char *endptr = NULL;

  if (strcmp (optarg, "stdin") == 0) {
    control_fd = STDIN_FILENO;
    return;
  }

  errno = 0;
  val = strtol (optarg, &endptr, 10);

  if ((errno != 0 && val == 0) ||
      (errno == ERANGE && (val == LONG_MAX || val == LONG_MIN))) {
    fprintf (stderr, "%s: strtol: \"%s\": %s\n",
             g_get_prgname (), optarg, strerror (errno));
    exit (EXIT_FAILURE);
  }

  if (endptr == optarg || *endptr != '\0') {
    fprintf (stderr, "%s: \"%s\": Not a valid integer\n",
             g_get_prgname (), optarg);
    exit (EXIT_FAILURE);
  }

  if (val < 0 || val > INT_MAX) {
    fprintf (stderr, "%s: \"%s\": File descriptor out of range\n",
             g_get_prgname (), optarg);
    exit (EXIT_FAILURE);
  }

  control_fd = val;
}


void
x_controlfd_init (void)
{
  if (control_fd != -1) {
    stream = fdopen (control_fd, "r");
    if (stream == NULL)
      g_warning (_("Can't open control fd %d: %s\n"),
                 control_fd, strerror (errno));
  }

  if (stream != NULL)
    tag = g_unix_fd_add (control_fd, G_IO_IN, can_read, NULL);
}


void
x_controlfd_free (void)
{
  if (tag != 0) {
    (void) g_source_remove (tag);
    tag = 0;
  }
}


/*! \brief Search for a page by filename, optionally opening it.
 *
 * Searches all open windows for a page with filename \a arg, and
 * return the found window and page.  The filename must be absolute
 * and is normalized in the normal manner.  If the page wasn't found
 * and \a open_if_not_found is \c TRUE, open the file in the last
 * openend window.
 *
 * \returns whether a page has been found (or opened)
 */
static gboolean
find_page (GschemToplevel **w_current_return, PAGE **page_return,
           const gchar *arg, gboolean open_if_not_found)
{
  gchar *fn;
  GError *err = NULL;

  GschemToplevel *w_current = NULL;
  PAGE *page;

  if (!g_path_is_absolute (arg)) {
    fprintf (stderr, "Path must be absolute\n");
    return FALSE;
  }

  fn = f_normalize_filename (arg, &err);
  if (fn == NULL) {
    fprintf (stderr, "%s: %s\n", arg, err->message);
    return FALSE;
  }

  for (const GList *l = global_window_list; l != NULL; l = l->next) {
    w_current = GSCHEM_TOPLEVEL (l->data);
    page = s_page_search (gschem_toplevel_get_toplevel (w_current), fn);
    if (page == NULL || page->is_untitled)
      continue;

    *w_current_return = w_current;
    *page_return = page;
    g_free (fn);
    return TRUE;
  }

  if (!open_if_not_found) {
    fprintf (stderr, "%s: File hasn't been opened\n", fn);
    g_free (fn);
    return FALSE;
  }

  /* arbitrarily use last window to open file */
  g_return_val_if_fail (w_current != NULL, FALSE);
  page = x_highlevel_open_page (w_current, fn);
  g_free (fn);
  if (page == NULL)
    return FALSE;

  *w_current_return = w_current;
  *page_return = page;
  return TRUE;
}


/*! \brief Split argument string into individual tokens.
 *
 * Modifies \a buf in place.
 *
 * On error, prints a message and doesn't change the return arguments.
 */
static void
split_args (gchar ***args_return, gint *count_return, gchar *buf)
{
  GSList *tokens = NULL;
  size_t len = strlen (buf);
  size_t skip = 0;

  do {
    size_t sp = strchrnul (buf + skip, ' ') - buf;
    size_t bs = strchrnul (buf + skip, '\\') - buf;

    if (sp < bs) {
      if (sp != 0)
        tokens = g_slist_prepend (tokens, g_strndup (buf, sp));
      buf += sp + 1;
      len -= sp + 1;
      skip = 0;
    } else if (bs < sp) {
      if (buf[bs + 1] != ' ' && buf[bs + 1] != '\\') {
        fprintf (stderr, "Backslash may only be followed by space or another "
                         "backslash\n");
        g_slist_free (tokens);
        return;
      }
      memmove (buf + bs, buf + bs + 1, len - bs);  /* include trailing NUL */
      skip = bs + 1;
    } else {
      if (buf[0] != '\0')
        tokens = g_slist_prepend (tokens, g_strdup (buf));
      break;
    }
  } while (1);

  gint count = g_slist_length (tokens);
  gchar **args = g_new (char *, count);

  GSList *l = tokens;
  for (unsigned int i = 0; i < count; i++) {
    args[count - i - 1] = (char *) l->data;
    l = l->next;
  }

  g_slist_free (tokens);

  *args_return = args;
  *count_return = count;
}


/*! \brief Parse and execute the command passed in \a buf.
 *
 * Modifies \a buf in place.
 */
static void
process_command (gchar *buf)
{
  gchar **args = NULL;
  gint count = 0;

  split_args (&args, &count, buf);

  if (count == 0) {
    /* empty line */
    g_free (args);
    return;
  }

  GschemToplevel *w_current;
  PAGE *page;

  if (strcmp (args[0], "visit") == 0) {
    if (count != 2)
      fprintf (stderr, "Command usage: visit FILE\n");
    else if (find_page (&w_current, &page, args[1], TRUE)) {
      x_window_set_current_page (w_current, page);
      x_window_present (w_current);
    }
  }

  else if (strcmp (args[0], "save") == 0) {
    if (count != 2)
      fprintf (stderr, "Command usage: save FILE\n");
    else if (find_page (&w_current, &page, args[1], FALSE))
      x_highlevel_save_page (w_current, page);
  }

  else if (strcmp (args[0], "save-all") == 0) {
    if (count != 1)
      fprintf (stderr, "Command usage: save-all\n");
    else
      for (const GList *l = global_window_list; l != NULL; l = l->next)
        x_highlevel_save_all (GSCHEM_TOPLEVEL (l->data));
  }

  else if (strcmp (args[0], "revert") == 0) {
    if (count != 2)
      fprintf (stderr, "Command usage: revert FILE\n");
    else if (find_page (&w_current, &page, args[1], FALSE))
      x_highlevel_revert_page (w_current, page);
  }

  else if (strcmp (args[0], "close") == 0) {
    if (count != 2)
      fprintf (stderr, "Command usage: close FILE\n");
    else if (find_page (&w_current, &page, args[1], FALSE))
      x_highlevel_close_page (w_current, page);
  }

  else if (strcmp (args[0], "patch-filename") == 0) {
    if (count != 3)
      fprintf (stderr, "Command usage: patch-filename FILE PATCHFILE\n");
    else if (find_page (&w_current, &page, args[1], FALSE)) {
      g_free (page->patch_filename);
      if (g_path_is_absolute (args[2]))
        page->patch_filename = g_strdup (args[2]);
      else {
        gchar *dir = g_path_get_dirname (page->page_filename);
        page->patch_filename = g_build_filename (dir, args[2], NULL);
        g_free (dir);
      }
    }
  }

  else if (strcmp (args[0], "import-patch") == 0) {
    if (count != 2 && count != 3)
      fprintf (stderr, "Command usage: import-patch FILE [PATCHFILE]\n");
    else if (find_page (&w_current, &page, args[1], FALSE)) {
      if (count == 3) {
        g_free (page->patch_filename);
        if (g_path_is_absolute (args[2]))
          page->patch_filename = g_strdup (args[2]);
        else {
          gchar *dir = g_path_get_dirname (page->page_filename);
          page->patch_filename = g_build_filename (dir, args[2], NULL);
          g_free (dir);
        }
      }
      if (page->patch_filename == NULL)
        fprintf (stderr, "No patch filename given\n");
      else
        x_patch_do_import (w_current, page);
    }
  }

  else if (strcmp (args[0], "quit") == 0) {
    if (count != 1)
      fprintf (stderr, "Command usage: quit\n");
    else
      x_window_close_all (NULL);
  }

  else if (strcmp (args[0], "help") == 0 ||
           strcmp (args[0], "h") == 0 ||
           strcmp (args[0], "?") == 0) {
    if (count != 1)
      fprintf (stderr, "Command usage: help\n");
    else
      fprintf (stderr, "%s", help_string);
  }

  else
    fprintf (stderr, "Unknown command \"%s\"\n", args[0]);

  for (unsigned int i = 0; i < count; i++)
    g_free (args[i]);
  g_free (args);
}


/*! \brief Called when \c control_fd becomes ready to read.
 *
 * \returns whether the function expects to be called again
 */
static gboolean
can_read (gint fd, GIOCondition condition, gpointer user_data)
{
  char *buf = NULL;
  size_t bufsize = 0;
  ssize_t len;

  errno = 0;
  len = getline (&buf, &bufsize, stream);

  if (errno != 0) {
    g_warning (_("Can't read from control fd %d: %s\n"),
               control_fd, strerror (errno));
    goto error;
  }

  if (len == -1) {
    g_warning (_("Received EOF on control fd %d\n"), control_fd);
    goto error;
  }

  if (len == 0) {
    g_warning (_("Read zero bytes from control fd %d\n"), control_fd);
    goto error;
  }

  buf[len - 1] = '\0';

  process_command (buf);

  free (buf);
  return TRUE;

error:
  free (buf);
  fclose (stream);
  tag = 0;
  return FALSE;
}
