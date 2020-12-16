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

/*! \file gschem_action.c
 * \brief Action mechanism.
 *
 * Actions are represented by a GschemAction struct which contains a
 * pointer to the callback function as well as some metadata (action
 * name, icon, etc.).  To Scheme code, actions are visible as SMOBs,
 * applicable foreign object wrappers around the action structs which
 * behave like procedures but make it trivial to retrieve the
 * GschemAction struct for a given Scheme action object.
 *
 * This design made several improvements possible:
 *
 * - Actions are defined in a single place: the DEFINE_ACTION macro in
 *   actions.c declares a global symbol referring to the action, sets
 *   the action metadata, and defines the action callback function.
 *   Menu items, toolbar buttons, and the context menu all share the
 *   same set of actions.
 *
 * - The state of actions representing an option (e.g., "Show/Hide
 *   Invisible Text") is indicated by a checkbox in the menu item and
 *   by the toolbar button being depressed.  The state of mode actions
 *   (e.g., "Add Rectangle") is indicated by the toolbar button being
 *   depressed.  When an action can't be executed (e.g., "Redo"), the
 *   menu item and toolbar button are insensitive.
 *
 * - Toolbar and context menu are configured the same way menus are.
 *
 * - "Repeat Last Action" (usually bound to ".") uses the same logic
 *   as the middle mouse button repeat action does, i.e., only actions
 *   that "make sense" qualify for repeating.
 *
 * Written by Roland Lutz 2019.
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

#define DEFINE_ACTION(c_id, id, icon, name, label, menu_label, tooltip, type) \
  GschemAction *action_ ## c_id; \
  static void action_callback_ ## c_id (GschemAction *action, \
                                        GschemToplevel *w_current)
#include "actions.c"
#undef DEFINE_ACTION

#include "actions.decl.x"


static scm_t_bits action_tag;


/*! \brief Register statically-defined gschem action.
 *
 * Allocates a new GschemAction struct, sets the meta-information
 * passed as arguments, and creates a public top-level Scheme binding
 * for the action in the current environment.
 *
 * \note This function should only be used indirectly by defining an
 *       action in actions.c.
 */
GschemAction *
gschem_action_register (gchar *id,
                        gchar *icon_name,
                        gchar *name,
                        gchar *label,
                        gchar *menu_label,
                        gchar *tooltip,
                        GschemActionType type,
                        void (*activate) (GschemAction *, GschemToplevel *))
{
  GschemAction *action = g_new0 (GschemAction, 1);

  action->id = id;
  action->icon_name = icon_name;
  action->name = name;
  action->label = label;
  action->menu_label = menu_label;
  action->tooltip = tooltip;
  action->type = type;
  action->activate = activate;

  action->thunk = SCM_UNDEFINED;

  /* actions are global objects, so their smob should be permanent */
  action->smob = scm_permanent_object (
    scm_new_smob (action_tag, (scm_t_bits) action));

  /* create public binding */
  scm_dynwind_begin (0);
  {
    gchar *name = g_strdup_printf ("&%s", action->id);
    scm_dynwind_free (name);

    scm_c_define (name, action->smob);
    scm_c_export (name, NULL);
  }
  scm_dynwind_end ();

  return action;
}


/*! \brief Run an action.
 *
 * If you want to invoke an action explicitly, use this function.
 *
 * Runs the activation function associated with \a action in the
 * Scheme toplevel context of \a w_current.
 */
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


/* The following functions define the Guile SMOB type which represents
 * GschemAction structures in Scheme.
 *
 * Pointer identity is preserved by storing the SMOB representation of
 * a GschemAction structure in its "smob" member.  This means you can
 * safely compare #<gschem-action ...> objects using "eq?".
 *
 * A GschemAction structure does not own its "smob" member; on the
 * contrary, a user-defined action may be freed if its SMOB is
 * garbage collected before an associated widget has been created.
 */

/*! \brief Return whether a given Guile expression represents an action.
 */
int
scm_is_action (SCM x)
{
  return SCM_SMOB_PREDICATE (action_tag, x);
}

/*! \brief Return the action represented by a Guile expression.
 */
GschemAction *
scm_to_action (SCM smob)
{
  scm_assert_smob_type (action_tag, smob);
  return GSCHEM_ACTION (SCM_SMOB_DATA (smob));
}


/*! \brief Callback function for activating user-defined actions.
 *
 * Actions defined from Scheme code don't have a hard-coded C callback
 * function.  Instead, this function is used, which calls the Scheme
 * thunk stored in the action structure.
 */
static void
activate_scheme_thunk (GschemAction *action, GschemToplevel *w_current)
{
  g_return_if_fail (!SCM_UNBNDP (action->thunk));
  g_scm_eval_protected (scm_list_1 (action->thunk), SCM_UNDEFINED);
}

/*! \brief Guile interface for creating user-defined actions.
 *
 * Creates a new action wrapping the given Guile "thunk" (that is,
 * parameter-less function) and meta-information and returns a SMOB
 * representing it.
 */
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
    action->type = GSCHEM_ACTION_TYPE_ACTUATE;
    action->activate = activate_scheme_thunk;

    action->thunk = SCM_UNDEFINED;

    /* immediately create the smob and keep a reference on the stack
       so the garbage collector knows about the allocated data */
    action->smob = smob = scm_new_smob (action_tag, (scm_t_bits) action);
  }
  scm_dynwind_end ();

  /* only then, fill in the thunk field */
  action->thunk = s_thunk;

  return smob;
}

/*! \brief Guile subr for testing whether a given Scheme expression
 *         represents an action.
 *
 * This is like scm_is_action but returns #t or #f instead of 1 or 0.
 */
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
    scm_puts ("#<gschem-action ", port);
    scm_puts (action->id, port);
    scm_puts (">", port);
  } else if (action->name != NULL) {
    scm_puts ("#<gschem-action \"", port);
    scm_puts (action->name, port);
    scm_puts ("\">", port);
  } else
    scm_puts ("#<gschem-action>", port);

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

/*! \brief Initialize Scheme action interface.
 *
 * Sets up the action SMOB type and defines the modules <tt>(gschem
 * core action)</tt> and <tt>(gschem core builtins)</tt>.  Must be
 * called before calling any other action-related functions.
 */
void
gschem_action_init (void)
{
  action_tag = scm_make_smob_type ("gschem-action", 0);
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


/*! \class _Dispatcher
 *  \brief Action state dispatcher.
 *
 * Actions are global objects which aren't associated with a specific
 * GschemToplevel.  This means that there is no global "state" of an
 * action--a given action may be sensitive in one toplevel and not in
 * another with, say, a different selection.
 *
 * In order to resolve this, a GschemToplevel has one action state
 * dispatcher object for each action that has widgets associated with
 * it.  The dispatcher object keeps track of the state of the action
 * (whether it is currently sensitive and/or active) and provides a
 * point for widgets to connect to in order to react to state changes.
 *
 * The dispatchers are stored in the \ref action_state_dispatchers
 * field of the toplevel and created on-demand by \ref get_dispatcher.
 */

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

  void (*set_sensitive) (Dispatcher *dispatcher, gboolean sensitive);
  void (*set_active) (Dispatcher *dispatcher, gboolean is_active);

  void (*set_name) (Dispatcher *dispatcher, gchar *name);
  void (*set_label) (Dispatcher *dispatcher, gchar *label);
  void (*set_menu_label) (Dispatcher *dispatcher, gchar *menu_label);
};

struct _Dispatcher {
  GObject parent_instance;

  guint sensitive : 1;
  guint active : 1;

  gchar *name;
  gchar *label;
  gchar *menu_label;
};

enum {
  SET_SENSITIVE,
  SET_ACTIVE,
  SET_NAME,
  SET_LABEL,
  SET_MENU_LABEL,
  LAST_SIGNAL
};

static void dispatcher_class_init (DispatcherClass *class);
static void dispatcher_instance_init (Dispatcher *dispatcher);
static void dispatcher_finalize (GObject *object);

static void dispatcher_set_sensitive (Dispatcher *dispatcher,
                                      gboolean sensitive);
static void dispatcher_set_active (Dispatcher *dispatcher,
                                   gboolean is_active);

static void dispatcher_set_name (Dispatcher *dispatcher, gchar *name);
static void dispatcher_set_label (Dispatcher *dispatcher, gchar *label);
static void dispatcher_set_menu_label (Dispatcher *dispatcher,
                                       gchar *menu_label);

static gpointer dispatcher_parent_class = NULL;
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
      (GInstanceInitFunc) dispatcher_instance_init,
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
  dispatcher_parent_class = g_type_class_peek_parent (class);

  G_OBJECT_CLASS (class)->finalize = dispatcher_finalize;

  class->set_sensitive = dispatcher_set_sensitive;
  class->set_active = dispatcher_set_active;

  class->set_name = dispatcher_set_name;
  class->set_label = dispatcher_set_label;
  class->set_menu_label = dispatcher_set_menu_label;

  dispatcher_signals[SET_SENSITIVE] =
    g_signal_new ("set-sensitive",
                  G_OBJECT_CLASS_TYPE (class),
                  G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
                  G_STRUCT_OFFSET (DispatcherClass, set_sensitive),
                  NULL, NULL,
                  g_cclosure_marshal_VOID__BOOLEAN,
                  G_TYPE_NONE, 1,
                  G_TYPE_BOOLEAN);

  dispatcher_signals[SET_ACTIVE] =
    g_signal_new ("set-active",
                  G_OBJECT_CLASS_TYPE (class),
                  G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
                  G_STRUCT_OFFSET (DispatcherClass, set_active),
                  NULL, NULL,
                  g_cclosure_marshal_VOID__BOOLEAN,
                  G_TYPE_NONE, 1,
                  G_TYPE_BOOLEAN);

  dispatcher_signals[SET_NAME] =
    g_signal_new ("set-name",
                  G_OBJECT_CLASS_TYPE (class),
                  G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
                  G_STRUCT_OFFSET (DispatcherClass, set_name),
                  NULL, NULL,
                  g_cclosure_marshal_VOID__STRING,
                  G_TYPE_NONE, 1,
                  G_TYPE_STRING);

  dispatcher_signals[SET_LABEL] =
    g_signal_new ("set-label",
                  G_OBJECT_CLASS_TYPE (class),
                  G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
                  G_STRUCT_OFFSET (DispatcherClass, set_label),
                  NULL, NULL,
                  g_cclosure_marshal_VOID__STRING,
                  G_TYPE_NONE, 1,
                  G_TYPE_STRING);

  dispatcher_signals[SET_MENU_LABEL] =
    g_signal_new ("set-menu-label",
                  G_OBJECT_CLASS_TYPE (class),
                  G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
                  G_STRUCT_OFFSET (DispatcherClass, set_menu_label),
                  NULL, NULL,
                  g_cclosure_marshal_VOID__STRING,
                  G_TYPE_NONE, 1,
                  G_TYPE_STRING);
}


static void
dispatcher_instance_init (Dispatcher *dispatcher)
{
  dispatcher->sensitive = TRUE;
}


static void
dispatcher_finalize (GObject *object)
{
  Dispatcher *dispatcher = DISPATCHER (object);

  g_free (dispatcher->name);
  g_free (dispatcher->label);
  g_free (dispatcher->menu_label);

  G_OBJECT_CLASS (dispatcher_parent_class)->finalize (object);
}


static Dispatcher *
get_dispatcher (GschemAction *action,
                GschemToplevel *w_current)
{
  Dispatcher *dispatcher = g_hash_table_lookup (
    w_current->action_state_dispatchers, action);

  if (dispatcher == NULL) {
    dispatcher = g_object_new (TYPE_DISPATCHER, NULL);
    g_hash_table_insert (w_current->action_state_dispatchers,
                         action, dispatcher);
  }

  return dispatcher;
}


/*! \brief Set whether an action is "sensitive", i.e., clickable.
 *
 * Updates all widgets associated with the action to indicate the new
 * status.  If \a sensitive is \c FALSE, menu items and tool buttons
 * are displayed greyed-out and can't be selected by the user; if \a
 * sensitive is \c TRUE, they can be selected normally.
 *
 * Even if an action has been marked as insensitive, it can still be
 * activated normally via a hotkey.
 */
void
gschem_action_set_sensitive (GschemAction *action, gboolean sensitive,
                             GschemToplevel *w_current)
{
  Dispatcher *dispatcher = get_dispatcher (action, w_current);
  g_return_if_fail (IS_DISPATCHER (dispatcher));
  g_signal_emit (dispatcher, dispatcher_signals[SET_SENSITIVE], 0, sensitive);
}


/*! \brief Set whether an action is displayed as "active".
 *
 * Updates all widgets associated with the action to indicate the new
 * status.  Menu items are rendered depending on the action type.
 * Toolbar buttons are rendered in "depressed" state if \a is_active
 * is \c TRUE and in normal state if \a is_active is \c FALSE.
 */
void
gschem_action_set_active (GschemAction *action, gboolean is_active,
                          GschemToplevel *w_current)
{
  Dispatcher *dispatcher = get_dispatcher (action, w_current);
  g_return_if_fail (IS_DISPATCHER (dispatcher));
  g_signal_emit (dispatcher, dispatcher_signals[SET_ACTIVE], 0, is_active);
}


/*! \brief Set the displayed strings of an action.
 *
 * Updates all widgets associated with the action to show the new
 * strings.  Menu items show \a label or \a menu_label, depending on
 * whether they are part of the main menu.  Toolbar buttons show \a
 * name as tooltip.  All strings can be \c NULL, in which case the
 * default string for the action is used.
 *
 * \note Setting a string to its default value is not the same thing
 *       as not setting the string at all.
 */
void
gschem_action_set_strings (GschemAction *action,
                           gchar *name, gchar *label, gchar *menu_label,
                           GschemToplevel *w_current)
{
  Dispatcher *dispatcher = get_dispatcher (action, w_current);
  g_return_if_fail (IS_DISPATCHER (dispatcher));

  g_signal_emit (dispatcher, dispatcher_signals[SET_NAME], 0,
                 name != NULL ? name : action->name);
  g_signal_emit (dispatcher, dispatcher_signals[SET_LABEL], 0,
                 label != NULL ? label : action->label);
  g_signal_emit (dispatcher, dispatcher_signals[SET_MENU_LABEL], 0,
                 menu_label != NULL ? menu_label : action->menu_label);
}


/*! \brief Dispatcher class closure for "set-sensitive" signal.
 *
 * Invoked whenever a "set-sensitive" signal is emitted for a
 * dispatcher.  Updates the dispatcher's \a sensitive flag so new
 * widgets can be constructed in the correct state.
 */
static void
dispatcher_set_sensitive (Dispatcher *dispatcher, gboolean sensitive)
{
  dispatcher->sensitive = sensitive;
}


/*! \brief Dispatcher class closure for "set-active" signal.
 *
 * Invoked whenever a "set-active" signal is emitted for a dispatcher.
 * Updates the dispatcher's \a active flag so new widgets can be
 * constructed in the correct state.
 */
static void
dispatcher_set_active (Dispatcher *dispatcher, gboolean is_active)
{
  dispatcher->active = is_active;
}


/*! \brief Dispatcher class closures for "set-name", "set-label", and
 *         "set-menu-label" signals.
 *
 * Invoked whenever the corresponding signal is emitted for a
 * dispatcher.  Updates the dispatcher's stored string so new widgets
 * can be constructed with the correct label.
 */
static void
dispatcher_set_name (Dispatcher *dispatcher, gchar *name)
{
  g_free (dispatcher->name);
  dispatcher->name = g_strdup (name);
}

static void
dispatcher_set_label (Dispatcher *dispatcher, gchar *label)
{
  g_free (dispatcher->label);
  dispatcher->label = g_strdup (label);
}

static void
dispatcher_set_menu_label (Dispatcher *dispatcher, gchar *menu_label)
{
  g_free (dispatcher->menu_label);
  dispatcher->menu_label = g_strdup (menu_label);
}


/******************************************************************************/


static void
menu_item_toggled (GtkCheckMenuItem *menu_item, gpointer user_data);

static void
menu_item_set_active (GtkCheckMenuItem *menu_item, gboolean is_active)
{
  /* make sure the toggle handler isn't called recursively */
  g_signal_handlers_block_matched (
    menu_item, G_SIGNAL_MATCH_FUNC, 0, 0, NULL,
    G_CALLBACK (menu_item_toggled), NULL);

  gtk_check_menu_item_set_active (menu_item, is_active);

  g_signal_handlers_unblock_matched (
    menu_item, G_SIGNAL_MATCH_FUNC, 0, 0, NULL,
    G_CALLBACK (menu_item_toggled), NULL);
}

static void
menu_item_activate (GtkMenuItem *menu_item, gpointer user_data)
{
  GschemAction *action = g_object_get_data (G_OBJECT (menu_item), "action");
  GschemToplevel *w_current = GSCHEM_TOPLEVEL (user_data);
  gschem_action_activate (action, w_current);
}

static void
menu_item_toggled (GtkCheckMenuItem *menu_item, gpointer user_data)
{
  /* menu item self-toggles when clicked--undo this first */
  menu_item_set_active (menu_item,
                        !gtk_check_menu_item_get_active (menu_item));

  GschemAction *action = g_object_get_data (G_OBJECT (menu_item), "action");
  GschemToplevel *w_current = GSCHEM_TOPLEVEL (user_data);
  gschem_action_activate (action, w_current);
}

static char *
get_accel_string (GschemAction *action)
{
  /* look up key binding in global keymap */
  SCM s_expr = scm_list_2 (scm_from_utf8_symbol ("find-key"), action->smob);
  SCM s_keys = g_scm_eval_protected (s_expr, scm_interaction_environment ());
  return scm_is_true (s_keys) ? scm_to_utf8_string (s_keys) : NULL;
}

/*! \brief Create action menu item.
 *
 * Creates a GtkMenuItem, GtkImageMenuItem, or GtkCheckMenuItem widget
 * (depending on the action type), configures it according to the
 * action metadata, attaches the appropriate submenu (if applicable),
 * and hooks the widget up with the corresponding dispatcher object so
 * it will be updated when the action status changes.
 */
GtkWidget *
gschem_action_create_menu_item (GschemAction *action,
                                gboolean use_menu_label,
                                GschemToplevel *w_current)
{
  GtkWidget *menu_item;

  if (action->type == GSCHEM_ACTION_TYPE_TOGGLE_CHECK)
    menu_item = g_object_new (GTK_TYPE_CHECK_MENU_ITEM, NULL);
  else if (action->type == GSCHEM_ACTION_TYPE_TOGGLE_RADIO)
    menu_item = g_object_new (GTK_TYPE_CHECK_MENU_ITEM,
                              "draw-as-radio", TRUE, NULL);
  else if (action->icon_name == NULL)
    menu_item = g_object_new (GTK_TYPE_MENU_ITEM, NULL);
  else {
    menu_item = g_object_new (GTK_TYPE_IMAGE_MENU_ITEM, NULL);

    GtkWidget *image = gtk_image_new ();
    gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM (menu_item), image);

    /* If there's a matching stock item, use it.
       Otherwise lookup the name in the icon theme. */
    GtkStockItem stock_item;
    if (gtk_stock_lookup (action->icon_name, &stock_item))
      gtk_image_set_from_stock (GTK_IMAGE (image), action->icon_name,
                                GTK_ICON_SIZE_MENU);
    else
      gtk_image_set_from_icon_name (GTK_IMAGE (image), action->icon_name,
                                    GTK_ICON_SIZE_MENU);
  }

  /* use custom label widget */
  char *accel_string = get_accel_string (action);
  GtkWidget *label = g_object_new (GSCHEM_TYPE_ACCEL_LABEL, NULL);
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_label_set_use_underline (GTK_LABEL (label), TRUE);
  gtk_label_set_label (GTK_LABEL (label), use_menu_label ? action->menu_label
                                                         : action->label);
  gschem_accel_label_set_accel_string (GSCHEM_ACCEL_LABEL (label),
                                       accel_string);
  gtk_container_add (GTK_CONTAINER (menu_item), label);
  gtk_widget_show (label);
  free (accel_string);

  gtk_widget_set_tooltip_text (menu_item, action->tooltip);

  /* attach submenus */
  if (action == action_file_open_recent)
    gtk_menu_item_set_submenu (GTK_MENU_ITEM (menu_item),
                               w_current->recent_chooser_menu);
  else if (action == action_docking_area_left)
    gtk_menu_item_set_submenu (GTK_MENU_ITEM (menu_item),
                               w_current->left_docking_area_menu);
  else if (action == action_docking_area_bottom)
    gtk_menu_item_set_submenu (GTK_MENU_ITEM (menu_item),
                               w_current->bottom_docking_area_menu);
  else if (action == action_docking_area_right)
    gtk_menu_item_set_submenu (GTK_MENU_ITEM (menu_item),
                               w_current->right_docking_area_menu);
  else if (action == action_options_grid_size) {
    GtkWidget *menu = gtk_menu_new ();
    gtk_menu_shell_append (GTK_MENU_SHELL (menu),
                           gschem_action_create_menu_item (
                             action_options_scale_up_snap_size,
                             use_menu_label, w_current));
    gtk_menu_shell_append (GTK_MENU_SHELL (menu),
                           gschem_action_create_menu_item (
                             action_options_scale_down_snap_size,
                             use_menu_label, w_current));
    gtk_widget_show_all (menu);
    gtk_menu_item_set_submenu (GTK_MENU_ITEM (menu_item), menu);
  }

  /* register callback so the action gets run */
  g_object_set_data (G_OBJECT (menu_item), "action", action);
  if (GTK_IS_CHECK_MENU_ITEM (menu_item))
    g_signal_connect (G_OBJECT (menu_item), "toggled",
                      G_CALLBACK (menu_item_toggled), w_current);
  else
    g_signal_connect (G_OBJECT (menu_item), "activate",
                      G_CALLBACK (menu_item_activate), w_current);

  /* connect menu item to the dispatcher for status updates */
  Dispatcher *dispatcher = get_dispatcher (action, w_current);
  g_signal_connect_swapped (dispatcher, "set-sensitive",
                            G_CALLBACK (gtk_widget_set_sensitive), menu_item);
  gtk_widget_set_sensitive (GTK_WIDGET (menu_item), dispatcher->sensitive);
  if (GTK_IS_CHECK_MENU_ITEM (menu_item)) {
    g_signal_connect_swapped (dispatcher, "set-active",
                              G_CALLBACK (menu_item_set_active), menu_item);
    menu_item_set_active (GTK_CHECK_MENU_ITEM (menu_item), dispatcher->active);
  }
  g_signal_connect_swapped (dispatcher, use_menu_label ? "set-menu-label"
                                                       : "set-label",
                            G_CALLBACK (gtk_label_set_label), label);
  gchar *l_str = use_menu_label ? dispatcher->menu_label : dispatcher->label;
  if (l_str != NULL)
    gtk_label_set_label (GTK_LABEL (label), l_str);

  return menu_item;
}


/******************************************************************************/


static void
spin_button_value_changed (GtkSpinButton *spin_button,
                           GschemToplevel *w_current)
{
  gtk_widget_grab_focus (w_current->drawing_area);
}

static gboolean
create_grid_size_menu_proxy (GtkToolItem *tool_item,
                             GschemToplevel *w_current)
{
  gtk_tool_item_set_proxy_menu_item (
    tool_item, "gschem-grid-size-menu-id",
    gschem_action_create_menu_item (
      action_options_grid_size, FALSE, w_current));

  return TRUE;
}

/*! \brief Create special toolbar widget for "Grid Size" action.
 *
 * This is called by \ref gschem_action_create_tool_button to create
 * the special spin button widget.
 */
static GtkToolItem *
create_grid_size_tool_item (GschemAction *action,
                            GschemToplevel *w_current)
{
  GtkWidget *spin_button = x_grid_size_sb_new (w_current);
  g_signal_connect (spin_button, "value-changed",
                    G_CALLBACK (spin_button_value_changed), w_current);

  GtkToolItem *tool_item = gtk_tool_item_new ();

  if (action->tooltip == NULL)
    gtk_widget_set_tooltip_text (GTK_WIDGET (tool_item), action->name);
  else {
    gchar *tooltip_text = g_strdup_printf ("%s\n%s", action->name,
                                                     action->tooltip);
    gtk_widget_set_tooltip_text (GTK_WIDGET (tool_item), tooltip_text);
    g_free (tooltip_text);
  }

  g_signal_connect (tool_item, "create-menu-proxy",
                    G_CALLBACK (create_grid_size_menu_proxy), w_current);

  gtk_container_add (GTK_CONTAINER (tool_item), spin_button);
  return tool_item;
}


/******************************************************************************/


static void
tool_button_toggled (GtkToggleToolButton *button, gpointer user_data);

static void
tool_button_set_active (GtkToggleToolButton *button, gboolean is_active)
{
  /* make sure the toggle handler isn't called recursively */
  g_signal_handlers_block_matched (
    button, G_SIGNAL_MATCH_FUNC, 0, 0, NULL,
    G_CALLBACK (tool_button_toggled), NULL);

  gtk_toggle_tool_button_set_active (button, is_active);

  g_signal_handlers_unblock_matched (
    button, G_SIGNAL_MATCH_FUNC, 0, 0, NULL,
    G_CALLBACK (tool_button_toggled), NULL);
}

static void
tool_button_clicked (GtkToolButton *button, gpointer user_data)
{
  GschemAction *action = g_object_get_data (G_OBJECT (button), "action");
  GschemToplevel *w_current = GSCHEM_TOPLEVEL (user_data);
  gschem_action_activate (action, w_current);
}

static void
tool_button_toggled (GtkToggleToolButton *button, gpointer user_data)
{
  /* button self-toggles when clicked--undo this first */
  tool_button_set_active (button,
                          !gtk_toggle_tool_button_get_active (button));

  GschemAction *action = g_object_get_data (G_OBJECT (button), "action");
  GschemToplevel *w_current = GSCHEM_TOPLEVEL (user_data);
  gschem_action_activate (action, w_current);
}

/*! \brief Create action toolbar button.
 *
 * Creates a GtkToolButton or GtkToggleToolButton widget (depending on
 * the action type), configures it according to the action metadata,
 * and hooks it up with the corresponding dispatcher object so it will
 * be updated when the action status changes.
 */
GtkToolItem *
gschem_action_create_tool_button (GschemAction *action,
                                  GschemToplevel *w_current)
{
  GtkToolItem *button;

  if (action == action_options_grid_size)
    return create_grid_size_tool_item (action, w_current);

  if (action->type != GSCHEM_ACTION_TYPE_ACTUATE)
    button = g_object_new (GTK_TYPE_TOGGLE_TOOL_BUTTON, NULL);
  else
    button = g_object_new (GTK_TYPE_TOOL_BUTTON, NULL);

  gtk_tool_button_set_label (GTK_TOOL_BUTTON (button), action->label);

  if (action->tooltip == NULL)
    gtk_widget_set_tooltip_text (GTK_WIDGET (button), action->name);
  else {
    gchar *tooltip_text = g_strdup_printf ("%s\n%s", action->name,
                                                     action->tooltip);
    gtk_widget_set_tooltip_text (GTK_WIDGET (button), tooltip_text);
    g_free (tooltip_text);
  }

  GtkWidget *image = gtk_image_new ();
  gtk_tool_button_set_icon_widget (GTK_TOOL_BUTTON (button), image);

  /* If there's a matching stock item, use it.
     Otherwise lookup the name in the icon theme. */
  GtkStockItem stock_item;
  if (gtk_stock_lookup (action->icon_name, &stock_item))
    gtk_image_set_from_stock (GTK_IMAGE (image), action->icon_name,
                              GTK_ICON_SIZE_LARGE_TOOLBAR);
  else
    gtk_image_set_from_icon_name (GTK_IMAGE (image), action->icon_name,
                                  GTK_ICON_SIZE_LARGE_TOOLBAR);

  /* register callback so the action gets run */
  g_object_set_data (G_OBJECT (button), "action", action);
  if (GTK_IS_TOGGLE_TOOL_BUTTON (button))
    g_signal_connect (button, "toggled",
                      G_CALLBACK (tool_button_toggled), w_current);
  else
    g_signal_connect (button, "clicked",
                      G_CALLBACK (tool_button_clicked), w_current);

  /* connect button to the dispatcher for status updates */
  Dispatcher *dispatcher = get_dispatcher (action, w_current);
  g_signal_connect_swapped (dispatcher, "set-sensitive",
                            G_CALLBACK (gtk_widget_set_sensitive), button);
  gtk_widget_set_sensitive (GTK_WIDGET (button), dispatcher->sensitive);
  if (GTK_IS_TOGGLE_TOOL_BUTTON (button)) {
    g_signal_connect_swapped (dispatcher, "set-active",
                              G_CALLBACK (tool_button_set_active), button);
    tool_button_set_active (GTK_TOGGLE_TOOL_BUTTON (button),
                            dispatcher->active);
  }
  g_signal_connect_swapped (dispatcher, "set-name",
                            G_CALLBACK (gtk_widget_set_tooltip_text), button);
  if (dispatcher->name != NULL)
    gtk_widget_set_tooltip_text (GTK_WIDGET (button), dispatcher->name);

  return button;
}
