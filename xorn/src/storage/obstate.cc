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

#include "internal.h"
#include <stdlib.h>
#include <string.h>


void *copy_data(xorn_obtype_t type, void const *src)
{
	size_t size;

	switch (type) {
	case xornsch_obtype_arc:       size = sizeof(xornsch_arc); break;
	case xornsch_obtype_box:       size = sizeof(xornsch_box); break;
	case xornsch_obtype_circle:    size = sizeof(xornsch_circle); break;
	case xornsch_obtype_component: size = sizeof(xornsch_component); break;
	case xornsch_obtype_line:      size = sizeof(xornsch_line); break;
	case xornsch_obtype_net:       size = sizeof(xornsch_net); break;
	case xornsch_obtype_path:      size = sizeof(xornsch_path); break;
	case xornsch_obtype_picture:   size = sizeof(xornsch_picture); break;
	case xornsch_obtype_text:      size = sizeof(xornsch_text); break;
	default:                       throw std::bad_alloc();
	}

	void *dest = malloc(size);
	if (dest == NULL)
		throw std::bad_alloc();

	memcpy(dest, src, size);
	return dest;
}

static void duplicate_string(xorn_string &str)
{
	if (str.len == 0) {
		str.s = NULL;
		return;
	}

	char *buf = (char *)malloc(str.len);
	if (buf == NULL)
		throw std::bad_alloc();
	memcpy(buf, str.s, str.len);
	str.s = buf;
}

static void incref_pointer(xorn_pointer &pointer)
{
	if (pointer.incref)
		pointer.incref(pointer.ptr);
}

static void decref_pointer(xorn_pointer &pointer)
{
	if (pointer.decref)
		pointer.decref(pointer.ptr);
}

static void normalize_line_attr(xornsch_line_attr &line)
{
	if (line.dash_style == 0 || line.dash_style == 1)
		line.dash_length = 0.;
	if (line.dash_style == 0)
		line.dash_space = 0.;
}

static void normalize_fill_attr(xornsch_fill_attr &fill)
{
	if (fill.type != 2 && fill.type != 3) {
		fill.width = 0.;
		fill.angle0 = 0;
		fill.pitch0 = 0.;
	}
	if (fill.type != 2) {
		fill.angle1 = 0;
		fill.pitch1 = 0.;
	}
}

obstate::obstate(xorn_obtype_t type, void const *data)
	: refcnt(1), type(type), data(copy_data(type, data))
{
	try {
		switch(type) {
		case xornsch_obtype_arc:
			normalize_line_attr(
				((xornsch_arc *)this->data)->line);
			break;
		case xornsch_obtype_box:
			normalize_line_attr(
				((xornsch_box *)this->data)->line);
			normalize_fill_attr(
				((xornsch_box *)this->data)->fill);
			break;
		case xornsch_obtype_circle:
			normalize_line_attr(
				((xornsch_circle *)this->data)->line);
			normalize_fill_attr(
				((xornsch_circle *)this->data)->fill);
			break;
		case xornsch_obtype_component:
			incref_pointer(
				((xornsch_component *)this->data)->symbol);
			break;
		case xornsch_obtype_line:
			normalize_line_attr(
				((xornsch_line *)this->data)->line);
			break;
		case xornsch_obtype_net:
			/* nothing to do */
			break;
		case xornsch_obtype_path:
			duplicate_string(
				((xornsch_path *)this->data)->pathdata);
			normalize_line_attr(
				((xornsch_path *)this->data)->line);
			normalize_fill_attr(
				((xornsch_path *)this->data)->fill);
			break;
		case xornsch_obtype_picture:
			incref_pointer(
				((xornsch_picture *)this->data)->pixmap);
			break;
		case xornsch_obtype_text:
			duplicate_string(((xornsch_text *)this->data)->text);
			break;
		default:
			throw std::bad_alloc();  /* bad object type */
		}
	} catch (std::bad_alloc const &) {
		free(this->data);
		throw;
	}
}

obstate::~obstate()
{
	switch(type) {
	case xornsch_obtype_component:
		decref_pointer(((xornsch_component *)data)->symbol);
		break;
	case xornsch_obtype_path:
		free(const_cast<char *>(((xornsch_path *)data)->pathdata.s));
		break;
	case xornsch_obtype_picture:
		decref_pointer(((xornsch_picture *)data)->pixmap);
		break;
	case xornsch_obtype_text:
		free(const_cast<char *>(((xornsch_text *)data)->text.s));
		break;
	default:
		/* do nothing */;
	}
	free(this->data);
}

void obstate::inc_refcnt()
{
	++refcnt;
}

void obstate::dec_refcnt()
{
	if (--refcnt == 0)
		delete this;
}
