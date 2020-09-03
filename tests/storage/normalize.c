/* Copyright (C) 2013-2020 Roland Lutz

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software Foundation,
   Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.  */

#include <xornstorage.h>
#include <alloca.h>
#include <assert.h>
#include <string.h>


static void assert_line(xorn_revision_t rev, xorn_object_t ob,
			double width, int cap_style, int dash_style,
			double dash_length, double dash_space)
{
	xorn_selection_t sel;
	xorn_attst_t state = xorn_attst_na;
	struct xornsch_line_attr expected_line, real_line;

	memset(&expected_line, 0, sizeof expected_line);
	expected_line.width = width;
	expected_line.cap_style = cap_style;
	expected_line.dash_style = dash_style;
	expected_line.dash_length = dash_length;
	expected_line.dash_space = dash_space;

	sel = xorn_select_object(ob);
	assert(sel != NULL);
	xornsch_get_line(rev, sel, &state, &real_line);
	assert(state == xorn_attst_consistent);
	assert(memcmp(&expected_line, &real_line, sizeof expected_line) == 0);
	xorn_free_selection(sel);
}

static void assert_fill(xorn_revision_t rev, xorn_object_t ob,
			int type, double width, int angle0, double pitch0,
			int angle1, double pitch1)
{
	xorn_selection_t sel;
	xorn_attst_t state = xorn_attst_na;
	struct xornsch_fill_attr expected_fill, real_fill;

	memset(&expected_fill, 0, sizeof expected_fill);
	expected_fill.type = type;
	expected_fill.width = width;
	expected_fill.angle0 = angle0;
	expected_fill.pitch0 = pitch0;
	expected_fill.angle1 = angle1;
	expected_fill.pitch1 = pitch1;

	sel = xorn_select_object(ob);
	assert(sel != NULL);
	xornsch_get_fill(rev, sel, &state, &real_fill);
	assert(state == xorn_attst_consistent);
	assert(memcmp(&expected_fill, &real_fill, sizeof expected_fill) == 0);
	xorn_free_selection(sel);
}

static void check_line_attr(xorn_revision_t rev, xorn_object_t ob,
			    xorn_obtype_t type, ptrdiff_t offset, size_t size)
{
	void *data = alloca(size);
	struct xornsch_line_attr *line = data + offset;

	memset(data, 0, size);
	line->width = 70.;
	line->cap_style = 1;
	line->dash_length = 73.;
	line->dash_space = 74.;

	line->dash_style = 0;
	assert(xorn_set_object_data(rev, ob, type, data, NULL) == 0);
	assert_line(rev, ob, 70., 1, 0, 0., 0.);

	line->dash_style = 1;
	assert(xorn_set_object_data(rev, ob, type, data, NULL) == 0);
	assert_line(rev, ob, 70., 1, 1, 0., 74.);

	line->dash_style = 2;
	assert(xorn_set_object_data(rev, ob, type, data, NULL) == 0);
	assert_line(rev, ob, 70., 1, 2, 73., 74.);

	line->dash_style = 3;
	assert(xorn_set_object_data(rev, ob, type, data, NULL) == 0);
	assert_line(rev, ob, 70., 1, 3, 73., 74.);

	line->dash_style = 4;
	assert(xorn_set_object_data(rev, ob, type, data, NULL) == 0);
	assert_line(rev, ob, 70., 1, 4, 73., 74.);
}

static void check_fill_attr(xorn_revision_t rev, xorn_object_t ob,
			    xorn_obtype_t type, ptrdiff_t offset, size_t size)
{
	void *data = alloca(size);
	struct xornsch_fill_attr *fill = data + offset;

	memset(data, 0, size);
	fill->width = 81.;
	fill->angle0 = 82;
	fill->pitch0 = 83.;
	fill->angle1 = 84;
	fill->pitch1 = 85.;

	fill->type = 0;
	assert(xorn_set_object_data(rev, ob, type, data, NULL) == 0);
	assert_fill(rev, ob, 0, 0., 0, 0., 0, 0.);

	fill->type = 1;
	assert(xorn_set_object_data(rev, ob, type, data, NULL) == 0);
	assert_fill(rev, ob, 1, 0., 0, 0., 0, 0.);

	fill->type = 2;
	assert(xorn_set_object_data(rev, ob, type, data, NULL) == 0);
	assert_fill(rev, ob, 2, 81., 82, 83., 84, 85.);

	fill->type = 3;
	assert(xorn_set_object_data(rev, ob, type, data, NULL) == 0);
	assert_fill(rev, ob, 3, 81., 82, 83., 0, 0.);

	fill->type = 4;
	assert(xorn_set_object_data(rev, ob, type, data, NULL) == 0);
	assert_fill(rev, ob, 4, 0., 0, 0., 0, 0.);
}

static void check_string(xorn_revision_t rev, xorn_object_t ob,
			 xorn_obtype_t type, ptrdiff_t offset, size_t size)
{
	void *data = alloca(size);
	struct xorn_string *in = data + offset;
	const struct xorn_string *out;

	memset(data, 0, size);
	in->s = "ABCDE";

	in->len = 3;
	assert(xorn_set_object_data(rev, ob, type, data, NULL) == 0);
	out = xorn_get_object_data(rev, ob, type) + offset;
	assert(out->s != NULL);
	assert(out->s != in->s);
	assert(memcmp(in->s, out->s, 3) == 0);
	assert(out->len == 3);

	in->len = 0;
	assert(xorn_set_object_data(rev, ob, type, data, NULL) == 0);
	out = xorn_get_object_data(rev, ob, type) + offset;
	assert(out->s == NULL);
	assert(out->len == 0);
}

int main(void)
{
	xorn_revision_t rev;
	xorn_object_t ob;
	struct xornsch_net net;

	rev = xorn_new_revision(NULL);
	assert(rev != NULL);
	memset(&net, 0, sizeof net);
	ob = xornsch_add_net(rev, &net, NULL);
	assert(ob != NULL);

	/* arc */
	check_line_attr(rev, ob, xornsch_obtype_arc,
			offsetof(struct xornsch_arc, line),
			sizeof(struct xornsch_arc));

	/* box */
	check_line_attr(rev, ob, xornsch_obtype_box,
			offsetof(struct xornsch_box, line),
			sizeof(struct xornsch_box));

	check_fill_attr(rev, ob, xornsch_obtype_box,
			offsetof(struct xornsch_box, fill),
			sizeof(struct xornsch_box));

	/* circle */
	check_line_attr(rev, ob, xornsch_obtype_circle,
			offsetof(struct xornsch_circle, line),
			sizeof(struct xornsch_circle));

	check_fill_attr(rev, ob, xornsch_obtype_circle,
			offsetof(struct xornsch_circle, fill),
			sizeof(struct xornsch_circle));

	/* component */

	/* line */
	check_line_attr(rev, ob, xornsch_obtype_line,
			offsetof(struct xornsch_line, line),
			sizeof(struct xornsch_line));

	/* net */

	/* path */
	check_string(rev, ob, xornsch_obtype_path,
		     offsetof(struct xornsch_path, pathdata),
		     sizeof(struct xornsch_path));

	check_line_attr(rev, ob, xornsch_obtype_path,
			offsetof(struct xornsch_path, line),
			sizeof(struct xornsch_path));

	check_fill_attr(rev, ob, xornsch_obtype_path,
			offsetof(struct xornsch_path, fill),
			sizeof(struct xornsch_path));

	/* picture */

	/* text */
	check_string(rev, ob, xornsch_obtype_text,
		     offsetof(struct xornsch_text, text),
		     sizeof(struct xornsch_text));

	xorn_free_revision(rev);
	return 0;
}
