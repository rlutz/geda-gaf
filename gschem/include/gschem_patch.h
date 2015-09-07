/* gEDA - GPL Electronic Design Automation
 * gschem - gEDA Schematic Capture
 * Copyright (C) 2015 gEDA Contributors (see ChangeLog for details)
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
 * \file gschem_patch.h
 *
 * \brief Back annotation code; handling patches coming from external tools.
 */

/* TODO: run indent(1) to get indentation compatible with geda policy */

typedef enum {
	GSCHEM_PATCH_DEL_CONN,
	GSCHEM_PATCH_ADD_CONN,
	GSCHEM_PATCH_CHANGE_ATTRIB,
	GSCHEM_PATCH_NET_INFO,
} gschem_patch_op_t;

/* this type of object should be only on a search list */
#define OBJ_PATCH 'p'

typedef struct gschem_patch_line_s gschem_patch_line_t;
struct gschem_patch_line_s {
	gschem_patch_op_t op;
	char *id;
	union {
		char *net_name;
		char *attrib_name;
		GList *ids;    /* for net_info */
	} arg1;
	union {
		char *attrib_val;
	} arg2;

	gschem_patch_line_t *next;
};

typedef struct {
	GList *lines;        /* an ordered list of patch lines (of gschem_patch_line_t *)  */
	GHashTable *pins;    /* refdes-pinnumber -> pin object */
	GHashTable *nets;    /* net_name -> GList* of pins as seen by the sender */
} gschem_patch_state_t;

typedef struct {
	OBJECT *object;
	char *text;
} gschem_patch_hit_t;

int gschem_patch_state_init(gschem_patch_state_t *st, const char *fn);
int gschem_patch_state_build(gschem_patch_state_t *st, OBJECT *o);
GSList *gschem_patch_state_execute(gschem_patch_state_t *st, GSList *diffs);
void gschem_patch_state_destroy(gschem_patch_state_t *st);
