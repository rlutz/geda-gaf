#include <libgeda/sab.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include <prototype_priv.h>

#define PARAM_BUFFER_LENGTH 256

static char *action_text[] = { "discard", "bypass", "exec" };

static void *
ParseBypassArgs (char *args)
{
  GSList *arg_list = NULL;
  char *seg = args;
  /* strtok is not reentrant and the reentrant version is POSIX specfic
   * which means we have to do this in two passes. The string held in
   args will be valid as long as I am in this function or above it on
   the stack. */
  while ((seg = strtok (seg, ";")) != NULL) {
    arg_list = g_slist_append (arg_list, seg);
    seg = NULL;
  }

  /* Now replace each entry in arg_list with a list of pins */
  for (GSList * cur = arg_list; cur != NULL; cur = cur->next) {
    char *pin_list = (char *) cur->data;
    pin_set *set;
    char *name = NULL;
    char *as;

    if ((as = strstr (pin_list, "as")) != NULL) {
      name = as + 2;
      while (*name != '\0' && isspace (*name))
        name++;
      while (as != pin_list && isspace (*(as - 1)))
        as--;
      if (as == pin_list || *name == '\0') {  /* arg is malformed so discard and move on */
        cur->data = NULL;
        continue;
      }
      *as = '\0';
    }
    set = g_new (pin_set, 1);
    set->label = g_strdup (name);

    as = pin_list;
    set->pins = NULL;
    while ((as = strtok (as, ",")) != NULL) {
      set->pins = g_slist_append (set->pins, GUINT_TO_POINTER (atoi (as)));
      as = NULL;
    }

    cur->data = (void *) set;

  }
  arg_list = g_slist_remove_all (arg_list, NULL);
  return (void *) arg_list;
}

static gchar *
ConstructBypassArg (GSList *arg_list)
{
  gchar *buf1;
  gchar *buf2;
  gchar *buf3;
  GSList *cur_seg = arg_list;

  buf1 = g_strdup ("");
  while (cur_seg != NULL) {
    pin_set *pset = (pin_set *) (cur_seg->data);
    GSList *cur_pin = pset->pins;

    buf2 = g_strdup ("");
    while (cur_pin != NULL) {
      buf3 =
        g_strdup_printf ("%s%s%d", buf2, (strlen (buf2) > 0 ? "," : ""),
                         GPOINTER_TO_UINT (cur_pin->data));
      g_free (buf2);
      buf2 = buf3;
      cur_pin->data = NULL;
      cur_pin = cur_pin->next;
    }
    g_slist_free_full (g_steal_pointer (&(pset->pins)), g_free);

    if (pset->label != NULL) {
      buf3 = g_strconcat (buf2, " as ", pset->label, NULL);
      g_free (buf2);
      buf2 = buf3;
      g_free (g_steal_pointer (&(pset->label)));
    }

    buf3 = g_strconcat (buf1, (strlen (buf1) > 0 ? ";" : ""), buf2, NULL);
    g_free (buf1);
    g_free (buf2);
    buf1 = buf3;

    g_free (g_steal_pointer (&(cur_seg->data)));
    cur_seg = cur_seg->next;
  }
/* The source list will be freed by the caller */

  return buf1;
}

/* Retrieves all the sab-param attributes of the provided OBJECT and parses them,
 * ignoring any which are malformed. */
sab_action_set *
SabGetSet (OBJECT *object)
{
  sab_action_set *new_set = NULL;
  OBJECT *attr;
  int attr_index = 0;
  char attr_buf[PARAM_BUFFER_LENGTH];


  if (object == NULL)
    return NULL;

  new_set = g_new (sab_action_set, 1);
  new_set->base = object;
  new_set->actions = NULL;

  while ((attr =
          o_attrib_find_attrib_by_name (object->attribs, "sab-param",
                                        attr_index++)) != NULL) {
    sab_action *new_action;
    char *attr_val;

    new_action = g_new (sab_action, 1);
    new_action->src = attr;
    if (!o_attrib_get_name_value (attr, NULL, &attr_val)) {
      printf ("ERROR: Unable to retrieve value of sab-param in SAB.\n");
      exit (-1);
    }
    if (strlen (attr_val) > PARAM_BUFFER_LENGTH) {
      printf ("ERROR: Length of sab-param exceeds buffer length.\n");
      exit (-1);
    }
    strcpy (attr_buf, attr_val);
    g_free (attr_val);

    if ((attr_val = strtok (attr_buf, ":")) != NULL) {
      new_action->context = g_strdup (attr_val);

      if ((attr_val = strtok (NULL, ":")) != NULL) {
        int i;

        if (*attr_val == '#') {
          new_action->order = atoi (attr_val + 1);
          if ((attr_val = strtok (NULL, ":")) == NULL) {  /* malformed: missing action */
            g_free (new_action->context);
            g_free (new_action);
            continue;
          }

        } else {
          new_action->order = -1;
        }

        for (i = 0; i < strlen (attr_val); i++)
          attr_val[i] = tolower (attr_val[i]);

        /* The discard action ignores any parameters so we can safely drop
         * anything remaining if the action is 'discard'. Otherwise we need
         * tp keep going. */
        if (strcmp (attr_val, action_text[0]) == 0) {
          new_action->action = discard;
          new_action->action_param = NULL;
        } else if (strcmp (attr_val, action_text[1]) == 0) {
          new_action->action = bypass;
          new_action->action_param = ParseBypassArgs (strtok (NULL, ""));
        } else if (strcmp (attr_val, action_text[2]) == 0) {
          new_action->action = exec;
          new_action->action_param = g_strdup (strtok (NULL, ""));
        } else {
          free (new_action);
          continue;
        }

        new_set->actions = g_slist_append (new_set->actions, new_action);
      } else {
        free (new_action);
      }
    } else {
      free (new_action);
    }
  }
  return new_set;
}

void
SabUpdateBypassPin (sab_action_set *set, char *cur_pin, char *new_pin)
{
  gpointer cpin = GUINT_TO_POINTER (atoi (cur_pin));
  gpointer npin = GUINT_TO_POINTER (atoi (new_pin));

  if (set != NULL) {
    GSList *action_elem = set->actions;
    while (action_elem != NULL) {
      sab_action *action = (sab_action *) (action_elem->data);
      if (action != NULL && action->action == bypass) {
        GSList *group = (GSList *) (action->action_param);
        while (group != NULL) {
          pin_set *pins = (pin_set *) (group->data);
          if (pins != NULL) {
            GSList *pin = pins->pins;
            while (pin != NULL) {
              if (pin->data == cpin)
                pin->data = npin;

              pin = pin->next;
            }
          }
          group = group->next;
        }
      }
      action_elem = action_elem->next;
    }
  }
}

void
SabReleaseSet (sab_action_set *set, TOPLEVEL *topLevel)
{

  if (set != NULL) {
    GSList *action = set->actions;
    while (action != NULL) {
      sab_action *act = (sab_action *) action->data;
      if (act != NULL) {
        gchar *label = NULL;
        gchar *order = NULL;
        gchar *param = NULL;
        gchar *text = NULL;

        label = g_strdup_printf ("sab-param=%s", act->context);
        g_free (act->context);

        if (act->order > -1) {
          order =
            g_strdup_printf (":#%d:%s", act->order, action_text[act->action]);
        } else {
          order = g_strdup_printf (":%s", action_text[act->action]);
        }

        switch (act->action) {
        case discard:
          break;

        case bypass:
          param = ConstructBypassArg ((GSList *) (act->action_param));
          g_slist_free_full (g_steal_pointer (&(act->action_param)), g_free);
          break;

        case exec:
          param = strdup (act->action_param);
          g_free (act->action_param);
          break;

        default:
          printf ("ERROR: In SabReleaseAction unknown action found,\n");
          exit (-1);
        }

        text =
          g_strconcat (label, order,
                       ((param != NULL
                         && strlen (param) > 0) ? ":" : ""), param, NULL);
        g_free (label);
        g_free (order);
        g_free (param);

        o_text_set_string (topLevel, act->src, text);
        g_free (text);

        g_free (g_steal_pointer (&(action->data)));
      }
      action = action->next;
    }
    g_slist_free_full (g_steal_pointer (&(set->actions)), g_free);
  }
  g_free (set);
}
