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
#include <math.h>
#include <string.h>

#define NO_ERROR ((xorn_error_t) -1)


static size_t sizeof_data(xorn_obtype_t type)
{
	switch (type) {
	case xornsch_obtype_arc:       return sizeof(struct xornsch_arc);
	case xornsch_obtype_box:       return sizeof(struct xornsch_box);
	case xornsch_obtype_circle:    return sizeof(struct xornsch_circle);
	case xornsch_obtype_component: return sizeof(struct xornsch_component);
	case xornsch_obtype_line:      return sizeof(struct xornsch_line);
	case xornsch_obtype_net:       return sizeof(struct xornsch_net);
	case xornsch_obtype_path:      return sizeof(struct xornsch_path);
	case xornsch_obtype_picture:   return sizeof(struct xornsch_picture);
	case xornsch_obtype_text:      return sizeof(struct xornsch_text);
	default:                       assert(false);
	}
}

static void test(xorn_obtype_t type, const void *data, int expected_status)
{
	xorn_revision_t rev;
	xorn_object_t ob;
	size_t size = sizeof_data(type);
	void *zero = alloca(size);
	const void *out;
	xorn_error_t err;

	assert((rev = xorn_new_revision(NULL)) != NULL);
	memset(zero, 0, size);

	if (expected_status != 1) {
		err = NO_ERROR;
		assert((ob = xorn_add_object(rev, type, data, &err)) != NULL);
		assert(err == NO_ERROR);
		assert((out = xorn_get_object_data(rev, ob, type)) != NULL);
		if (expected_status == 0)
			assert(memcmp(data, out, size) == 0);
		else
			assert(memcmp(data, out, size) != 0);

		err = NO_ERROR;
		assert((ob = xorn_add_object(rev, type, zero, &err)) != NULL);
		assert(err == NO_ERROR);
		assert((out = xorn_get_object_data(rev, ob, type)) != NULL);
		assert(memcmp(zero, out, size) == 0);
		err = NO_ERROR;
		assert(xorn_set_object_data(rev, ob, type, data, &err) == 0);
		assert(err == NO_ERROR);
		assert((out = xorn_get_object_data(rev, ob, type)) != NULL);
		if (expected_status == 0)
			assert(memcmp(data, out, size) == 0);
		else
			assert(memcmp(data, out, size) != 0);
	} else {
		err = NO_ERROR;
		assert(xorn_add_object(rev, type, data, &err) == NULL);
		assert(err == xorn_error_invalid_object_data);

		err = NO_ERROR;
		assert((ob = xorn_add_object(rev, type, zero, &err)) != NULL);
		assert(err == NO_ERROR);
		assert((out = xorn_get_object_data(rev, ob, type)) != NULL);
		assert(memcmp(zero, out, size) == 0);
		err = NO_ERROR;
		assert(xorn_set_object_data(rev, ob, type, data, &err) == -1);
		assert(err == xorn_error_invalid_object_data);
	}

	xorn_free_revision(rev);
}

static void check_line_attr(xorn_obtype_t type, ptrdiff_t offset, size_t size)
{
	void *data = alloca(size);
	struct xornsch_line_attr *line = data + offset;

	memset(data, 0, size);
	line->width = -1.;		test(type, data, 1);
	line->width = 0.;		test(type, data, 0);
	line->width = 1.;		test(type, data, 0);
	line->width = INFINITY;		test(type, data, 1);

	memset(data, 0, size);
	line->cap_style = -1;		test(type, data, 1);
	line->cap_style = 0;		test(type, data, 0);
	line->cap_style = 2;		test(type, data, 0);
	line->cap_style = 3;		test(type, data, 1);

	memset(data, 0, size);
	line->dash_style = -1;		test(type, data, 1);
	line->dash_style = 0;		test(type, data, 0);
	line->dash_style = 4;		test(type, data, 0);
	line->dash_style = 5;		test(type, data, 1);

	memset(data, 0, size);
	line->dash_length = -1.;	test(type, data, 1);
	line->dash_length = 0.;		test(type, data, 0);
	line->dash_length = 1.;		test(type, data, 2);
	line->dash_length = INFINITY;	test(type, data, 1);

	memset(data, 0, size);
	line->dash_space = -1.;		test(type, data, 1);
	line->dash_space = 0.;		test(type, data, 0);
	line->dash_space = 1.;		test(type, data, 2);
	line->dash_space = INFINITY;	test(type, data, 1);
}

static void check_fill_attr(xorn_obtype_t type, ptrdiff_t offset, size_t size)
{
	void *data = alloca(size);
	struct xornsch_fill_attr *fill = data + offset;

	memset(data, 0, size);
	fill->type = -1;		test(type, data, 1);
	fill->type = 0;			test(type, data, 0);
	fill->type = 4;			test(type, data, 0);
	fill->type = 5;			test(type, data, 1);

	memset(data, 0, size);
	fill->width = -1.;		test(type, data, 1);
	fill->width = 0.;		test(type, data, 0);
	fill->width = 1.;		test(type, data, 2);
	fill->width = INFINITY;		test(type, data, 1);

	memset(data, 0, size);
	fill->angle0 = -1;		test(type, data, 2);
	fill->angle0 = 0;		test(type, data, 0);
	fill->angle0 = 1;		test(type, data, 2);
	fill->angle0 = 360;		test(type, data, 2);

	memset(data, 0, size);
	fill->pitch0 = -1.;		test(type, data, 1);
	fill->pitch0 = 0.;		test(type, data, 0);
	fill->pitch0 = 1.;		test(type, data, 2);
	fill->pitch0 = INFINITY;	test(type, data, 1);

	memset(data, 0, size);
	fill->angle1 = -1;		test(type, data, 2);
	fill->angle1 = 0;		test(type, data, 0);
	fill->angle1 = 1;		test(type, data, 2);
	fill->angle1 = 360;		test(type, data, 2);

	memset(data, 0, size);
	fill->pitch1 = -1.;		test(type, data, 1);
	fill->pitch1 = 0.;		test(type, data, 0);
	fill->pitch1 = 1.;		test(type, data, 2);
	fill->pitch1 = INFINITY;	test(type, data, 1);
}

static void check_arc(void)
{
	struct xornsch_arc data;

	memset(&data, 0, sizeof data);
	data.pos.x = -1.;		test(xornsch_obtype_arc, &data, 0);
	data.pos.x = 0.;		test(xornsch_obtype_arc, &data, 0);
	data.pos.x = 1.;		test(xornsch_obtype_arc, &data, 0);
	data.pos.x = INFINITY;		test(xornsch_obtype_arc, &data, 1);

	memset(&data, 0, sizeof data);
	data.pos.y = -1.;		test(xornsch_obtype_arc, &data, 0);
	data.pos.y = 0.;		test(xornsch_obtype_arc, &data, 0);
	data.pos.y = 1.;		test(xornsch_obtype_arc, &data, 0);
	data.pos.y = INFINITY;		test(xornsch_obtype_arc, &data, 1);

	memset(&data, 0, sizeof data);
	data.radius = -1.;		test(xornsch_obtype_arc, &data, 1);
	data.radius = 0.;		test(xornsch_obtype_arc, &data, 0);
	data.radius = 1.;		test(xornsch_obtype_arc, &data, 0);
	data.radius = INFINITY;		test(xornsch_obtype_arc, &data, 1);

	memset(&data, 0, sizeof data);
	data.startangle = -1;		test(xornsch_obtype_arc, &data, 0);
	data.startangle = 0;		test(xornsch_obtype_arc, &data, 0);
	data.startangle = 1;		test(xornsch_obtype_arc, &data, 0);
	data.startangle = 360;		test(xornsch_obtype_arc, &data, 0);

	memset(&data, 0, sizeof data);
	data.sweepangle = -1;		test(xornsch_obtype_arc, &data, 0);
	data.sweepangle = 0;		test(xornsch_obtype_arc, &data, 0);
	data.sweepangle = 1;		test(xornsch_obtype_arc, &data, 0);
	data.sweepangle = 360;		test(xornsch_obtype_arc, &data, 0);

	memset(&data, 0, sizeof data);
	data.color = -1;		test(xornsch_obtype_arc, &data, 1);
	data.color = 0;			test(xornsch_obtype_arc, &data, 0);
	data.color = 20;		test(xornsch_obtype_arc, &data, 0);
	data.color = 21;		test(xornsch_obtype_arc, &data, 1);

	check_line_attr(xornsch_obtype_arc,
			offsetof(struct xornsch_arc, line),
			sizeof(struct xornsch_arc));
}

static void check_box(void)
{
	struct xornsch_box data;

	memset(&data, 0, sizeof data);
	data.pos.x = -1.;		test(xornsch_obtype_box, &data, 0);
	data.pos.x = 0.;		test(xornsch_obtype_box, &data, 0);
	data.pos.x = 1.;		test(xornsch_obtype_box, &data, 0);
	data.pos.x = INFINITY;		test(xornsch_obtype_box, &data, 1);

	memset(&data, 0, sizeof data);
	data.pos.y = -1.;		test(xornsch_obtype_box, &data, 0);
	data.pos.y = 0.;		test(xornsch_obtype_box, &data, 0);
	data.pos.y = 1.;		test(xornsch_obtype_box, &data, 0);
	data.pos.y = INFINITY;		test(xornsch_obtype_box, &data, 1);

	memset(&data, 0, sizeof data);
	data.size.x = -1.;		test(xornsch_obtype_box, &data, 0);
	data.size.x = 0.;		test(xornsch_obtype_box, &data, 0);
	data.size.x = 1.;		test(xornsch_obtype_box, &data, 0);
	data.size.x = INFINITY;		test(xornsch_obtype_box, &data, 1);

	memset(&data, 0, sizeof data);
	data.size.y = -1.;		test(xornsch_obtype_box, &data, 0);
	data.size.y = 0.;		test(xornsch_obtype_box, &data, 0);
	data.size.y = 1.;		test(xornsch_obtype_box, &data, 0);
	data.size.y = INFINITY;		test(xornsch_obtype_box, &data, 1);

	memset(&data, 0, sizeof data);
	data.color = -1;		test(xornsch_obtype_box, &data, 1);
	data.color = 0;			test(xornsch_obtype_box, &data, 0);
	data.color = 20;		test(xornsch_obtype_box, &data, 0);
	data.color = 21;		test(xornsch_obtype_box, &data, 1);

	check_line_attr(xornsch_obtype_box,
			offsetof(struct xornsch_box, line),
			sizeof(struct xornsch_box));

	check_fill_attr(xornsch_obtype_box,
			offsetof(struct xornsch_box, fill),
			sizeof(struct xornsch_box));
}

static void check_circle(void)
{
	struct xornsch_circle data;

	memset(&data, 0, sizeof data);
	data.pos.x = -1.;		test(xornsch_obtype_circle, &data, 0);
	data.pos.x = 0.;		test(xornsch_obtype_circle, &data, 0);
	data.pos.x = 1.;		test(xornsch_obtype_circle, &data, 0);
	data.pos.x = INFINITY;		test(xornsch_obtype_circle, &data, 1);

	memset(&data, 0, sizeof data);
	data.pos.y = -1.;		test(xornsch_obtype_circle, &data, 0);
	data.pos.y = 0.;		test(xornsch_obtype_circle, &data, 0);
	data.pos.y = 1.;		test(xornsch_obtype_circle, &data, 0);
	data.pos.y = INFINITY;		test(xornsch_obtype_circle, &data, 1);

	memset(&data, 0, sizeof data);
	data.radius = -1.;		test(xornsch_obtype_circle, &data, 1);
	data.radius = 0.;		test(xornsch_obtype_circle, &data, 0);
	data.radius = 1.;		test(xornsch_obtype_circle, &data, 0);
	data.radius = INFINITY;		test(xornsch_obtype_circle, &data, 1);

	memset(&data, 0, sizeof data);
	data.color = -1;		test(xornsch_obtype_circle, &data, 1);
	data.color = 0;			test(xornsch_obtype_circle, &data, 0);
	data.color = 20;		test(xornsch_obtype_circle, &data, 0);
	data.color = 21;		test(xornsch_obtype_circle, &data, 1);

	check_line_attr(xornsch_obtype_circle,
			offsetof(struct xornsch_circle, line),
			sizeof(struct xornsch_circle));

	check_fill_attr(xornsch_obtype_circle,
			offsetof(struct xornsch_circle, fill),
			sizeof(struct xornsch_circle));
}

static void check_component(void)
{
	struct xornsch_component data;

	memset(&data, 0, sizeof data);
	data.pos.x = -1.;	test(xornsch_obtype_component, &data, 0);
	data.pos.x = 0.;	test(xornsch_obtype_component, &data, 0);
	data.pos.x = 1.;	test(xornsch_obtype_component, &data, 0);
	data.pos.x = INFINITY;	test(xornsch_obtype_component, &data, 1);

	memset(&data, 0, sizeof data);
	data.pos.y = -1.;	test(xornsch_obtype_component, &data, 0);
	data.pos.y = 0.;	test(xornsch_obtype_component, &data, 0);
	data.pos.y = 1.;	test(xornsch_obtype_component, &data, 0);
	data.pos.y = INFINITY;	test(xornsch_obtype_component, &data, 1);

	memset(&data, 0, sizeof data);
	data.angle = -1;	test(xornsch_obtype_component, &data, 1);
	data.angle = 0;		test(xornsch_obtype_component, &data, 0);
	data.angle = 1;		test(xornsch_obtype_component, &data, 1);
	data.angle = 270;	test(xornsch_obtype_component, &data, 0);
	data.angle = 360;	test(xornsch_obtype_component, &data, 1);
}

static void check_line(void)
{
	struct xornsch_line data;

	memset(&data, 0, sizeof data);
	data.pos.x = -1.;		test(xornsch_obtype_line, &data, 0);
	data.pos.x = 0.;		test(xornsch_obtype_line, &data, 0);
	data.pos.x = 1.;		test(xornsch_obtype_line, &data, 0);
	data.pos.x = INFINITY;		test(xornsch_obtype_line, &data, 1);

	memset(&data, 0, sizeof data);
	data.pos.y = -1.;		test(xornsch_obtype_line, &data, 0);
	data.pos.y = 0.;		test(xornsch_obtype_line, &data, 0);
	data.pos.y = 1.;		test(xornsch_obtype_line, &data, 0);
	data.pos.y = INFINITY;		test(xornsch_obtype_line, &data, 1);

	memset(&data, 0, sizeof data);
	data.size.x = -1.;		test(xornsch_obtype_line, &data, 0);
	data.size.x = 0.;		test(xornsch_obtype_line, &data, 0);
	data.size.x = 1.;		test(xornsch_obtype_line, &data, 0);
	data.size.x = INFINITY;		test(xornsch_obtype_line, &data, 1);

	memset(&data, 0, sizeof data);
	data.size.y = -1.;		test(xornsch_obtype_line, &data, 0);
	data.size.y = 0.;		test(xornsch_obtype_line, &data, 0);
	data.size.y = 1.;		test(xornsch_obtype_line, &data, 0);
	data.size.y = INFINITY;		test(xornsch_obtype_line, &data, 1);

	memset(&data, 0, sizeof data);
	data.color = -1;		test(xornsch_obtype_line, &data, 1);
	data.color = 0;			test(xornsch_obtype_line, &data, 0);
	data.color = 20;		test(xornsch_obtype_line, &data, 0);
	data.color = 21;		test(xornsch_obtype_line, &data, 1);

	check_line_attr(xornsch_obtype_line,
			offsetof(struct xornsch_line, line),
			sizeof(struct xornsch_line));
}

static void check_net(void)
{
	struct xornsch_net data;

	memset(&data, 0, sizeof data);
	data.pos.x = -1.;		test(xornsch_obtype_net, &data, 0);
	data.pos.x = 0.;		test(xornsch_obtype_net, &data, 0);
	data.pos.x = 1.;		test(xornsch_obtype_net, &data, 0);
	data.pos.x = INFINITY;		test(xornsch_obtype_net, &data, 1);

	memset(&data, 0, sizeof data);
	data.pos.y = -1.;		test(xornsch_obtype_net, &data, 0);
	data.pos.y = 0.;		test(xornsch_obtype_net, &data, 0);
	data.pos.y = 1.;		test(xornsch_obtype_net, &data, 0);
	data.pos.y = INFINITY;		test(xornsch_obtype_net, &data, 1);

	memset(&data, 0, sizeof data);
	data.size.x = -1.;		test(xornsch_obtype_net, &data, 0);
	data.size.x = 0.;		test(xornsch_obtype_net, &data, 0);
	data.size.x = 1.;		test(xornsch_obtype_net, &data, 0);
	data.size.x = INFINITY;		test(xornsch_obtype_net, &data, 1);

	memset(&data, 0, sizeof data);
	data.size.y = -1.;		test(xornsch_obtype_net, &data, 0);
	data.size.y = 0.;		test(xornsch_obtype_net, &data, 0);
	data.size.y = 1.;		test(xornsch_obtype_net, &data, 0);
	data.size.y = INFINITY;		test(xornsch_obtype_net, &data, 1);

	memset(&data, 0, sizeof data);
	data.color = -1;		test(xornsch_obtype_net, &data, 1);
	data.color = 0;			test(xornsch_obtype_net, &data, 0);
	data.color = 20;		test(xornsch_obtype_net, &data, 0);
	data.color = 21;		test(xornsch_obtype_net, &data, 1);

	memset(&data, 0, sizeof data);
	data.is_inverted = false;	test(xornsch_obtype_net, &data, 0);
	data.is_inverted = true;	test(xornsch_obtype_net, &data, 1);
	data.is_pin = true;
	data.is_inverted = false;	test(xornsch_obtype_net, &data, 0);
	data.is_inverted = true;	test(xornsch_obtype_net, &data, 0);
}

static void check_path(void)
{
	struct xornsch_path data;

	memset(&data, 0, sizeof data);
	data.color = -1;		test(xornsch_obtype_path, &data, 1);
	data.color = 0;			test(xornsch_obtype_path, &data, 0);
	data.color = 20;		test(xornsch_obtype_path, &data, 0);
	data.color = 21;		test(xornsch_obtype_path, &data, 1);

	check_line_attr(xornsch_obtype_path,
			offsetof(struct xornsch_path, line),
			sizeof(struct xornsch_path));

	check_fill_attr(xornsch_obtype_path,
			offsetof(struct xornsch_path, fill),
			sizeof(struct xornsch_path));
}

static void check_picture(void)
{
	struct xornsch_picture data;

	memset(&data, 0, sizeof data);
	data.pos.x = -1.;		test(xornsch_obtype_picture, &data, 0);
	data.pos.x = 0.;		test(xornsch_obtype_picture, &data, 0);
	data.pos.x = 1.;		test(xornsch_obtype_picture, &data, 0);
	data.pos.x = INFINITY;		test(xornsch_obtype_picture, &data, 1);

	memset(&data, 0, sizeof data);
	data.pos.y = -1.;		test(xornsch_obtype_picture, &data, 0);
	data.pos.y = 0.;		test(xornsch_obtype_picture, &data, 0);
	data.pos.y = 1.;		test(xornsch_obtype_picture, &data, 0);
	data.pos.y = INFINITY;		test(xornsch_obtype_picture, &data, 1);

	memset(&data, 0, sizeof data);
	data.size.x = -1.;		test(xornsch_obtype_picture, &data, 0);
	data.size.x = 0.;		test(xornsch_obtype_picture, &data, 0);
	data.size.x = 1.;		test(xornsch_obtype_picture, &data, 0);
	data.size.x = INFINITY;		test(xornsch_obtype_picture, &data, 1);

	memset(&data, 0, sizeof data);
	data.size.y = -1.;		test(xornsch_obtype_picture, &data, 0);
	data.size.y = 0.;		test(xornsch_obtype_picture, &data, 0);
	data.size.y = 1.;		test(xornsch_obtype_picture, &data, 0);
	data.size.y = INFINITY;		test(xornsch_obtype_picture, &data, 1);

	memset(&data, 0, sizeof data);
	data.angle = -1;		test(xornsch_obtype_picture, &data, 1);
	data.angle = 0;			test(xornsch_obtype_picture, &data, 0);
	data.angle = 1;			test(xornsch_obtype_picture, &data, 1);
	data.angle = 270;		test(xornsch_obtype_picture, &data, 0);
	data.angle = 360;		test(xornsch_obtype_picture, &data, 1);
}

static void check_text(void)
{
	struct xornsch_text data;

	memset(&data, 0, sizeof data);
	data.pos.x = -1.;		test(xornsch_obtype_text, &data, 0);
	data.pos.x = 0.;		test(xornsch_obtype_text, &data, 0);
	data.pos.x = 1.;		test(xornsch_obtype_text, &data, 0);
	data.pos.x = INFINITY;		test(xornsch_obtype_text, &data, 1);

	memset(&data, 0, sizeof data);
	data.pos.y = -1.;		test(xornsch_obtype_text, &data, 0);
	data.pos.y = 0.;		test(xornsch_obtype_text, &data, 0);
	data.pos.y = 1.;		test(xornsch_obtype_text, &data, 0);
	data.pos.y = INFINITY;		test(xornsch_obtype_text, &data, 1);

	memset(&data, 0, sizeof data);
	data.color = -1;		test(xornsch_obtype_text, &data, 1);
	data.color = 0;			test(xornsch_obtype_text, &data, 0);
	data.color = 20;		test(xornsch_obtype_text, &data, 0);
	data.color = 21;		test(xornsch_obtype_text, &data, 1);

	memset(&data, 0, sizeof data);
	data.text_size = -1;		test(xornsch_obtype_text, &data, 1);
	data.text_size = 0;		test(xornsch_obtype_text, &data, 0);
	data.text_size = 1;		test(xornsch_obtype_text, &data, 0);
	data.text_size = 1000000;	test(xornsch_obtype_text, &data, 0);

	memset(&data, 0, sizeof data);
	data.show_name_value = -1;	test(xornsch_obtype_text, &data, 1);
	data.show_name_value = 0;	test(xornsch_obtype_text, &data, 0);
	data.show_name_value = 1;	test(xornsch_obtype_text, &data, 0);
	data.show_name_value = 2;	test(xornsch_obtype_text, &data, 0);
	data.show_name_value = 3;	test(xornsch_obtype_text, &data, 1);

	memset(&data, 0, sizeof data);
	data.angle = -1;		test(xornsch_obtype_text, &data, 1);
	data.angle = 0;			test(xornsch_obtype_text, &data, 0);
	data.angle = 1;			test(xornsch_obtype_text, &data, 1);
	data.angle = 270;		test(xornsch_obtype_text, &data, 0);
	data.angle = 360;		test(xornsch_obtype_text, &data, 1);

	memset(&data, 0, sizeof data);
	data.alignment = -1;		test(xornsch_obtype_text, &data, 1);
	data.alignment = 0;		test(xornsch_obtype_text, &data, 0);
	data.alignment = 8;		test(xornsch_obtype_text, &data, 0);
	data.alignment = 9;		test(xornsch_obtype_text, &data, 1);
}

int main(void)
{
	check_arc();
	check_box();
	check_circle();
	check_component();
	check_line();
	check_net();
	check_path();
	check_picture();
	check_text();

	return 0;
}
