# gaf.netlist - gEDA Netlist Extraction and Generation
# Copyright (C) 1998-2010 Ales Hvezda
# Copyright (C) 1998-2010 gEDA Contributors (see ChangeLog for details)
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

## \namespace gaf.netlist.pp_power
## Post-processing: New-style power symbols.

from gettext import gettext as _
import gaf.attrib

def postproc_blueprints(netlist):
    for schematic in netlist.schematics:
        for component in schematic.components:
            netname = component.get_attribute('netname', None)
            if netname is None:
                continue

            if component.refdes is not None:
                component.error(_("refdes= and netname= attributes "
                                  "are mutually exclusive"))
            if gaf.attrib.search_all(component.ob, 'net'):
                component.error(_("netname= and net= attributes "
                                  "are mutually exclusive"))

            if not component.pins:
                component.error(_("power symbol doesn't have pins"))
            if len(component.pins) > 1:
                component.error(_("multiple pins on power symbol"))

            for pin in component.pins:
                if pin.number is not None or pin.ob.attached_objects():
                    pin.warn(_("pin attributes on power symbol are ignored"))
                pin.net.names_from_net_attribute.append(netname)

            component.has_netname_attrib = True
