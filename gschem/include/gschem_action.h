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

#ifndef GSCHEM_ACTION_H
#define GSCHEM_ACTION_H

#define GSCHEM_ACTION(obj) ((GschemAction *) (obj))

typedef struct _GschemAction GschemAction;

struct _GschemAction {
  gchar *id;
  gchar *icon_name;
  gchar *name;
  gchar *label;
  gchar *menu_label;
  gchar *tooltip;

  void (*activate) (GschemToplevel *w_current);
};


int scm_is_action (SCM x);
GschemAction *scm_to_action (SCM smob);

void gschem_action_activate (GschemAction *action,
                             GschemToplevel *w_current);

GschemAction *gschem_action_register (gchar *id,
                                      gchar *icon_name,
                                      gchar *name,
                                      gchar *label,
                                      gchar *menu_label,
                                      gchar *tooltip,
                                      void (*activate) (GschemToplevel *));

void gschem_action_init (void);

#endif /* GSCHEM_ACTION_H */
