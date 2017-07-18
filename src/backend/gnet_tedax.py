# gnet_tedax.py - tEDAx gnetlist backend
# Copyright (C) 2017 Roland Lutz
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

# Since the tEDAx format creates entities implicitly when they are
# referenced, packages without a footprint, a value, and connections
# as well as nets without connections are dropped from the netlist.
# This shouldn't be a problem since these aren't interesting for the
# PCB flow anyway.
#
# tEDAx nomenclature:
#   net     -> "network"
#   package -> "component" (this is NOT a gEDA/gaf component)
#   pin     -> "pin"
#
# For a documentation of the tEDAx format, see:
#   http://repo.hu/projects/tedax/syntax.html

def escape(s):
    return s.replace('\\', '\\\\') \
            .replace('\n', '\\n') \
            .replace('\r', '\\r') \
            .replace('\t', '\\t') \
            .replace(' ', '\ ')

def run(f, netlist):
    def w(*fields):
        line = ' '.join(escape(field) for field in fields)
        if len(line) >= 512:
            sys.stderr.write("ERROR: output format limits lines "
                             "to 511 characters (%d needed)\n" % len(line))
            sys.exit(3)
        f.write(line + '\n')

    w('tEDAx', 'v1')

    # The 4th parameter is the netlist name.  gEDA/gaf doesn't have a
    # concept of this, so just use 'netlist' here.  You might want to
    # change this to e.g. the filename of the first toplevel sheet.
    w('begin', 'netlist', 'v1', 'netlist')

    for package in netlist.packages:
        footprint = package.get_attribute('footprint', None)
        if footprint is not None:
            w('footprint', package.refdes, footprint)

        value = package.get_attribute('value', None)
        if value is not None:
            # Since gEDA/gaf doesn't have a notion of a unit, don't
            # specify the optional 4th "unit" field.
            w('value', package.refdes, value)

        device = package.get_attribute('device', None)
        if device is not None:
            w('device', package.refdes, device)

        # TODO: spiceval/spicedev lines?

        # Add any attributes here which you want to preserve:
        for attr_name in ['footprints']:
            attr_value = package.get_attribute(attr_name, None)
            if attr_value is not None:
                w('comptag', package.refdes, attr_name, attr_value)

        for pin in package.pins:
            pinlabel = pin.get_attribute('pinlabel', None)
            if pinlabel is not None:
                w('pinname', package.refdes, pin.number, pinlabel)

            # tEDAx has two fields "pinslot" and "pinidx" which
            # enumerate the pins within a slot for use with SPICE.
            # There is no concept of this in gEDA/gaf; the way
            # gEDA/gaf slots and pins are mapped to SPICE devices
            # depends on the backend used.  We're going here with the
            # simplest approach, used in the "spice_noqsi" backend: a
            # gEDA/gaf package is mapped one-to-one to a SPICE device,
            # the SPICE pin number given by the pinseq= attribute.

            value = pin.get_attribute('pinseq', None)
            if value is not None:
                w('pinidx', package.refdes, pin.number, value)

            # TODO: spicedev field?

    for net in netlist.nets:
        for pin in net.connections:
            w('conn', net.name, pin.package.refdes, pin.number)

    w('end', 'netlist')
