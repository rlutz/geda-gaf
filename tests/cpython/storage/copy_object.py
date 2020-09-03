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

import xorn.storage, Setup

def throws(fun, *args, **kwds):
    try:
        fun(*args, **kwds)
    except Exception as e:
        return type(e)

rev0, rev1, rev2, rev3, ob0, ob1a, ob1b = Setup.setup()


# can't copy object which doesn't exist in source revision

rev4 = xorn.storage.Revision(rev3)
assert rev4 is not None

assert throws(rev4.copy_object, rev0, ob0) == KeyError

rev4.finalize()

objects = rev4.get_objects()
assert type(objects) == list
assert len(objects) == 2
assert objects == [ob0, ob1b]


# can copy object otherwise

rev4 = xorn.storage.Revision(rev3)
assert rev4 is not None

ob0copy = rev4.copy_object(rev1, ob0)
assert ob0copy is not None

rev4.finalize()

assert type(rev4.get_object_data(ob0copy)) != type(rev4.get_object_data(ob0))
assert type(rev4.get_object_data(ob0copy)) == type(rev1.get_object_data(ob0))

objects = rev4.get_objects()
assert type(objects) == list
assert len(objects) == 3
assert objects == [ob0, ob1b, ob0copy]
