# Copyright (C) 2013-2020 Roland Lutz
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software Foundation,
# Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.

import xorn.storage

def get_line(rev, ob):
    line = rev.get_object_data(ob).line
    return line.width, line.cap_style, line.dash_style, \
                       line.dash_length, line.dash_space

def get_fill(rev, ob):
    fill = rev.get_object_data(ob).fill
    return fill.type, fill.width, fill.angle0, fill.pitch0, \
                                  fill.angle1, fill.pitch1

def check_line_attr(obtype):
    rev = xorn.storage.Revision()
    data = obtype()
    ob = rev.add_object(data)

    data.line.width = 70.
    data.line.cap_style = 1
    data.line.dash_length = 73.
    data.line.dash_space = 74.

    data.line.dash_style = 0
    rev.set_object_data(ob, data)
    assert get_line(rev, ob) == (70., 1, 0, 0., 0.)

    data.line.dash_style = 1
    rev.set_object_data(ob, data)
    assert get_line(rev, ob) == (70., 1, 1, 0., 74.)

    data.line.dash_style = 2
    rev.set_object_data(ob, data)
    assert get_line(rev, ob) == (70., 1, 2, 73., 74.)

    data.line.dash_style = 3
    rev.set_object_data(ob, data)
    assert get_line(rev, ob) == (70., 1, 3, 73., 74.)

    data.line.dash_style = 4
    rev.set_object_data(ob, data)
    assert get_line(rev, ob) == (70., 1, 4, 73., 74.)

def check_fill_attr(obtype):
    rev = xorn.storage.Revision()
    data = obtype()
    ob = rev.add_object(data)

    data.fill.width = 81.
    data.fill.angle0 = 82
    data.fill.pitch0 = 83.
    data.fill.angle1 = 84
    data.fill.pitch1 = 85.

    data.fill.type = 0
    rev.set_object_data(ob, data)
    assert get_fill(rev, ob) == (0, 0., 0, 0., 0, 0.)

    data.fill.type = 1
    rev.set_object_data(ob, data)
    assert get_fill(rev, ob) == (1, 0., 0, 0., 0, 0.)

    data.fill.type = 2
    rev.set_object_data(ob, data)
    assert get_fill(rev, ob) == (2, 81., 82, 83., 84, 85.)

    data.fill.type = 3
    rev.set_object_data(ob, data)
    assert get_fill(rev, ob) == (3, 81., 82, 83., 0, 0.)

    data.fill.type = 4
    rev.set_object_data(ob, data)
    assert get_fill(rev, ob) == (4, 0., 0, 0., 0, 0.)

# arc
check_line_attr(xorn.storage.Arc)

# box
check_line_attr(xorn.storage.Box)
check_fill_attr(xorn.storage.Box)

# circle
check_line_attr(xorn.storage.Circle)
check_fill_attr(xorn.storage.Circle)

# component
pass

# line
check_line_attr(xorn.storage.Line)

# net
pass

# path
check_line_attr(xorn.storage.Path)
check_fill_attr(xorn.storage.Path)

# picture
pass

# text
pass
