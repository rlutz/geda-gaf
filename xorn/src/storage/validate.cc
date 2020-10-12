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
#include <math.h>


static bool bool_is_valid(bool x)
{
	/* this is probably optimized out */
	return x == false || x == true;
}

static bool double_is_valid(double x)
{
	int c = fpclassify(x);
	return c == FP_ZERO || c == FP_NORMAL;
}

static bool double_is_valid_nonnegative(double x)
{
	int c = fpclassify(x);
	return c == FP_ZERO || (c == FP_NORMAL && x > 0.);
}

static bool angle_is_valid(int angle)
{
	return angle == 0 || angle == 90 || angle == 180 || angle == 270;
}

static bool string_is_valid(struct xorn_string const &string)
{
	return string.len == 0 || string.s != NULL;
}

static bool line_is_valid(struct xornsch_line_attr const &line)
{
	return double_is_valid_nonnegative(line.width)
	    && line.cap_style >= 0 && line.cap_style < 3
	    && line.dash_style >= 0 && line.dash_style < 5
	    && double_is_valid_nonnegative(line.dash_length)
	    && double_is_valid_nonnegative(line.dash_space);
}

static bool fill_is_valid(struct xornsch_fill_attr const &fill)
{
	return fill.type >= 0 && fill.type < 5
	    && double_is_valid_nonnegative(fill.width)
	    && double_is_valid_nonnegative(fill.pitch0)
	    && double_is_valid_nonnegative(fill.pitch1);
}

bool data_is_valid(xorn_obtype_t type, void const *data)
{
	switch (type) {
	case xornsch_obtype_arc: {
		xornsch_arc const &d = *(xornsch_arc const *)data;
		return double_is_valid(d.pos.x)
		    && double_is_valid(d.pos.y)
		    && double_is_valid_nonnegative(d.radius)
		    && d.color >= 0 && d.color < 21
		    && line_is_valid(d.line);
	}
	case xornsch_obtype_box: {
		xornsch_box const &d = *(xornsch_box const *)data;
		return double_is_valid(d.pos.x)
		    && double_is_valid(d.pos.y)
		    && double_is_valid(d.size.x)
		    && double_is_valid(d.size.y)
		    && d.color >= 0 && d.color < 21
		    && line_is_valid(d.line)
		    && fill_is_valid(d.fill);
	}
	case xornsch_obtype_circle: {
		xornsch_circle const &d = *(xornsch_circle const *)data;
		return double_is_valid(d.pos.x)
		    && double_is_valid(d.pos.y)
		    && double_is_valid_nonnegative(d.radius)
		    && d.color >= 0 && d.color < 21
		    && line_is_valid(d.line)
		    && fill_is_valid(d.fill);
	}
	case xornsch_obtype_component: {
		xornsch_component const &d = *(xornsch_component const *)data;
		return double_is_valid(d.pos.x)
		    && double_is_valid(d.pos.y)
		    && bool_is_valid(d.selectable)
		    && angle_is_valid(d.angle)
		    && bool_is_valid(d.mirror);
	}
	case xornsch_obtype_line: {
		xornsch_line const &d = *(xornsch_line const *)data;
		return double_is_valid(d.pos.x)
		    && double_is_valid(d.pos.y)
		    && double_is_valid(d.size.x)
		    && double_is_valid(d.size.y)
		    && d.color >= 0 && d.color < 21
		    && line_is_valid(d.line);
	}
	case xornsch_obtype_net: {
		xornsch_net const &d = *(xornsch_net const *)data;
		return double_is_valid(d.pos.x)
		    && double_is_valid(d.pos.y)
		    && double_is_valid(d.size.x)
		    && double_is_valid(d.size.y)
		    && d.color >= 0 && d.color < 21
		    && bool_is_valid(d.is_bus)
		    && bool_is_valid(d.is_pin)
		    && bool_is_valid(d.is_inverted)
		    && (d.is_pin || !d.is_inverted);
	}
	case xornsch_obtype_path: {
		xornsch_path const &d = *(xornsch_path const *)data;
		return string_is_valid(d.pathdata)
		    && d.color >= 0 && d.color < 21
		    && line_is_valid(d.line)
		    && fill_is_valid(d.fill);
	}
	case xornsch_obtype_picture: {
		xornsch_picture const &d = *(xornsch_picture const *)data;
		return double_is_valid(d.pos.x)
		    && double_is_valid(d.pos.y)
		    && double_is_valid(d.size.x)
		    && double_is_valid(d.size.y)
		    && angle_is_valid(d.angle)
		    && bool_is_valid(d.mirror);
	}
	case xornsch_obtype_text: {
		xornsch_text const &d = *(xornsch_text const *)data;
		return double_is_valid(d.pos.x)
		    && double_is_valid(d.pos.y)
		    && d.color >= 0 && d.color < 21
		    && d.text_size >= 0
		    && bool_is_valid(d.visibility)
		    && d.show_name_value >= 0 && d.show_name_value < 3
		    && angle_is_valid(d.angle)
		    && d.alignment >= 0 && d.alignment < 9
		    && string_is_valid(d.text);
	}
	default:
		return false;
	}
}
