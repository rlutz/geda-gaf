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

## \namespace gaf.netlist.pp_hierarchy
## Post-processing: Hierarchy traversal.

from gettext import gettext as _
import gaf.attrib

def postproc_blueprints(netlist):
    for schematic in netlist.schematics:
        schematic.ports = {}

        for component in schematic.components:
            portname = component.get_attribute('portname', None)
            if portname is None:
                continue

            if component.refdes is not None:
                component.error(_("refdes= and portname= attributes "
                                  "are mutually exclusive"))
            if gaf.attrib.search_all(component.ob, 'net'):
                component.error(_("portname= and net= attributes "
                                  "are mutually exclusive"))

            if not component.pins:
                component.error(_("I/O symbol doesn't have pins"))
            if len(component.pins) > 1:
                component.error(_("multiple pins on I/O symbol"))
            for pin in component.pins:
                if pin.number is not None or pin.ob.attached_objects():
                    pin.warn(_("pin attributes on I/O symbol are ignored"))

            try:
                ports = schematic.ports[portname]
            except KeyError:
                ports = schematic.ports[portname] = []
            ports.append(component)

            component.has_portname_attrib = True

## Connect subsheet I/O ports to the instantiating component's pins.
#
# Disconnect all connections from and to composite components as
# they have been replaced with the actual subschematics.
#
# remove all composite components and ports

def postproc_instances(netlist):
    remove_components = set()

    for component in netlist.components:
        if not component.blueprint.composite_sources:
            continue

        # collect potential old-style ports
        refdes_dict = {}
        for subsheet in component.subsheets:
            for potential_port in subsheet.components:
                try:
                    l = refdes_dict[potential_port.blueprint.refdes]
                except KeyError:
                    l = refdes_dict[potential_port.blueprint.refdes] = []
                l.append(potential_port)

        processed_labels = set()
        for cpin in component.cpins:
            dest_net = cpin.local_net.net
            cpin.local_net.cpins.remove(cpin)
            dest_net.component_pins.remove(cpin)

            label = cpin.blueprint.get_attribute('pinlabel', None)
            if label is None:
                cpin.error(_("pin on composite component is missing a label"))
                continue
            if label in processed_labels:
                cpin.error(_("duplicate pin for port `%s' "
                             "on composite component") % label)
                continue
            processed_labels.add(label)

            # search for the matching port
            ports = [subsheet.components_by_blueprint[port]
                     for subsheet in component.subsheets
                     for port in subsheet.blueprint.ports.get(label, [])]

            for port in refdes_dict.get(label, []):
                # found an old-style port
                if port.blueprint.has_netname_attrib:
                    port.error(_("netname= attribute can't be used "
                                 "on an I/O symbol"))
                if gaf.attrib.search_all(port.blueprint.ob, 'net'):
                    port.error(_("net= attribute can't be used "
                                 "on an I/O symbol"))
                if port.blueprint.composite_sources:
                    port.error(_("I/O symbol can't be a subschematic"))
                if port.blueprint.is_graphical:
                    port.error(_("I/O symbol can't be graphical"))

                if not port.cpins:
                    port.error(_("I/O symbol doesn't have pins"))
                    continue
                if len(port.cpins) > 1:
                    port.error(_("multiple pins on I/O symbol"))
                    continue

                ports.append(port)

            if not ports:
                cpin.warn(_("missing I/O symbol for port `%s' "
                            "inside schematic") % label)
            elif len(ports) > 1:
                cpin.warn(_("multiple I/O symbols for port `%s' "
                            "inside schematic") % label)

            for port in ports:
                src_net = port.cpins[0].local_net.net

                # merge nets
                if src_net != dest_net:
                    src_net.merge_into(dest_net)
                    dest_net.component_pins += src_net.component_pins
                    del src_net.component_pins[:]

                # remove port component
                remove_components.add(port)
                port.sheet.components.remove(port)
                del port.sheet.components_by_blueprint[port.blueprint]

                port.cpins[0].local_net.cpins.remove(port.cpins[0])
                dest_net.component_pins.remove(port.cpins[0])

        # After all pins have been connected, remove the component.
        remove_components.add(component)
        component.sheet.components.remove(component)
        del component.sheet.components_by_blueprint[component.blueprint]

    netlist.components = [component for component in netlist.components
                          if component not in remove_components]

    for component in netlist.components:
        if component.blueprint.has_portname_attrib:
            component.error(_("unmatched I/O symbol"))
