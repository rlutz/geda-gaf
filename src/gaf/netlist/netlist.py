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

## \namespace gaf.netlist.netlist
## Main entry point for netlist generation.
#
# See the class Netlist for details.

import os, sys
from gettext import gettext as _
import gaf.attrib
import gaf.read
import gaf.netlist.blueprint
import gaf.netlist.instance
import gaf.netlist.net
import gaf.netlist.package
import gaf.netlist.pp_graphical
import gaf.netlist.pp_hierarchy
import gaf.netlist.pp_netattrib
import gaf.netlist.pp_power
import gaf.netlist.pp_slotting
import gaf.netlist.slib

## Global netlist object representing the result of a netlister run.

class Netlist:
    ## Create a netlist.
    #
    # This is the main function which creates a netlist.  The most
    # important argument is \a toplevel_filenames; it contains the
    # filenames of the schematic pages which should be traversed.
    # Other schematic pages are loaded as necessary if the \a
    # traverse_hierarchy argument is set.
    #
    # \param [in] toplevel_filenames
    #     list of filenames for the toplevel schematics, as given on
    #     the command line
    #
    # \param [in] traverse_hierarchy
    #     whether to descend into sub-schematics
    #
    # \param [in] verbose_mode
    #     whether to print "Loading schematic" messages
    #
    # \param [in] prefer_netname_attribute
    #     whether to prefer net names set via a net segment's \c
    #     netname= attribute over net names set via a pin's \c net=
    #     attribute
    #
    # \param [in] flat_package_namespace
    #     whether to use a common package namespace for all subsheets
    #
    # \param [in] flat_netname_namespace
    #     whether to use a common \c netname= namespace for all subsheets
    #
    # \param [in] flat_netattrib_namespace
    #     whether to use a common \c net= namespace for all subsheets
    #
    # \param [in] refdes_mangle_func
    #     function for mangling package/component refdes's
    #
    # \param [in] netname_mangle_func
    #     function for mangling net names
    #
    # \param [in] default_net_name
    #     naming template for unnamed nets
    #
    # \param [in] default_bus_name
    #     naming template for unnamed buses

    def __init__(self, toplevel_filenames,
                       traverse_hierarchy,
                       verbose_mode = False,
                       prefer_netname_attribute = False,
                       flat_package_namespace = False,
                       flat_netname_namespace = False,
                       flat_netattrib_namespace = False,
                       refdes_mangle_func = NotImplemented,
                       netname_mangle_func = NotImplemented,
                       default_net_name = 'unnamed_net',
                       default_bus_name = 'unnamed_bus',
                       show_error_coordinates = False):
        ## Aggregated list of all components in the netlist.
        self.components = []
        ## List of sheets for the schematics named on the command line.
        self.toplevel_sheets = []
        ## List of sheets.
        self.sheets = []

        ## Whether an error has occurred.
        self.failed = False

        ## List of nets.
        #
        # Populated by gaf.netlist.net.
        self.nets = None

        ## List of packages.
        #
        # Populated by gaf.netlist.package.
        self.packages = None

        ## Convenience dictionary for looking up packages by their refdes.
        self.packages_by_refdes = None

        ## Convenience dictionary for looking up nets by their name.
        self.nets_by_name = None

        ## List of schematic blueprints.
        self.schematics = []
        ## Dictionary mapping filenames to schematic blueprints.
        self.schematics_by_filename = {}

        ## Whether a common package namespace for all subsheets was used.
        self.flat_package_namespace = flat_package_namespace
        ## Function which was used for mangling package/component refdes's.
        self.refdes_mangle_func = refdes_mangle_func

        ## Whether to print coordinate hints for errors.
        self.show_error_coordinates = show_error_coordinates

        def load_schematic(filename):
            if filename in self.schematics_by_filename:
                return

            self.schematics_by_filename[filename] = None

            if verbose_mode:
                sys.stderr.write(_("Loading schematic [%s]\n") % filename)

            try:
                rev = gaf.read.read(filename, load_symbols = True)
            except Exception as e:
                if str(e):
                    sys.stderr.write(_("ERROR: Failed to load '%s': %s\n")
                                         % (filename, e))
                else:
                    sys.stderr.write(_("ERROR: Failed to load '%s'\n")
                                         % filename)
                sys.exit(2)

            rev.finalize()
            schematic = gaf.netlist.blueprint.Schematic(
                rev, filename, self)
            self.schematics.append(schematic)
            self.schematics_by_filename[filename] = schematic

            # Check if the component object represents a subsheet (i.e.,
            # has a "source=" attribute), and if so, get the filenames.

            for component in schematic.components:
                component.composite_sources = []

                for value in component.get_attributes('source'):
                    for filename in value.split(','):
                        if filename.startswith(' '):
                            component.warn(
                                _("leading spaces in source names "
                                  "are deprecated"))
                            filename = filename.lstrip(' ')

                        full_filename = \
                            gaf.netlist.slib.s_slib_search_single(
                                filename)
                        if full_filename is None:
                            component.error(
                                _("failed to load subcircuit '%s': "
                                  "schematic not found in source library")
                                % filename)
                            continue

                        load_schematic(full_filename)
                        component.composite_sources.append(
                            self.schematics_by_filename[full_filename])

        for filename in toplevel_filenames:
            load_schematic(filename)

        gaf.netlist.pp_power.postproc_blueprints(self)
        gaf.netlist.pp_hierarchy.postproc_blueprints(self)
        gaf.netlist.pp_slotting.postproc_blueprints(self)
        gaf.netlist.pp_netattrib.postproc_blueprints(self)
        gaf.netlist.pp_graphical.postproc_blueprints(self)
        gaf.netlist.package.postproc_blueprints(self)

        # look for component type conflicts
        for schematic in self.schematics:
            for component in schematic.components:
                if component.composite_sources and component.is_graphical:
                    # Do not bother traversing the hierarchy if the symbol
                    # has an graphical attribute attached to it.
                    component.warn(_("source= is set for graphical component"))
                    component.composite_sources = []

                if component.has_netname_attrib and \
                   component.has_portname_attrib:
                    component.error(_("netname= and portname= attributes "
                                      "are mutually exclusive"))

                if component.has_netname_attrib and \
                   component.composite_sources:
                    component.error(_("power symbol can't be a subschematic"))
                    component.composite_sources = []
                if component.has_portname_attrib and \
                   component.composite_sources:
                    component.error(_("I/O symbol can't be a subschematic"))
                    component.composite_sources = []

                if component.has_netname_attrib and component.is_graphical:
                    component.error(_("power symbol can't be graphical"))
                if component.has_portname_attrib and component.is_graphical:
                    component.error(_("I/O symbol can't be graphical"))

        # collect parameters
        for schematic in self.schematics:
            for component in schematic.components:
                component.parameters = {}

                for func in [gaf.attrib.search_inherited,
                             gaf.attrib.search_attached]:
                    names = set()
                    for val in func(component.ob, 'param'):
                        try:
                            name, value = gaf.attrib.parse_string(val)
                        except gaf.attrib.MalformedAttributeError:
                            component.error(
                                _("malformed param= attribute: %s") % val)
                            continue

                        if name in names:
                            component.error(
                                _("duplicate param= attribute: %s") % name)
                            continue

                        component.parameters[name] = value
                        names.add(name)

        # Traverse the schematic files and create the component objects
        # accordingly.

        def s_traverse_sheet1(sheet):
            for component in sheet.components:
                # now you need to traverse any underlying schematics

                # Check if the component object represents a subsheet (i.e.,
                # has a "source=" attribute), and if so, traverse that sheet.

                for subschematic in component.blueprint.composite_sources:
                    # can't do the following, don't know why... HACK TODO
                    #component.hierarchy_tag = u_basic_strdup(refdes)
                    subsheet = gaf.netlist.instance.Sheet(
                        component.sheet.netlist, subschematic, component)
                    s_traverse_sheet1(subsheet)

        for filename in toplevel_filenames:
            sheet = gaf.netlist.instance.Sheet(
                self, self.schematics_by_filename[filename], None)
            self.toplevel_sheets.append(sheet)
            if traverse_hierarchy:
                s_traverse_sheet1(sheet)

        # now that all the sheets have been read, go through and do the
        # post processing work

        # List the components in the same order as the old gnetlist code.

        def collect_components(sheet):
            for component in sheet.components:
                sheet.netlist.components.append(component)
                for subsheet in component.subsheets:
                    collect_components(subsheet)

        for sheet in self.toplevel_sheets:
            collect_components(sheet)

        # create net objects
        gaf.netlist.net.postproc_instances(
            self, { False: flat_netname_namespace,
                    True: flat_netattrib_namespace },
                  prefer_netname_attribute,
                  default_net_name, default_bus_name)

        # assign net names
        for net in self.nets:
            net.name = netname_mangle_func(
                net.unmangled_name, net.namespace)

        # assign component pins
        for component in self.components:
            for cpin in component.cpins:
                if (component.sheet, cpin.blueprint.net) \
                       not in cpin.local_net.net.sheets_and_net_blueprints:
                    cpin.local_net.net.sheets_and_net_blueprints.append(
                        (component.sheet, cpin.blueprint.net))

        for net in self.nets:
            for sheet, net_blueprint in net.sheets_and_net_blueprints:
                for cpin_blueprint in net_blueprint.pins:
                    if cpin_blueprint.ob is not None:
                        assert cpin_blueprint.ob.data().is_pin

                    cpin = sheet \
                        .components_by_blueprint[cpin_blueprint.component] \
                        .cpins_by_blueprint[cpin_blueprint]

                    assert cpin not in net.component_pins
                    net.component_pins.append(cpin)

        # Resolve hierarchy
        gaf.netlist.pp_hierarchy.postproc_instances(self)

        gaf.netlist.pp_graphical.postproc_instances(self)

        # group components into packages
        gaf.netlist.package.postproc_instances(
            self, flat_package_namespace)

        # see if any unconnected subsheet pins are connected to
        # multiple I/O ports; these need to be preserved or the
        # internal connections in the subsheet will be lost
        for net in self.nets:
            if net.is_unconnected_pin and len(net.connections) > 1:
                net.is_unconnected_pin = False

        # remove nets for unconnected pins
        self.nets = [net for net in self.nets if not net.is_unconnected_pin]


        # assign component refdes
        for component in self.components:
            if component.blueprint.refdes is not None:
                component.refdes = refdes_mangle_func(
                    component.blueprint.refdes,
                    None if flat_package_namespace
                         else component.sheet.namespace)

        # assign package refdes
        for package in self.packages:
            if package.namespace is not None:
                package.refdes = refdes_mangle_func(
                    package.unmangled_refdes, package.namespace)
            else:
                # If refdes mangling is disabled, packages don't have
                # a sheet attribute, so just use the unmangled refdes.
                package.refdes = package.unmangled_refdes

        # compile convenience hashes, checking for cross-page name clashes
        self.packages_by_refdes = {}
        for package in self.packages:
            if package.refdes in self.packages_by_refdes:
                other_package = self.packages_by_refdes[package.refdes]
                self.error(_("refdes conflict across hierarchy: "
                             "refdes `%s' is used by package `%s' on page "
                             "`%s' and by package `%s' on page `%s'") % (
                    package.refdes,
                    other_package.unmangled_refdes,
                    refdes_mangle_func('', other_package.namespace),
                    package.unmangled_refdes,
                    refdes_mangle_func('', package.namespace)))
            self.packages_by_refdes[package.refdes] = package

        self.nets_by_name = {}
        for net in self.nets:
            if net.name in self.nets_by_name:
                other_net = self.nets_by_name[net.name]
                self.error(_("net name conflict across hierarchy: "
                             "net name `%s' is used by net `%s' on page "
                             "`%s' and by net `%s' on page `%s'") % (
                    net.name,
                    other_net.unmangled_name,
                    netname_mangle_func('', other_net.namespace),
                    net.unmangled_name,
                    netname_mangle_func('', net.namespace)))
            self.nets_by_name[net.name] = net

    ## Return the value of a toplevel attribute.
    #
    # Searches for an floating attribute with the name \a name in the
    # schematic files listed on the command line.  Calls \ref error if
    # multiple attributes with different values are found.
    #
    # Traditionally, this function returned <tt>'not found'</tt> when
    # no such attribute existed in the toplevel schematic.
    #
    # \throws ValueError if no matching attribute was found and no \a
    #                    default was given

    def get_toplevel_attribute(self, name, default = KeyError):
        if not isinstance(name, basestring):
            raise ValueError

        values = []
        for sheet in self.toplevel_sheets:
            values += gaf.attrib.search_floating(
                sheet.blueprint.rev, name)

        if values:
            for value in values[1:]:
                if value != values[0]:
                    self.error(
                        _("inconsistent values for toplevel attribute "
                          "\"%s\": %s") % (
                              name, _(" vs. ").join(_("\"%s\"") % value
                                                    for value in values)))
                    return values[0]
            return values[0]

        if default is not KeyError:
            return default
        raise KeyError

    ## Print an error message and mark the netlist as failed.

    def error(self, msg):
        sys.stderr.write(_("error: %s\n" % msg))
        self.failed = True

    ## Print a warning message.

    def warn(self, msg):
        sys.stderr.write(_("warning: %s\n" % msg))
