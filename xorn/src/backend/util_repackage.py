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

## \file util_repackage.py
## Re-grouping components into packages using a custom refdes function.

import sys
from gettext import gettext as _
import gaf.netlist.package

class Package(gaf.netlist.package.Package):
    def __init__(self, *args, **kwds):
        gaf.netlist.package.Package.__init__(self, *args, **kwds)

    def error(self, msg):
        sys.stderr.write(_("(re-packaged) package `%s': error: %s\n") % (
                             self.refdes, msg))
        self.netlist.failed = True

    def warn(self, msg):
        sys.stderr.write(_("(re-packaged) package `%s': warning: %s\n") % (
                             self.refdes, msg))

class PackagePin(gaf.netlist.package.PackagePin):
    def __init__(self, *args, **kwds):
        gaf.netlist.package.PackagePin.__init__(self, *args, **kwds)

    def error(self, msg):
        sys.stderr.write(
            _("(re-packaged) package `%s', pin `%s': error: %s\n") % (
                self.package.refdes, self.number, msg))
        self.package.netlist.failed = True

    def warn(self, msg):
        sys.stderr.write(
            _("(re-packaged) package `%s', pin `%s': warning: %s\n") % (
                self.package.refdes, self.number, msg))

def blueprint_requires_refdes(component):
    if component.is_graphical or component.has_netname_attrib \
                              or component.has_portname_attrib:
        return False

    for pin in component.pins:
        if pin.has_netattrib:
            return False

    return True

## Re-group components into packages using a custom refdes function.
#
# With the new netlisting code, netlist generation is completely
# independent from the backend invocation.  Therefore, it is no longer
# possible for a backend to override the way \c gnetlist determines
# the refdes of a component in order to achieve a different grouping
# of components into packages.
#
# This function allows a backend to repeat the grouping stage using a
# custom refdes function.  It doesn't change the actual netlist but
# returns a list of "alternative" package objects which can be used
# instead of \c netlist.packages.
#
# \a refdes_func should be a callback function which returns the
# desired refdes for a given component instance.  In the simplest
# case, it would do some kind of transformation on \c
# component.blueprint.refdes.

def repackage(netlist, refdes_func):
    new_packages = []
    pkg_dict = {}

    for component in netlist.components:
        new_refdes = refdes_func(component)

        if new_refdes is None:
            if blueprint_requires_refdes(component.blueprint):
                component.warning(_("component dropped during re-packaging"))
            continue

        if netlist.flat_package_namespace:
            namespace = None
        else:
            namespace = component.sheet.namespace

        try:
            package = pkg_dict[namespace, new_refdes]
        except KeyError:
            package = Package(netlist, namespace, new_refdes)
            new_packages.append(package)
            pkg_dict[namespace, new_refdes] = package

        package.components.append(component)

        for cpin in component.cpins:
            try:
                ppin = package.pins_by_number[cpin.blueprint.number]
            except KeyError:
                ppin = PackagePin(package, cpin.blueprint.number)
                package.pins.append(ppin)
                package.pins_by_number[cpin.blueprint.number] = ppin
            ppin.cpins.append(cpin)

    for package in new_packages:
        for ppin in package.pins:
            nets = []
            for cpin in ppin.cpins:
                if cpin.local_net.net not in nets:
                    nets.append(cpin.local_net.net)
            assert nets
            if len(nets) > 1:
                ppin.error(_("multiple nets connected to pin "
                             "after re-packaging: %s")
                           % _(" vs. ").join(_("\"%s\"") % net.name
                                             for net in nets))
            ppin.net = nets[0]

    for package in new_packages:
        if package.namespace is not None:
            package.refdes = netlist.refdes_mangle_func(
                package.unmangled_refdes, package.namespace)
        else:
            # If refdes mangling is disabled, packages don't have
            # a sheet attribute, so just use the unmangled refdes.
            package.refdes = package.unmangled_refdes

    # compile convenience hash, checking for cross-page name clashes
    packages_by_refdes = {}
    for package in new_packages:
        if package.refdes in packages_by_refdes:
            other_package = packages_by_refdes[package.refdes]
            self.error(_("refdes conflict across hierarchy after re-packaging: "
                         "refdes `%s' is used by package `%s' on page "
                         "`%s' and by package `%s' on page `%s'") % (
                package.refdes,
                other_package.unmangled_refdes,
                netlist.refdes_mangle_func('', other_package.namespace),
                package.unmangled_refdes,
                netlist.refdes_mangle_func('', package.namespace)))
        packages_by_refdes[package.refdes] = package

    return new_packages
