/* gEDA - GPL Electronic Design Automation
 * gschem - gEDA Schematic Capture
 * Copyright (C) 2015-2020 gEDA Contributors (see ChangeLog for details)
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

/*! \file gschem_patch.c
 * \brief Back-annotation from pcb-rnd.
 *
 * \bug Back-annotation definitively needs automated tests, but there
 *      is currently no easy way to do this.
 */

#include <config.h>
#include "gschem.h"

#define NETATTRIB_DELIMITERS ",; "

static const gboolean debug =
#if DEBUG_PATCH
	TRUE;
#else
	FALSE;
#endif

static void free_patch_line (gschem_patch_line_t *line);


/******************************************************************************/
/*! \section init  Initializing the patch state.                              */

static int
patch_parse (gschem_patch_state_t *st, FILE *f, const char *fn)
{
  char *word = NULL;
  int alloced = 0, used = 0;
  int c, lineno = 1;
  gschem_patch_line_t *current = NULL;

  enum {
    ST_INIT,
    ST_COMMENT,
    ST_OP,
    ST_PRE_STR,
    ST_STR
  } state = ST_INIT;

  g_assert (st->lines == NULL);

  do {
    enum {
      DO_NOP,
      DO_APPEND,
      DO_END_OP,
      DO_END_STR
    } what_to_do = DO_NOP;
    gboolean end_line = FALSE;

    c = fgetc (f);

    switch (state) {
      case ST_INIT:
        switch (c) {
          case '#':
            state = ST_COMMENT;
            break;
          case '\r':
          case '\n':
          case EOF:
          case ' ':
          case '\t':
            break;
          default:
            used = 0;
            what_to_do = DO_APPEND;
            state = ST_OP;
        }
        break;

      case ST_OP:
        switch (c) {
          case '#':
            what_to_do = DO_END_OP;
            end_line = TRUE;
            state = ST_COMMENT;
            break;
          case ' ':
          case '\t':
            what_to_do = DO_END_OP;
            state = ST_PRE_STR;
            break;
          case '\r':
          case '\n':
          case EOF:
            what_to_do = DO_END_OP;
            end_line = TRUE;
            state = ST_INIT;
            break;
          default:
            what_to_do = DO_APPEND;
        }
        break;

      case ST_PRE_STR:
        switch (c) {
          case '#':
            end_line = TRUE;
            state = ST_COMMENT;
            break;
          case ' ':
          case '\t':
            break;
          case '\r':
          case '\n':
          case EOF:
            end_line = TRUE;
            state = ST_INIT;
            break;
          default:
            used = 0;
            what_to_do = DO_APPEND;
            state = ST_STR;
        }
        break;

      case ST_STR:
        switch (c) {
          case '#':
            what_to_do = DO_END_STR;
            end_line = TRUE;
            state = ST_COMMENT;
            break;
          case ' ':
          case '\t':
            what_to_do = DO_END_STR;
            state = ST_PRE_STR;
            break;
          case '\r':
          case '\n':
          case EOF:
            what_to_do = DO_END_STR;
            end_line = TRUE;
            state = ST_INIT;
            break;
          default:
            what_to_do = DO_APPEND;
        }
        break;

      case ST_COMMENT:
        switch (c) {
          case '\r':
          case '\n':
          case EOF:
            state = ST_INIT;
            break;
        }
        break;
    }

    switch (what_to_do) {
      case DO_NOP:
        break;

      case DO_APPEND:
        if (used >= alloced) {
          alloced += 64;
          word = realloc (word, alloced);
        }
        word[used] = c;
        used++;
        break;

      case DO_END_OP:
        if (used >= alloced) {
          alloced += 64;
          word = realloc (word, alloced);
        }
        word[used] = '\0';
        used++;

        if (current != NULL) {
          fprintf (stderr, "%s:%d: Internal error\n", fn, lineno);
          goto error;
        }
        current = g_slice_new0 (gschem_patch_line_t);

        if (strcmp (word, "add_conn") == 0)
          current->op = GSCHEM_PATCH_ADD_CONN;
        else if (strcmp (word, "del_conn") == 0)
          current->op = GSCHEM_PATCH_DEL_CONN;
        else if (strcmp (word, "change_attrib") == 0)
          current->op = GSCHEM_PATCH_CHANGE_ATTRIB;
        else if (strcmp (word, "net_info") == 0)
          current->op = GSCHEM_PATCH_NET_INFO;
        else {
          fprintf (stderr, "%s:%d: Syntax error: unknown opcode `%s'\n",
                           fn, lineno, word);
          goto error;
        }
        used = 0;
        break;

      case DO_END_STR:
        if (used >= alloced) {
          alloced += 64;
          word = realloc (word, alloced);
        }
        word[used] = '\0';
        used++;

        if (*word != '\0') {
          if (current == NULL) {
            fprintf (stderr, "%s:%d: Internal error\n", fn, lineno);
            goto error;
          }
          switch (current->op) {
            case GSCHEM_PATCH_DEL_CONN:
            case GSCHEM_PATCH_ADD_CONN:
              if (current->id == NULL)
                current->id = strdup (word);
              else if (current->arg1.net_name == NULL)
                current->arg1.net_name = strdup (word);
              else {
                fprintf (stderr, "%s:%d: Need two arguments for the "
                                 "connection: netname and pinname\n",
                                 fn, lineno);
                goto error;
              }
              break;
            case GSCHEM_PATCH_CHANGE_ATTRIB:
              if (current->id == NULL)
                current->id = strdup (word);
              else if (current->arg1.attrib_name == NULL)
                current->arg1.attrib_name = strdup (word);
              else if (current->arg2.attrib_val == NULL)
                current->arg2.attrib_val = strdup (word);
              else {
                fprintf (stderr, "%s:%d: Need three arguments for an "
                                 "attrib change: id attr_name attr_val\n",
                                 fn, lineno);
                goto error;
              }
              break;
            case GSCHEM_PATCH_NET_INFO:
              if (current->id == NULL)
                current->id = strdup (word);
              else
                current->arg1.ids =
                  g_list_prepend (current->arg1.ids, strdup (word));
              break;
          }
        }
        used = 0;
        break;
    }

    if (end_line) {
      if (current == NULL) {
        fprintf (stderr, "%s:%d: Internal error\n", fn, lineno);
        goto error;
      }
      switch (current->op) {
        case GSCHEM_PATCH_DEL_CONN:
        case GSCHEM_PATCH_ADD_CONN:
          if (current->id == NULL ||
              current->arg1.net_name == NULL) {
            fprintf (stderr, "%s:%d: Not enough arguments\n", fn, lineno);
            goto error;
          }
          break;
        case GSCHEM_PATCH_CHANGE_ATTRIB:
          if (current->id == NULL ||
              current->arg1.attrib_name == NULL ||
              current->arg2.attrib_val == NULL) {
            fprintf (stderr, "%s:%d: Not enough arguments\n", fn, lineno);
            goto error;
          }
          break;
        case GSCHEM_PATCH_NET_INFO:
          if (current->id == NULL) {
            fprintf (stderr, "%s:%d: Not enough arguments\n", fn, lineno);
            goto error;
          }
          break;
      }
      st->lines = g_list_prepend (st->lines, current);
      current = NULL;
      used = 0;
    }

    if (c == '\n')
      lineno++;
  } while (c != EOF);

  st->lines = g_list_reverse (st->lines);
  if (current != NULL)
    free_patch_line (current);
  free (word);
  return 0;

error:
  g_list_free_full (st->lines, (GDestroyNotify) free_patch_line);
  if (current != NULL)
    free_patch_line (current);
  free (word);
  return -1;
}


static void
debug_print_lines (gschem_patch_state_t *st)
{
  static const char *op_names[] = {
    "disconnect",
    "connect",
    "chnage attribute",
    "net_info"
  };

  for (GList *i = st->lines; i != NULL; i = i->next) {
    gschem_patch_line_t *l = i->data;
    if (l == NULL) {
      fprintf (stderr, "NULL data on list\n");
      continue;
    }
    switch (l->op) {
      case GSCHEM_PATCH_DEL_CONN:
      case GSCHEM_PATCH_ADD_CONN:
        fprintf (stderr, "%s %s %s\n", op_names[l->op], l->id,
                         l->arg1.net_name);
        break;
      case GSCHEM_PATCH_CHANGE_ATTRIB:
        fprintf (stderr, "%s %s %s=%s\n", op_names[l->op], l->id,
                         l->arg1.attrib_name, l->arg2.attrib_val);
        break;
      case GSCHEM_PATCH_NET_INFO:
        fprintf (stderr, "%s %s", op_names[l->op], l->id);
        for (GList *p = l->arg1.ids; p != NULL; p = p->next)
          fprintf (stderr, " %s", (char *) p->data);
        fprintf (stderr, "\n");
        break;
    }
  }
}

/*! \brief Initialize a patch state struct and read a patch file.
 *
 * If reading the patch file fails, \c -1 is returned and \a st is
 * left uninitialized.
 *
 * \returns \c 0 on success, \c -1 on failure
 */
int
gschem_patch_state_init (gschem_patch_state_t *st, const char *fn)
{
  FILE *f;
  int res;

  memset (st, 0, sizeof *st);

  f = fopen (fn, "r");
  if (f == NULL)
    return -1;

  res = patch_parse (st, f, fn);

  if (debug)
    debug_print_lines (st);

  if (res == 0) {
    /* Create hashes for faster lookups avoiding O(objects*patches) */
    st->pins = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, NULL);
    st->comps = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, NULL);
    st->nets = g_hash_table_new (g_str_hash, g_str_equal);
    for (GList *i = st->lines; i != NULL; i = i->next) {
      gschem_patch_line_t *l = i->data;
      if (l->op == GSCHEM_PATCH_NET_INFO) {
        g_hash_table_insert (st->nets, l->id, l->arg1.ids);
        l->arg1.ids = NULL;  /* transfer ownership */
      }
    }
  }

  fclose (f);
  return res;
}


/******************************************************************************/
/*! \section build  Populating the patch state hashes.                        */


/* insert item in a hash table of slists */
static void
build_insert_hash_list (GHashTable *hash, char *full_name, void *item)
{
  GSList *lst = g_hash_table_lookup (hash, full_name);
  g_hash_table_insert (hash, full_name, g_slist_prepend (lst, item));
}

static gschem_patch_pin_t *
alloc_pin (OBJECT *pin_obj, char *net)
{
  gschem_patch_pin_t *p = g_slice_new (gschem_patch_pin_t);
  p->obj = pin_obj;
  p->net = net;
  return p;
}

/*! \brief Make an object known to a patch state.
 *
 * \returns \c 0
 */
int
gschem_patch_state_build (gschem_patch_state_t *st, OBJECT *o)
{
  GList *l;
  gchar *refdes, *pin;

  switch (o->type) {
    case OBJ_COMPLEX:
      refdes = o_attrib_search_object_attribs_by_name (o, "refdes", 0);
      if (refdes == NULL)
        break;

      /* map the component */
      build_insert_hash_list (st->comps, g_strdup (refdes), o);

      /* map pins */
      for (GList *i = o->complex->prim_objs; i != NULL; i = i->next) {
        OBJECT *sub = i->data;
        switch (sub->type) {
          case OBJ_PIN:
            pin = o_attrib_search_object_attribs_by_name (sub, "pinnumber", 0);
            if (pin != NULL) {
              char *full_name;
              full_name = g_strdup_printf ("%s-%s", refdes, pin);
              //printf ("add: '%s' -> '%p' o=%p at=%p p=%p\n",
              //        full_name, sub, o, sub->attached_to, sub->parent);
              //fflush (stdout);
              build_insert_hash_list (st->pins, full_name,
                                      alloc_pin (sub, NULL));
              g_free (pin);
            }
            break;
        }
      }

      /* map net attribute connections */
      l = o_attrib_return_attribs (o);
      for (GList *i = l; i != NULL; i = i->next) {
        OBJECT *attrib = i->data;
        /* I know, I know, I should use o_attrib_get_name_value(), but it'd be
           ridicolous to get everything strdup'd */
        if (attrib->type != OBJ_TEXT)
          continue;

        if (strncmp (attrib->text->string, "net=", 4) == 0) {
          char *net = attrib->text->string + 4;
          char *pinlist = strchr (net, ':');
          char *full_name;
          if (pinlist != NULL) {
            int net_len;
            char *net_name = NULL;

            net_len = pinlist - net;
            if (net_len > 0)
              net_name = g_strndup (net, net_len);
            else
              net_name = NULL;

            /* create a copy of the pin list on which to run strtok_r(3) */
            pinlist = g_strdup(pinlist + 1);

            char *pinno, *saveptr = NULL;
            for (pinno = strtok_r (pinlist, NETATTRIB_DELIMITERS, &saveptr);
                 pinno != NULL;
                 pinno = strtok_r (NULL, NETATTRIB_DELIMITERS, &saveptr)) {
              full_name = g_strdup_printf ("%s-%s", refdes, pinno);
              //printf ("add: '%s' -> '%p';'%s'\n", full_name, o, net_name);
              //fflush (stdout);
              build_insert_hash_list (st->pins, full_name,
                                      alloc_pin (o, g_strdup (net_name)));
            }

            g_free (pinlist);
            g_free (net_name);
          }
        }
      }
      g_list_free (l);

      /* clean up */
      g_free (refdes);
      break;

    /* what to do with nets? */
    case OBJ_NET:
      //printf ("type: '%c'\n", o->type);
      break;

    /* ignore floating pins */
    case OBJ_PIN:
      break;

    /* ignore all graphical objects */
    case OBJ_TEXT:
    case OBJ_LINE:
    case OBJ_PATH:
    case OBJ_BOX:
    case OBJ_CIRCLE:
    case OBJ_PICTURE:
    case OBJ_BUS:
    case OBJ_ARC:
      break;
  }
  return 0;
}


/******************************************************************************/
/*! \section execute  Executing the patch state.                              */


static gboolean
free_key (gpointer key, gpointer value, gpointer user_data)
{
  free (key);
  return TRUE;
}

static gschem_patch_hit_t *
alloc_hit (OBJECT *obj, gchar *loc_name, gchar *action)
{
  gschem_patch_hit_t *hit = g_slice_new (gschem_patch_hit_t);
  hit->object = obj;
  hit->loc_name = loc_name;
  hit->action = action;
  return hit;
}

/*! \brief Get a list of all objects connected to this one (recursively).
 *
 * Gets an open list of objects to be checked and maps all connections
 * of all objects on the list.  The resulting new open list is empty,
 * while the found hash is non-empty.  For each new object on the
 * found hash, the value is determined by calling the user provided
 * hashval() callback.
 *
 * \param [in/out] found  (OBJECT *) -> (value) hash of all objects found
 * \param [in] open       GList of OBJECT's to start the sarch from
 * \param [in] hashval()  a callback that generates the value of the object;
 *                        all object values are NULL if hashval() is NULL
 * \param [in] user_ctx   user context pointer for hashval()
 *
 * \returns the new open list (empty list)
 *
 * \warning The caller must \c g_list_free the returned GList pointer.
 *          Also free the found hash.
 */
static GList *
s_conn_find_all (GHashTable *found, GList *open,
                 void *(*hashval) (void *user_ctx, OBJECT *o), void *user_ctx)
{
  /* iterate by consuming the first element of the list */
  for (GList *i = open; i != NULL; i = open) {
    OBJECT *o = i->data;

    open = g_list_remove (open, o);

    /* ... check if it's not yet found */
    if (g_hash_table_lookup (found, o) == NULL) {
      void *val;
      if (hashval != NULL)
        val = hashval (user_ctx, o);
      else
        val = NULL;
      g_hash_table_insert (found, o, val);
      open = s_conn_return_others (open, o);
    }
  }

  return open;
}

/* return the name of the object and add relevant objects to a
   name->obj hash in user_ctx */
static void *
exec_check_conn_hashval (void *user_ctx, OBJECT *o)
{
  gchar *name = NULL, *tmp;
  GHashTable *name2obj = user_ctx;

  switch (o->type) {
    case OBJ_NET:
      tmp = o_attrib_search_object_attribs_by_name (o, "netname", 0);
      if (tmp != NULL) {
        name = g_strdup_printf ("%c%s", OBJ_NET, tmp);
        g_free (tmp);
        g_hash_table_insert (name2obj, name, o);
      } else
        name = " ";   /* anon net segments are not interesting at all;
                         should be a static string as it doesn't end up
                         on name2obj where we free these strings */
      break;

    case OBJ_PIN:
      if (o->parent != NULL) {
        gchar *oname, *pname;

        oname = o_attrib_search_object_attribs_by_name (o->parent, "refdes", 0);
        if (oname == NULL)
          break;
        pname = o_attrib_search_object_attribs_by_name (o, "pinnumber", 0);
        name = g_strdup_printf ("%c%s-%s", OBJ_PIN, (char *) oname,
                                                    (char *) pname);
        g_free (oname);
        g_free (pname);
        g_hash_table_insert (name2obj, name, o);
      }
      break;
  }
  return name;
}

/* Build a name->object hash of everything connected to a pin */
static GHashTable *
exec_list_conns (OBJECT *pin)
{
  GHashTable *connections = g_hash_table_new (g_str_hash, g_str_equal);
  GHashTable *found = g_hash_table_new (g_direct_hash, NULL);
  GList *open = g_list_prepend (NULL, pin);
  open = s_conn_find_all (found, open, exec_check_conn_hashval, connections);

  g_hash_table_destroy (found);
  g_list_free (open);
  return connections;
}

static void
exec_free_conns (GHashTable *connections)
{
  g_hash_table_foreach_remove (connections, free_key, NULL);
  g_hash_table_destroy (connections);
}

static void
exec_debug_print_conns (GHashTable *connections)
{
  gpointer key, val;
  GHashTableIter cni;

  for (g_hash_table_iter_init (&cni, connections);
       g_hash_table_iter_next (&cni, &key, &val); )
    printf (" cn=%s %p\n", (char *) key, val);
}

static void
exec_conn_pretend (gschem_patch_line_t *patch, GList **net, int del)
{
  if (del) {
    for (GList *np = *net; np != NULL; ) {
      char *lname = np->data;
      np = np->next;
      if (strcmp (lname, patch->id) == 0) {
        *net = g_list_remove (*net, lname);
        g_free (lname);
      }
    }
  } else
    *net = g_list_prepend (*net, g_strdup (patch->id));
}

static GSList *
exec_check_conn (GSList *hits, gschem_patch_line_t *patch,
                 gschem_patch_pin_t *pin, GList **net, int del)
{
  GHashTable *connections = NULL;
  int len, offs;
  char *buff = NULL;
  int alloced = 0, connected;
  GString *msg = NULL;

  if (debug)
    printf ("exec %d:\n", del);

  if (pin->net == NULL) {
    connections = exec_list_conns (pin->obj);
    if (debug)
      exec_debug_print_conns (connections);

    /* check if we are connected to the network */
    len = strlen (patch->arg1.net_name);
    if (len + 2 > alloced) {
      alloced = len + 2 + 256;
      free (buff);
      buff = malloc (alloced);
    }
    *buff = OBJ_NET;
    memcpy (buff + 1, patch->arg1.net_name, len + 1);
    connected = g_hash_table_lookup (connections, buff) != NULL;
    offs = 1;
  } else {
    connected = strcmp (patch->arg1.net_name, pin->net) == 0;
    buff = g_strdup (pin->net);
    offs = 0;
  }

  /* Ugly hack: do not complain about (missing) connections to unnamed nets */
  if (strncmp (buff + offs, "unnamed_net", 11) != 0) {
    if (connected) {
      if (del) {
        gchar *tmp = g_strdup_printf (_("disconnect from net %s"), buff + offs);
        msg = g_string_new (tmp);
        g_free (tmp);
      }
    } else {
      if (!del) {
        gchar *tmp = g_strdup_printf (_("connect to net %s"), buff + offs);
        msg = g_string_new (tmp);
        g_free (tmp);
      }
    }
  }

  if (connections != NULL) {
    /* check if we still have a connection to any of the pins */
    GString *pin_msg = NULL;
    int pin_count = 0;
    for (GList *np = *net; np != NULL; np = np->next) {
      OBJECT *target;
      len = strlen (np->data);
      if (len + 2 > alloced) {
        alloced = len + 2 + 256;
        free (buff);
        buff = malloc (alloced);
      }
      *buff = OBJ_PIN;
      memcpy (buff + 1, np->data, len + 1);
      target = g_hash_table_lookup (connections, buff);
      if (target == pin->obj)
        continue;
      if ((target != NULL && del) || (target == NULL && !del)) {
        if (pin_msg == NULL)
          pin_msg = g_string_new (NULL);
        else
          g_string_append (pin_msg, ", ");
        g_string_append (pin_msg, buff + 1);
        pin_count++;
      }
    }
    if (pin_msg != NULL) {
      if (msg == NULL)
        msg = g_string_new (NULL);
      else
        g_string_append (msg, "; ");
      gchar *tmp = g_strdup_printf (del ? ngettext ("disconnect from pin %s",
                                                    "disconnect from pins %s",
                                                    pin_count)
                                        : ngettext ("connect to pin %s",
                                                    "connect to pins %s",
                                                    pin_count),
                                    g_string_free (pin_msg, FALSE));
      g_string_append (msg, tmp);
      g_free (tmp);
    }
    exec_free_conns (connections);
  }

  if (buff != NULL)
    free (buff);

  /* pretend that the item is resolved: update patch netlists */
  exec_conn_pretend (patch, net, del);

  if (msg != NULL) {
    return g_slist_prepend (hits, alloc_hit (pin->obj,
                                             g_strdup (patch->id),
                                             g_string_free (msg, FALSE)));
  }

  return hits;
}

static GSList *
exec_check_attrib (GSList *hits, gschem_patch_line_t *patch, OBJECT *comp)
{
  gchar *attr_val =
    o_attrib_search_object_attribs_by_name (comp, patch->arg1.attrib_name, 0);
  if (attr_val == NULL)
    return hits;
  if (strcmp (attr_val, patch->arg2.attrib_val) != 0) {
    gchar *msg = g_strdup_printf (_("change attribute \"%s\" "
                                    "from \"%s\" to \"%s\""),
                                  patch->arg1.attrib_name, attr_val,
                                  patch->arg2.attrib_val);
    hits = g_slist_prepend (hits, alloc_hit (comp, g_strdup (patch->id), msg));
  }
  g_free (attr_val);
  return hits;
}


/*! \brief Retrieve a list of hits from a fully built patch state.
 *
 * \returns a singly-linked list of hits in reverse order
 */
GSList *
gschem_patch_state_execute (gschem_patch_state_t *st)
{
  GList *onet, *net;
  GSList *pins, *comps;
  int found, del;
  GSList *hits = NULL;

  for (GList *i = st->lines; i != NULL; i = i->next) {
    gschem_patch_line_t *l = i->data;
    if (l == NULL) {
      fprintf (stderr, "NULL data on list\n");
      continue;
    }

    switch (l->op) {
      case GSCHEM_PATCH_DEL_CONN:
      case GSCHEM_PATCH_ADD_CONN:
        del = l->op == GSCHEM_PATCH_DEL_CONN;
        net = onet = g_hash_table_lookup (st->nets, l->arg1.net_name);
        pins = g_hash_table_lookup (st->pins, l->id);
        if (pins == NULL) {
          /* pin not found on open schematics */
          gchar *not_found = g_strdup_printf (_("%s (NOT FOUND)"), l->id);
          gchar *msg = g_strdup_printf (del ? _("disconnect from net %s")
                                            : _("connect to net %s"),
                                        l->arg1.net_name);
          hits = g_slist_prepend (hits, alloc_hit (NULL, not_found, msg));
          exec_conn_pretend (l, &net, del);
        } else {
          /* pin found */
          for (; pins != NULL; pins = g_slist_next (pins))
            hits = exec_check_conn (
              hits, l, (gschem_patch_pin_t *) pins->data, &net, del);
        }

        /* executing a diff may update the list */
        if (net != onet)
          g_hash_table_insert (st->nets, l->arg1.net_name, net);
        break;

      case GSCHEM_PATCH_CHANGE_ATTRIB:
        comps = g_hash_table_lookup (st->comps, l->id);
        for (found = 0; comps != NULL; comps = g_slist_next (comps)) {
          hits = exec_check_attrib (hits, l, (OBJECT *) comps->data);
          found++;
        }
        if (found == 0) {
          gchar *not_found = g_strdup_printf (_("%s (NOT FOUND)"), l->id);
          gchar *msg = g_strdup_printf (_("change attribute \"%s\" to \"%s\""),
                                        l->arg1.attrib_name,
                                        l->arg2.attrib_val);
          hits = g_slist_prepend (hits, alloc_hit (NULL, not_found, msg));
        }
        break;

      case GSCHEM_PATCH_NET_INFO:
        /* just ignore them, we've already built data structs while parsing */
        break;
    }
  }

  return g_slist_reverse (hits);
}


/******************************************************************************/
/*! \section destroy  Freeing the patch structures.                           */


static void
free_patch_line (gschem_patch_line_t *line)
{
  switch (line->op) {
    case GSCHEM_PATCH_DEL_CONN:
    case GSCHEM_PATCH_ADD_CONN:
      g_free (line->id);
      g_free (line->arg1.net_name);
      break;
    case GSCHEM_PATCH_CHANGE_ATTRIB:
      g_free (line->id);
      g_free (line->arg1.attrib_name);
      g_free (line->arg2.attrib_val);
      break;
    case GSCHEM_PATCH_NET_INFO:
      g_free (line->id);
      g_list_free_full (line->arg1.ids, g_free);
      break;
  }
  g_slice_free (gschem_patch_line_t, line);
}

static void
free_pin (gschem_patch_pin_t *pin)
{
  g_free (pin->net);
  g_slice_free (gschem_patch_pin_t, pin);
}

/*! \brief Free all memory allocated by a patch state.
 *
 * \note This *does not* free the patch state struct \a st itself.
 */
void
gschem_patch_state_destroy (gschem_patch_state_t *st)
{
  GHashTableIter iter;
  gpointer key, value;

  g_hash_table_iter_init (&iter, st->pins);
  while (g_hash_table_iter_next (&iter, &key, &value))
    g_slist_free_full ((GSList *) value, (GDestroyNotify) free_pin);

  g_hash_table_iter_init (&iter, st->comps);
  while (g_hash_table_iter_next (&iter, &key, &value))
    g_slist_free ((GSList *) value);

  g_hash_table_iter_init (&iter, st->nets);
  while (g_hash_table_iter_next (&iter, &key, &value))
    g_list_free_full ((GList *) value, g_free);

  g_hash_table_destroy (st->nets);
  g_hash_table_destroy (st->pins);
  g_hash_table_destroy (st->comps);
  g_list_free_full (st->lines, (GDestroyNotify) free_patch_line);
}


static void
free_hit (gschem_patch_hit_t *hit)
{
  g_free (hit->loc_name);
  g_free (hit->action);
  g_slice_free (gschem_patch_hit_t, hit);
}

/*! \brief Free a list of hits returned by \ref gschem_patch_state_execute.
 *
 * \note This frees all hits in the list as well as the list itself.
 */
void
gschem_patch_free_hit_list (GSList *hits)
{
  g_slist_free_full (hits, (GDestroyNotify) free_hit);
}
