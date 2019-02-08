/* gEDA - GPL Electronic Design Automation
 * gschem - gEDA Schematic Capture
 * Copyright (C) 1998-2010 Ales Hvezda
 * Copyright (C) 1998-2019 gEDA Contributors (see ChangeLog for details)
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
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

#define DEFINE_ACTION(c_id, id, icon, name, label, menu_label, tooltip) \
  static void action_callback_ ## c_id (GschemAction *action, \
                                        GschemToplevel *w_current)
#include "actions.c"
#undef DEFINE_ACTION

#include "actions.decl.x"


static scm_t_bits action_tag;


GschemAction *
gschem_action_register (gchar *id,
                        gchar *icon_name,
                        gchar *name,
                        gchar *label,
                        gchar *menu_label,
                        gchar *tooltip,
                        void (*activate) (GschemAction *, GschemToplevel *))
{
  GschemAction *action = g_new0 (GschemAction, 1);

  action->id = id;
  action->icon_name = icon_name;
  action->name = name;
  action->label = label;
  action->menu_label = menu_label;
  action->tooltip = tooltip;
  action->activate = activate;

  action->thunk = SCM_UNDEFINED;

  /* create public binding */
  scm_dynwind_begin (0);
  {
    gchar *name = g_strdup_printf ("&%s", action->id);
    scm_dynwind_free (name);

    scm_c_define (name, scm_new_smob (action_tag, (scm_t_bits) action));
    scm_c_export (name, NULL);
  }
  scm_dynwind_end ();

  return action;
}


void
gschem_action_activate (GschemAction *action,
                        GschemToplevel *w_current)
{
  g_return_if_fail (action->activate != NULL);
  g_return_if_fail (w_current != NULL);

  scm_dynwind_begin (0);
  g_dynwind_window (w_current);
  action->activate (action, w_current);
  scm_dynwind_end ();
}


/******************************************************************************/


int
scm_is_action (SCM x)
{
  return SCM_SMOB_PREDICATE (action_tag, x);
}

GschemAction *
scm_to_action (SCM smob)
{
  scm_assert_smob_type (action_tag, smob);
  return GSCHEM_ACTION (SCM_SMOB_DATA (smob));
}


static void
activate_scheme_thunk (GschemAction *action, GschemToplevel *w_current)
{
  g_return_if_fail (!SCM_UNBNDP (action->thunk));
  g_scm_eval_protected (scm_list_1 (action->thunk), SCM_UNDEFINED);
}

static SCM
make_action (SCM s_icon_name, SCM s_name, SCM s_label, SCM s_menu_label,
             SCM s_tooltip, SCM s_thunk)
{
  GschemAction *action;
  SCM smob;

  SCM_ASSERT (scm_is_false (s_icon_name) ||
              scm_is_string (s_icon_name), s_icon_name,
              SCM_ARG1, "%make-action!");
  SCM_ASSERT (scm_is_false (s_name) ||
              scm_is_string (s_name), s_name,
              SCM_ARG2, "%make-action!");
  SCM_ASSERT (scm_is_false (s_label) ||
              scm_is_string (s_label), s_label,
              SCM_ARG3, "%make-action!");
  SCM_ASSERT (scm_is_false (s_menu_label) ||
              scm_is_string (s_menu_label), s_menu_label,
              SCM_ARG4, "%make-action!");
  SCM_ASSERT (scm_is_false (s_tooltip) ||
              scm_is_string (s_tooltip), s_tooltip,
              SCM_ARG5, "%make-action!");
  SCM_ASSERT (scm_is_true (scm_thunk_p (s_thunk)), s_thunk,
              SCM_ARG6, "%make-action!");

  scm_dynwind_begin (0);
  {
    char *icon_name, *name, *label, *menu_label, *tooltip;

    icon_name = scm_is_false (s_icon_name) ? NULL :
                scm_to_utf8_string (s_icon_name);
    scm_dynwind_free (icon_name);

    name = scm_is_false (s_name) ? NULL :
           scm_to_utf8_string (s_name);
    scm_dynwind_free (name);

    label = scm_is_false (s_label) ? NULL :
            scm_to_utf8_string (s_label);
    scm_dynwind_free (label);

    menu_label = scm_is_false (s_menu_label) ? NULL :
                 scm_to_utf8_string (s_menu_label);
    scm_dynwind_free (menu_label);

    tooltip = scm_is_false (s_tooltip) ? NULL :
              scm_to_utf8_string (s_tooltip);
    scm_dynwind_free (tooltip);

    action = g_new0 (GschemAction, 1);

    action->id = NULL;
    action->icon_name = g_strdup (icon_name);
    action->name = g_strdup (name);
    action->label = g_strdup (label);
    action->menu_label = g_strdup (menu_label);
    action->tooltip = g_strdup (tooltip);
    action->activate = activate_scheme_thunk;

    action->thunk = SCM_UNDEFINED;

    /* immediately create the smob so the garbage collector knows
       about the allocated data */
    smob = scm_new_smob (action_tag, (scm_t_bits) action);
  }
  scm_dynwind_end ();

  /* only then, fill in the thunk field */
  action->thunk = s_thunk;

  return smob;
}

static SCM
action_p (SCM smob)
{
  return scm_from_bool (SCM_SMOB_PREDICATE (action_tag, smob));
}

static void
init_module_gschem_core_action (void *data)
{
  scm_c_define_gsubr ("%make-action", 6, 0, 0, make_action);
  scm_c_define_gsubr ("%action?", 1, 0, 0, action_p);

  scm_c_export ("%make-action", "%action?", NULL);
}

static void
init_module_gschem_core_builtins (void *data)
{
#include "actions.init.x"
}


static SCM
mark_action (SCM smob)
{
  GschemAction *action = GSCHEM_ACTION (SCM_SMOB_DATA (smob));

  return action->thunk;
}

static size_t
free_action (SCM smob)
{
  GschemAction *action = GSCHEM_ACTION (SCM_SMOB_DATA (smob));

  g_free (action->id);
  g_free (action->icon_name);
  g_free (action->name);
  g_free (action->label);
  g_free (action->menu_label);
  g_free (action->tooltip);

  return 0;
}

static int
print_action (SCM smob, SCM port, scm_print_state *pstate)
{
  GschemAction *action = GSCHEM_ACTION (SCM_SMOB_DATA (smob));

  if (action->id != NULL) {
    scm_puts ("#<action ", port);
    scm_puts (action->id, port);
    scm_puts (">", port);
  } else if (action->name != NULL) {
    scm_puts ("#<action \"", port);
    scm_puts (action->name, port);
    scm_puts ("\">", port);
  } else
    scm_puts ("#<action>", port);

  scm_remember_upto_here_1 (smob);
  return 1;  /* non-zero means success */
}

static SCM
equalp_action (SCM smob0, SCM smob1)
{
  GschemAction *action0 = GSCHEM_ACTION (SCM_SMOB_DATA (smob0));
  GschemAction *action1 = GSCHEM_ACTION (SCM_SMOB_DATA (smob1));

  return scm_from_bool (action0 == action1);
}

static SCM
apply_action (SCM smob)
{
  GschemAction *action = GSCHEM_ACTION (SCM_SMOB_DATA (smob));
  gschem_action_activate (action, g_current_window ());

  scm_remember_upto_here_1 (smob);
  return SCM_UNDEFINED;
}

void
gschem_action_init (void)
{
  action_tag = scm_make_smob_type ("action", 0);
  scm_set_smob_mark (action_tag, mark_action);
  scm_set_smob_free (action_tag, free_action);
  scm_set_smob_print (action_tag, print_action);
  scm_set_smob_equalp (action_tag, equalp_action);
  scm_set_smob_apply (action_tag, apply_action, 0, 0, 0);

  scm_c_define_module ("gschem core action",
                       init_module_gschem_core_action, NULL);

  scm_c_define_module ("gschem core builtins",
                       init_module_gschem_core_builtins, NULL);
}


/******************************************************************************/


#define TYPE_DISPATCHER \
  (gschem_action_state_dispatcher_get_type ())
#define DISPATCHER(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), TYPE_DISPATCHER, Dispatcher))
#define DISPATCHER_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), TYPE_DISPATCHER, DispatcherClass))
#define IS_DISPATCHER(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), TYPE_DISPATCHER))
#define DISPATCHER_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), TYPE_DISPATCHER, DispatcherClass))

typedef struct _DispatcherClass DispatcherClass;
typedef struct _Dispatcher      Dispatcher;

struct _DispatcherClass {
  GObjectClass parent_class;
};

struct _Dispatcher {
  GObject parent_instance;
};

enum {
  SET_SENSITIVE,
  LAST_SIGNAL
};

static void dispatcher_class_init (DispatcherClass *class);

static guint dispatcher_signals[LAST_SIGNAL] = { 0 };


static GType
gschem_action_state_dispatcher_get_type ()
{
  static GType type = 0;

  if (type == 0) {
    static const GTypeInfo info = {
      sizeof (DispatcherClass),
      NULL,                                     /* base_init */
      NULL,                                     /* base_finalize */
      (GClassInitFunc) dispatcher_class_init,
      NULL,                                     /* class_finalize */
      NULL,                                     /* class_data */
      sizeof (Dispatcher),
      0,                                        /* n_preallocs */
      NULL,                                     /* instance_init */
      NULL                                      /* value_table */
    };

    type = g_type_register_static (G_TYPE_OBJECT,
                                   "GschemActionStateDispatcher",
                                   &info, 0);
  }

  return type;
}


static void
dispatcher_class_init (DispatcherClass *class)
{
  dispatcher_signals[SET_SENSITIVE] =
    g_signal_new ("set-sensitive",
                  G_OBJECT_CLASS_TYPE (class),
                  G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
                  0, NULL, NULL,
                  g_cclosure_marshal_VOID__BOOLEAN,
                  G_TYPE_NONE, 1,
                  G_TYPE_BOOLEAN);
}


void
gschem_action_set_sensitive (GschemAction *action, gboolean sensitive,
                             GschemToplevel *w_current)
{
  Dispatcher *dispatcher = g_hash_table_lookup (
    w_current->action_state_dispatchers, action);
  if (dispatcher == NULL)
    /* no widget for this action */
    return;

  g_return_if_fail (IS_DISPATCHER (dispatcher));
  g_signal_emit (dispatcher, dispatcher_signals[SET_SENSITIVE], 0, sensitive);
}


GObject *
gschem_action_get_dispatcher (GschemAction *action,
                              GschemToplevel *w_current)
{
  Dispatcher *dispatcher = g_hash_table_lookup (
    w_current->action_state_dispatchers, action);

  if (dispatcher == NULL) {
    dispatcher = g_object_new (TYPE_DISPATCHER, NULL);
    g_hash_table_insert (w_current->action_state_dispatchers,
                         action, dispatcher);
  }

  return G_OBJECT (dispatcher);
}
