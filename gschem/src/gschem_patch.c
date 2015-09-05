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

/* This file hosts the back annotation code; it is handling patches
coming from external tools. */

/* TODO: run indent(1) to get indentation compatible with geda policy */

#include "gschem.h"

static void patch_list_free(GList *list);

#define error(...) \
do { \
	fprintf(stderr, __VA_ARGS__); \
	fprintf(stderr, " in line %d\n", lineno); \
	patch_list_free(st->lines); \
	free(word); \
	return -1; \
} while(0)


#define append(c) \
do { \
	if (used >= alloced) { \
		alloced += 64; \
		word = realloc(word, alloced); \
	} \
	word[used] = c; \
	used++; \
} while(0)

#define reset_current() \
do { \
	used = 0; \
	memset(&current, 0, sizeof(current)); \
} while(0)

#define reset_word() \
do { \
	used = 0; \
} while(0)

#define restart(c) \
do { \
	reset_word(); \
	append(c); \
} while(0)

#define END_OP() \
do { \
	append('\0'); \
	if (strcmp(word, "add_conn") == 0) current.op = GSCHEM_PATCH_ADD_CONN; \
	else if (strcmp(word, "del_conn") == 0) current.op = GSCHEM_PATCH_DEL_CONN; \
	else if (strcmp(word, "change_attrib") == 0) current.op = GSCHEM_PATCH_CHANGE_ATTRIB; \
	else if (strcmp(word, "net_info") == 0) current.op = GSCHEM_PATCH_NET_INFO; \
	else \
		error("Syntax error: unknown opcode %s\n", word); \
	reset_word(); \
} while(0)

#define END_STR() \
do { \
	append('\0'); \
	if (*word != '\0') {\
		switch(current.op) { \
			case GSCHEM_PATCH_DEL_CONN: \
			case GSCHEM_PATCH_ADD_CONN: \
				if (current.id == NULL) current.id = strdup(word); \
				else if (current.arg1.net_name == NULL) current.arg1.net_name = strdup(word); \
				else error("Need two arguments for the connection: netname and pinname"); \
				break; \
			case GSCHEM_PATCH_CHANGE_ATTRIB: \
				if (current.id == NULL) current.id = strdup(word); \
				else if (current.arg1.attrib_name == NULL) current.arg1.attrib_name = strdup(word); \
				else if (current.arg2.attrib_val == NULL) current.arg2.attrib_val = strdup(word); \
				else error("Need three arguments for an attrib change: id attr_name attr_val"); \
				break; \
			case GSCHEM_PATCH_NET_INFO: \
				if (current.id == NULL) current.id = strdup(word); \
				else current.arg1.ids = g_list_prepend(current.arg1.ids, strdup(word)); \
		} \
	} \
	reset_word(); \
} while(0)

#define require(s) \
do { \
	if (s == NULL) \
		error("Not enough arguments"); \
} while(0)

#define END_LINE() \
do { \
	gschem_patch_line_t *n; \
	switch(current.op) { \
		case GSCHEM_PATCH_DEL_CONN: \
		case GSCHEM_PATCH_ADD_CONN: \
			require(current.id); \
			require(current.arg1.net_name); \
			break; \
		case GSCHEM_PATCH_CHANGE_ATTRIB: \
			require(current.id); \
			require(current.arg1.attrib_name); \
			require(current.arg2.attrib_val); \
			break; \
		case GSCHEM_PATCH_NET_INFO: \
			require(current.id); \
			break; \
	} \
	n = malloc(sizeof(gschem_patch_line_t)); \
	memcpy(n, &current, sizeof(gschem_patch_line_t)); \
	st->lines = g_list_prepend(st->lines, n); \
	reset_current(); \
} while(0)

static int patch_parse(gschem_patch_state_t *st, FILE *f)
{
	char *word = NULL;
	int alloced = 0, used;
	int c, lineno;
	gschem_patch_line_t current;
	enum {
		ST_INIT,
		ST_COMMENT,
		ST_OP,
		ST_PRE_STR,
		ST_STR
	} state = ST_INIT;

	st->lines = g_list_alloc();
	lineno = 1;
	reset_current();
	do {
		c = fgetc(f);
		switch(state) {
			case ST_INIT:
				switch(c) {
					case '#': state = ST_COMMENT; break;
					case '\r':
					case '\n':
					case EOF:
					case ' ':
					case '\t':
						break;
					default:
						restart(c);
						state = ST_OP;
				}
				break;
			case ST_OP:
				switch(c) {
					case '#': END_OP(); END_LINE(); state = ST_COMMENT; break;
					case ' ':
					case '\t':
						END_OP();
						state = ST_PRE_STR;
						break;
					case '\r':
					case '\n':
					case EOF:
						END_OP();
						END_LINE();
						state = ST_INIT;
						break;
					default:
						append(c);
						break;
				}
				break;
			case ST_PRE_STR:
				switch(c) {
					case '#': END_LINE(); state = ST_COMMENT; break;
					case ' ':
					case '\t':
						break;
					case '\r':
					case '\n':
					case EOF:
						END_LINE();
						state = ST_INIT;
						break;
					default:
						restart(c);
						state = ST_STR;
						break;
				}
				break;
			case ST_STR:
				switch(c) {
					case '#': END_STR(); END_LINE(); state = ST_COMMENT; break;
					case ' ':
					case '\t':
						END_STR();
						state = ST_PRE_STR;
						break;
					case '\r':
					case '\n':
					case EOF:
						END_STR();
						END_LINE();
						state = ST_INIT;
						break;
					default:
						append(c);
						break;
				}
				break;
			case ST_COMMENT:
				switch(c) {
					case '\r':
					case '\n':
					case EOF:
						state = ST_INIT;
						break;
				}
				break;
		}
		if (c == '\n')
			lineno++;
	} while(c != EOF);
	st->lines = g_list_reverse(st->lines);
	return 0;
}

static void patch_list_free(GList *list)
{
	
}

int gschem_patch_state_init(gschem_patch_state_t *st, const char *fn)
{
	FILE *f;
	int res;

	f = fopen(fn, "r");
	if (f == NULL)
		return -1;

	res = patch_parse(st, f);

	fclose(f);
	return res;
}

