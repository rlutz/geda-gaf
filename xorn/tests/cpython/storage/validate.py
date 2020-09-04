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

INFINITY = float('inf')

def throws(fun, *args, **kwds):
    try:
        fun(*args, **kwds)
    except Exception as e:
        return type(e)

def equals(a, b):
    # data comparison isn't implemented yet,
    # so we need to compare the data manually
    if type(a) != type(b):
        return False
    af = [field for field in dir(a) if not field.startswith('_')]
    bf = [field for field in dir(a) if not field.startswith('_')]
    if af != bf:
        return False
    for field in af:
        if field == 'line':
            if not equals(a.line, b.line):
                return False
        elif field == 'fill':
            if not equals(a.fill, b.fill):
                return False
        else:
            try:
                if getattr(a, field) != getattr(b, field):
                    return False
            except AttributeError:
                if throws(getattr, a, field) != throws(getattr, b, field):
                    return False
    return True

def test(data, expected_status):
    rev = xorn.storage.Revision()

    if expected_status == 1:
        assert throws(rev.add_object, data) == ValueError
    else:
        ob = rev.add_object(data)
        out = rev.get_object_data(ob)
        if expected_status == 0:
            assert equals(out, data)
        else:
            assert not equals(out, data)

    zero = type(data)()
    ob = rev.add_object(zero)
    out = rev.get_object_data(ob)
    assert equals(out, zero)

    if expected_status == 1:
        assert throws(rev.set_object_data, ob, data) == ValueError
    else:
        rev.set_object_data(ob, data)
        out = rev.get_object_data(ob)
        if expected_status == 0:
            assert equals(out, data)
        else:
            assert not equals(out, data)

def check_line_attr(obtype):
    data = obtype()
    data.line.width = -1.;              test(data, 1)
    data.line.width = 0.;               test(data, 0)
    data.line.width = 1.;               test(data, 0)
    data.line.width = INFINITY;         test(data, 1)

    data = obtype()
    data.line.cap_style = -1;           test(data, 1)
    data.line.cap_style = 0;            test(data, 0)
    data.line.cap_style = 2;            test(data, 0)
    data.line.cap_style = 3;            test(data, 1)

    data = obtype()
    data.line.dash_style = -1;          test(data, 1)
    data.line.dash_style = 0;           test(data, 0)
    data.line.dash_style = 4;           test(data, 0)
    data.line.dash_style = 5;           test(data, 1)

    data = obtype()
    data.line.dash_length = -1.;        test(data, 1)
    data.line.dash_length = 0.;         test(data, 0)
    data.line.dash_length = 1.;         test(data, 2)
    data.line.dash_length = INFINITY;   test(data, 1)

    data = obtype()
    data.line.dash_space = -1.;         test(data, 1)
    data.line.dash_space = 0.;          test(data, 0)
    data.line.dash_space = 1.;          test(data, 2)
    data.line.dash_space = INFINITY;    test(data, 1)

def check_fill_attr(obtype):
    data = obtype()
    data.fill.type = -1;                test(data, 1)
    data.fill.type = 0;                 test(data, 0)
    data.fill.type = 4;                 test(data, 0)
    data.fill.type = 5;                 test(data, 1)

    data = obtype()
    data.fill.width = -1.;              test(data, 1)
    data.fill.width = 0.;               test(data, 0)
    data.fill.width = 1.;               test(data, 2)
    data.fill.width = INFINITY;         test(data, 1)

    data = obtype()
    data.fill.angle0 = -1;              test(data, 2)
    data.fill.angle0 = 0;               test(data, 0)
    data.fill.angle0 = 1;               test(data, 2)
    data.fill.angle0 = 360;             test(data, 2)

    data = obtype()
    data.fill.pitch0 = -1.;             test(data, 1)
    data.fill.pitch0 = 0.;              test(data, 0)
    data.fill.pitch0 = 1.;              test(data, 2)
    data.fill.pitch0 = INFINITY;        test(data, 1)

    data = obtype()
    data.fill.angle1 = -1;              test(data, 2)
    data.fill.angle1 = 0;               test(data, 0)
    data.fill.angle1 = 1;               test(data, 2)
    data.fill.angle1 = 360;             test(data, 2)

    data = obtype()
    data.fill.pitch1 = -1.;             test(data, 1)
    data.fill.pitch1 = 0.;              test(data, 0)
    data.fill.pitch1 = 1.;              test(data, 2)
    data.fill.pitch1 = INFINITY;        test(data, 1)

def check_arc():
    data = xorn.storage.Arc()
    data.x = -1.;                       test(data, 0)
    data.x = 0.;                        test(data, 0)
    data.x = 1.;                        test(data, 0)
    data.x = INFINITY;                  test(data, 1)

    data = xorn.storage.Arc()
    data.y = -1.;                       test(data, 0)
    data.y = 0.;                        test(data, 0)
    data.y = 1.;                        test(data, 0)
    data.y = INFINITY;                  test(data, 1)

    data = xorn.storage.Arc()
    data.radius = -1.;                  test(data, 1)
    data.radius = 0.;                   test(data, 0)
    data.radius = 1.;                   test(data, 0)
    data.radius = INFINITY;             test(data, 1)

    data = xorn.storage.Arc()
    data.startangle = -1;               test(data, 0)
    data.startangle = 0;                test(data, 0)
    data.startangle = 1;                test(data, 0)
    data.startangle = 360;              test(data, 0)

    data = xorn.storage.Arc()
    data.sweepangle = -1;               test(data, 0)
    data.sweepangle = 0;                test(data, 0)
    data.sweepangle = 1;                test(data, 0)
    data.sweepangle = 360;              test(data, 0)

    data = xorn.storage.Arc()
    data.color = -1;                    test(data, 1)
    data.color = 0;                     test(data, 0)
    data.color = 20;                    test(data, 0)
    data.color = 21;                    test(data, 1)

    check_line_attr(xorn.storage.Arc)

def check_box():
    data = xorn.storage.Box()
    data.x = -1.;                       test(data, 0)
    data.x = 0.;                        test(data, 0)
    data.x = 1.;                        test(data, 0)
    data.x = INFINITY;                  test(data, 1)

    data = xorn.storage.Box()
    data.y = -1.;                       test(data, 0)
    data.y = 0.;                        test(data, 0)
    data.y = 1.;                        test(data, 0)
    data.y = INFINITY;                  test(data, 1)

    data = xorn.storage.Box()
    data.width = -1.;                   test(data, 0)
    data.width = 0.;                    test(data, 0)
    data.width = 1.;                    test(data, 0)
    data.width = INFINITY;              test(data, 1)

    data = xorn.storage.Box()
    data.height = -1.;                  test(data, 0)
    data.height = 0.;                   test(data, 0)
    data.height = 1.;                   test(data, 0)
    data.height = INFINITY;             test(data, 1)

    data = xorn.storage.Box()
    data.color = -1;                    test(data, 1)
    data.color = 0;                     test(data, 0)
    data.color = 20;                    test(data, 0)
    data.color = 21;                    test(data, 1)

    check_line_attr(xorn.storage.Box)
    check_fill_attr(xorn.storage.Box)

def check_circle():
    data = xorn.storage.Circle()
    data.x = -1.;                       test(data, 0)
    data.x = 0.;                        test(data, 0)
    data.x = 1.;                        test(data, 0)
    data.x = INFINITY;                  test(data, 1)

    data = xorn.storage.Circle()
    data.y = -1.;                       test(data, 0)
    data.y = 0.;                        test(data, 0)
    data.y = 1.;                        test(data, 0)
    data.y = INFINITY;                  test(data, 1)

    data = xorn.storage.Circle()
    data.radius = -1.;                  test(data, 1)
    data.radius = 0.;                   test(data, 0)
    data.radius = 1.;                   test(data, 0)
    data.radius = INFINITY;             test(data, 1)

    data = xorn.storage.Circle()
    data.color = -1;                    test(data, 1)
    data.color = 0;                     test(data, 0)
    data.color = 20;                    test(data, 0)
    data.color = 21;                    test(data, 1)

    check_line_attr(xorn.storage.Circle)
    check_fill_attr(xorn.storage.Circle)

def check_component():
    data = xorn.storage.Component()
    data.x = -1.;                       test(data, 0)
    data.x = 0.;                        test(data, 0)
    data.x = 1.;                        test(data, 0)
    data.x = INFINITY;                  test(data, 1)

    data = xorn.storage.Component()
    data.y = -1.;                       test(data, 0)
    data.y = 0.;                        test(data, 0)
    data.y = 1.;                        test(data, 0)
    data.y = INFINITY;                  test(data, 1)

    data = xorn.storage.Component()
    data.angle = -1;                    test(data, 1)
    data.angle = 0;                     test(data, 0)
    data.angle = 1;                     test(data, 1)
    data.angle = 270;                   test(data, 0)
    data.angle = 360;                   test(data, 1)

def check_line():
    data = xorn.storage.Line()
    data.x = -1.;                       test(data, 0)
    data.x = 0.;                        test(data, 0)
    data.x = 1.;                        test(data, 0)
    data.x = INFINITY;                  test(data, 1)

    data = xorn.storage.Line()
    data.y = -1.;                       test(data, 0)
    data.y = 0.;                        test(data, 0)
    data.y = 1.;                        test(data, 0)
    data.y = INFINITY;                  test(data, 1)

    data = xorn.storage.Line()
    data.width = -1.;                   test(data, 0)
    data.width = 0.;                    test(data, 0)
    data.width = 1.;                    test(data, 0)
    data.width = INFINITY;              test(data, 1)

    data = xorn.storage.Line()
    data.height = -1.;                  test(data, 0)
    data.height = 0.;                   test(data, 0)
    data.height = 1.;                   test(data, 0)
    data.height = INFINITY;             test(data, 1)

    data = xorn.storage.Line()
    data.color = -1;                    test(data, 1)
    data.color = 0;                     test(data, 0)
    data.color = 20;                    test(data, 0)
    data.color = 21;                    test(data, 1)

    check_line_attr(xorn.storage.Line)

def check_net():
    data = xorn.storage.Net()
    data.x = -1.;                       test(data, 0)
    data.x = 0.;                        test(data, 0)
    data.x = 1.;                        test(data, 0)
    data.x = INFINITY;                  test(data, 1)

    data = xorn.storage.Net()
    data.y = -1.;                       test(data, 0)
    data.y = 0.;                        test(data, 0)
    data.y = 1.;                        test(data, 0)
    data.y = INFINITY;                  test(data, 1)

    data = xorn.storage.Net()
    data.width = -1.;                   test(data, 0)
    data.width = 0.;                    test(data, 0)
    data.width = 1.;                    test(data, 0)
    data.width = INFINITY;              test(data, 1)

    data = xorn.storage.Net()
    data.height = -1.;                  test(data, 0)
    data.height = 0.;                   test(data, 0)
    data.height = 1.;                   test(data, 0)
    data.height = INFINITY;             test(data, 1)

    data = xorn.storage.Net()
    data.color = -1;                    test(data, 1)
    data.color = 0;                     test(data, 0)
    data.color = 20;                    test(data, 0)
    data.color = 21;                    test(data, 1)

    data = xorn.storage.Net()
    data.is_inverted = False;           test(data, 0)
    data.is_inverted = True;            test(data, 1)
    data.is_pin = True
    data.is_inverted = False;           test(data, 0)
    data.is_inverted = True;            test(data, 0)

def check_path():
    data = xorn.storage.Path()
    data.color = -1;                    test(data, 1)
    data.color = 0;                     test(data, 0)
    data.color = 20;                    test(data, 0)
    data.color = 21;                    test(data, 1)

    check_line_attr(xorn.storage.Path)
    check_fill_attr(xorn.storage.Path)

def check_picture():
    data = xorn.storage.Picture()
    data.x = -1.;                       test(data, 0)
    data.x = 0.;                        test(data, 0)
    data.x = 1.;                        test(data, 0)
    data.x = INFINITY;                  test(data, 1)

    data = xorn.storage.Picture()
    data.y = -1.;                       test(data, 0)
    data.y = 0.;                        test(data, 0)
    data.y = 1.;                        test(data, 0)
    data.y = INFINITY;                  test(data, 1)

    data = xorn.storage.Picture()
    data.width = -1.;                   test(data, 0)
    data.width = 0.;                    test(data, 0)
    data.width = 1.;                    test(data, 0)
    data.width = INFINITY;              test(data, 1)

    data = xorn.storage.Picture()
    data.height = -1.;                  test(data, 0)
    data.height = 0.;                   test(data, 0)
    data.height = 1.;                   test(data, 0)
    data.height = INFINITY;             test(data, 1)

    data = xorn.storage.Picture()
    data.angle = -1;                    test(data, 1)
    data.angle = 0;                     test(data, 0)
    data.angle = 1;                     test(data, 1)
    data.angle = 270;                   test(data, 0)
    data.angle = 360;                   test(data, 1)

def check_text():
    data = xorn.storage.Text()
    data.x = -1.;                       test(data, 0)
    data.x = 0.;                        test(data, 0)
    data.x = 1.;                        test(data, 0)
    data.x = INFINITY;                  test(data, 1)

    data = xorn.storage.Text()
    data.y = -1.;                       test(data, 0)
    data.y = 0.;                        test(data, 0)
    data.y = 1.;                        test(data, 0)
    data.y = INFINITY;                  test(data, 1)

    data = xorn.storage.Text()
    data.color = -1;                    test(data, 1)
    data.color = 0;                     test(data, 0)
    data.color = 20;                    test(data, 0)
    data.color = 21;                    test(data, 1)

    data = xorn.storage.Text()
    data.text_size = -1;                test(data, 1)
    data.text_size = 0;                 test(data, 0)
    data.text_size = 1;                 test(data, 0)
    data.text_size = 1000000;           test(data, 0)

    data = xorn.storage.Text()
    data.show_name_value = -1;          test(data, 1)
    data.show_name_value = 0;           test(data, 0)
    data.show_name_value = 1;           test(data, 0)
    data.show_name_value = 2;           test(data, 0)
    data.show_name_value = 3;           test(data, 1)

    data = xorn.storage.Text()
    data.angle = -1;                    test(data, 1)
    data.angle = 0;                     test(data, 0)
    data.angle = 1;                     test(data, 1)
    data.angle = 270;                   test(data, 0)
    data.angle = 360;                   test(data, 1)

    data = xorn.storage.Text()
    data.alignment = -1;                test(data, 1)
    data.alignment = 0;                 test(data, 0)
    data.alignment = 8;                 test(data, 0)
    data.alignment = 9;                 test(data, 1)

check_arc()
check_box()
check_circle()
check_component()
check_line()
check_net()
check_path()
check_picture()
check_text()
