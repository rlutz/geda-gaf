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

# Normally, Python sources should be installed to `pkgpythondir'.
# However, the Python interpreter only looks for submodules in the
# package's directory, so for platforms which use a separate
# exec_prefix, the compiled extension modules wouldn't be found.
# Instead of doing some magic with the package's load path, just do
# the easy thing and install everything to `pkgpyexecdir'.

gafpyexecdir = $(pyexecdir)/gaf

nobase_gafpyexec_PYTHON = \
	__init__.py \
	attrib.py \
	clib.py \
	fileformat.py \
	plainformat.py \
	plainread.py \
	plainwrite.py \
	read.py \
	ref.py \
	write.py \
	xmlformat.py \
	xmlread.py \
	xmlwrite.py \
	netlist/__init__.py \
	netlist/backend.py \
	netlist/blueprint.py \
	netlist/conn.py \
	netlist/guile.py \
	netlist/instance.py \
	netlist/net.py \
	netlist/netlist.py \
	netlist/package.py \
	netlist/pp_graphical.py \
	netlist/pp_hierarchy.py \
	netlist/pp_netattrib.py \
	netlist/pp_power.py \
	netlist/pp_slotting.py \
	netlist/reportgui.py \
	netlist/slib.py
