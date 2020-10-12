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

import getopt, sys, os
from gettext import gettext as _
import xorn.command
import xorn.config
import xorn.fileutils
import xorn.proxy
import xorn.storage
import gaf.attrib
import gaf.read
import gaf.write

def main():
    extract_all = False
    list_embedded = False

    try:
        options, args = getopt.getopt(
            xorn.command.args, 'al', ['all', 'list', 'help', 'version'])
    except getopt.GetoptError as e:
        xorn.command.invalid_arguments(e.msg)

    for option, value in options:
        if option == '-a' or option == '--all':
            extract_all = True
        elif option == '-l' or option == '--list':
            list_embedded = True
        elif option == '--help':
            sys.stdout.write(_("""\
Usage: %s [OPTION]... SCHEMATIC [SYMBOL|PICTURE]...
Extract objects embedded in a schematic into a separate file

  -a, --all             extract all embedded objects
  -l, --list            list embedded object names
      --help            give this help
      --version         display version number

Report %s bugs to %s
""") % (xorn.command.program_name,
        xorn.config.PACKAGE_NAME,
        xorn.config.PACKAGE_BUGREPORT))
            sys.exit(0)
        elif option == '--version':
            xorn.command.core_version()

    if extract_all and list_embedded:
        xorn.command.invalid_arguments(_(
            "options `--all' and `--list' are mutually exclusive"))
    if extract_all or list_embedded:
        if len(args) < 1:
            xorn.command.invalid_arguments(_("missing argument"))
        if len(args) > 1:
            xorn.command.invalid_arguments(_("too many arguments"))
    elif len(args) < 2:
        xorn.command.invalid_arguments(_("not enough arguments"))

    try:
        rev = gaf.read.read(args[0])
    except gaf.fileformat.UnknownFormatError:
        sys.stderr.write(_("%s: %s: unrecognized file name extension\n")
                         % (xorn.command.program_short_name, args[0]))
        sys.exit(1)
    except IOError as e:
        sys.stderr.write(_("%s: can't read %s: %s\n")
                         % (xorn.command.program_short_name,
                            args[0], e.strerror))
        sys.exit(1)
    except UnicodeDecodeError as e:
        sys.stderr.write(_("%s: can't read %s: %s\n")
                         % (xorn.command.program_short_name, args[0], str(e)))
        sys.exit(1)
    except gaf.read.ParseError:
        sys.exit(1)

    embedded_symbols = {}
    embedded_pixmaps = {}

    for ob in rev.toplevel_objects():
        data = ob.data()
        if isinstance(data, xorn.storage.Component) and data.symbol.embedded \
                and not data.symbol.basename in embedded_symbols:
            embedded_symbols[data.symbol.basename.encode()] = ob
            if list_embedded:
                sys.stdout.write(data.symbol.basename + '\n')
        if isinstance(data, xorn.storage.Picture) and data.pixmap.embedded:
            filename = os.path.basename(data.pixmap.filename)
            if not filename in embedded_pixmaps:
                embedded_pixmaps[filename.encode()] = ob
            if list_embedded:
                sys.stdout.write(filename + '\n')

    if extract_all:
        filenames = list(set(embedded_symbols.keys() +
                             embedded_pixmaps.keys()))
        filenames.sort()
    else:
        filenames = args[1:]

    for filename in filenames:
        basename = os.path.basename(filename)
        if basename not in embedded_symbols \
                and basename not in embedded_pixmaps:
            sys.stderr.write(_("%s: can't extract '%s': "
                               "No such embedded symbol or pixmap\n")
                             % (xorn.command.program_short_name, basename))
            sys.exit(1)

    for filename in filenames:
        basename = os.path.basename(filename)
        if basename in embedded_symbols:
            ob = embedded_symbols[basename]
            if gaf.attrib.search_all(ob, 'slot'):
                sys.stderr.write(
                    _("Warning: Symbol \"%s\" is slotted; "
                      "pin numbers may have changed.\n") % basename)
            try:
                gaf.write.write(
                    xorn.proxy.RevisionProxy(ob.data().symbol.prim_objs),
                    filename)
            except (IOError, OSError) as e:
                sys.stderr.write(_("%s: can't write %s: %s\n") % (
                    xorn.command.program_short_name, filename, e.strerror))
                sys.exit(1)
        else:
            try:
                def write_func(f):
                    f.write(embedded_pixmaps[basename].data().pixmap.data)
                xorn.fileutils.write(filename, write_func)
            except (IOError, OSError) as e:
                sys.stderr.write(_("%s: can't write %s: %s\n") % (
                    xorn.command.program_short_name, filename, e.strerror))
                sys.exit(1)
