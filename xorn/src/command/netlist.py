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

import cStringIO, getopt, gettext, os, sys
from gettext import gettext as _

import xorn.command
import xorn.config
import xorn.fileutils
import gaf.clib
import gaf.netlist.backend
import gaf.netlist.netlist
import gaf.netlist.slib
import types
import sab

APPEND, PREPEND = xrange(2)

report_gui = None
report_gui_buf = None
report_gui_stderr = None

def parse_bool(value):
    if value in ['disabled', 'no', 'n', 'off', 'false', '0']:
        return False
    if value in ['enabled', 'yes', 'y', 'on', 'true', '1']:
        return True
    sys.stderr.write(_("%s: `%s' is not a valid boolean value\n")
                     % (xorn.command.program_short_name, value))
    sys.exit(1)

def parse_order(value):
    try:
        return { 'append': APPEND, 'prepend': PREPEND }[value]
    except KeyError:
        sys.stderr.write(_("%s: `%s' is not a valid mangling order\n")
                         % (xorn.command.program_short_name, value))
        sys.stderr.write(_("%s: Allowed values are: append prepend\n")
                         % (xorn.command.program_short_name))
        sys.exit(1)


## Add a directory to the symbol library.

def symbol_library(path, recursive, option_name):
    # invalid path?
    if not os.path.isdir(path):
        sys.stderr.write(_("%s: \"%s\" is not a directory (passed to %s)\n")
                         % (xorn.command.program_short_name, path,
                            option_name))
        sys.exit(1)

    gaf.clib.add_source(
        gaf.clib.DirectorySource(path, recursive),
        gaf.clib.uniquify_source_name(os.path.basename(path)))

## Add a command to the symbol library.
#
# \param [in] listcmd  command to get a list of symbols
# \param [in] getcmd   command to get a symbol from the library
# \param [in] name     optional descriptive name for symbol source

def symbol_library_command(value):
    tokens = value.split(':')
    if len(tokens) != 3:
        sys.stderr.write(_("Invalid library command argument: %s\n"
                           "Must be 'list' and 'get' commands and name "
                           "separated by colons\n") % value)
        sys.exit(1)
    listcmd, getcmd, name = tokens

    gaf.clib.add_source(gaf.clib.CommandSource(listcmd, getcmd),
                        gaf.clib.uniquify_source_name(name))

def source_library(path):
    # invalid path?
    if not os.path.isdir(path):
        sys.stderr.write(_("%s: \"%s\" is not a directory (passed to %s)\n")
                         % (xorn.command.program_short_name, path,
                            '--source-library'))
        sys.exit(1)

    gaf.netlist.slib.slib.append(path)

def source_library_search(path):
    # invalid path?
    if not os.path.isdir(path):
        sys.stderr.write(_("%s: \"%s\" is not a directory (passed to %s)\n")
                         % (xorn.command.program_short_name, path,
                            '--source-library-search'))
        sys.exit(1)

    for entry in os.listdir(path):
        # TODO: what if entry is str, not unicode?
        # don't do . and .. and special case font
        if entry != '.' and entry != '..' and entry.lower() != 'font':
            fullpath = os.path.join(path, entry)
            if not os.path.isdir(fullpath):
                continue
            if fullpath not in gaf.netlist.slib.slib:
                gaf.netlist.slib.slib.append(fullpath)


## Construct component and net names for hierarchical schematics.

def mangle(basename, component, separator0, order0, separator1, order1):
    if component is None:
        return basename
    if basename is None:
        return None

    # prefix which is attached to all component and net names
    # found in this schematic
    hierarchy_tag = []
    while component is not None:
        hierarchy_tag.insert(0, component.blueprint.refdes)
        component = component.sheet.instantiating_component
    assert hierarchy_tag

    if order0 == PREPEND:
        hierarchy_tag.reverse()

    if order1 == APPEND:
        return separator0.join(hierarchy_tag) + separator1 + basename
    elif order1 == PREPEND:
        return basename + separator1 + separator0.join(hierarchy_tag)


def usage():
    sys.stdout.write(_("Usage: %s [OPTION]... -g BACKEND [--] FILE...\n"
                       "       %s [OPTION]... -i [--] FILE...\n")
                     % ((xorn.command.program_name, ) * 2))
    sys.stdout.write(_("Generate a netlist from one or more "
                       "gEDA schematic files.\n"))
    sys.stdout.write("\n")
    sys.stdout.write(_("""\
General options:
  -o FILE               filename for netlist data output  [default: -]
  -g BACKEND            specify netlist backend to use
  -L DIR                add DIR to backend search path
  -O STRING             pass an option string to backend
  -c EXPR               evaluate Python expression at startup
  -i                    enter interactive Python interpreter after loading
  -v, --verbose         verbose mode (print loaded schematics)
"""))
    sys.stdout.write("\n")
    sys.stdout.write(_("""\
Symbol and source loading paths:
  --symbol-library=...
  --symbol-library-search=...
  --symbol-library-command=...
  --symbol-library-funcs=...
  --reset-symbol-library

  --source-library=...
  --source-library-search=...
  --reset-source-library

  Please note: Since there are no default paths and no configuration
               files are read, you will have to specify all symbol and
               source paths on the command line.
"""))
    sys.stdout.write("\n")
    sys.stdout.write(_("""\
Net naming options:
  --net-naming-priority=net-attribute|netname-attribute
                                            [default: net-attribute]
  --default-net-name=...                    [default: unnamed_net]
  --default-bus-name=...                    [default: unnamed_bus]
"""))
    sys.stdout.write("\n")
    sys.stdout.write(_("""\
Hierarchy options:
  --traverse-hierarchy                      [default]
  --dont-traverse-hierarchy

  --hierarchy-refdes-mangle=yes|no          [default: yes]
  --hierarchy-netname-mangle=yes|no         [default: yes]
  --hierarchy-netattrib-mangle=yes|no       [default: yes]
  --hierarchy-refdes-separator=...          [default: /]
  --hierarchy-refdes-order=append|prepend   [default: append]
  --hierarchy-netname-separator=...         [default: /]
  --hierarchy-netname-order=append|prepend  [default: append]
"""))
    sys.stdout.write("\n")
    sys.stdout.write(_("""\
Ignoring errors:
  --ignore-errors
  --dont-ignore-errors                      [default]
"""))
    sys.stdout.write("\n")
    sys.stdout.write(_("""\
Reporting errors:
  --report-gui
  --show-error-coordinates
  --dont-show-error-coordinates             [default]
"""))
    sys.stdout.write("\n")
    sys.stdout.write(_("""\
Miscellaneous options:
      --list-backends     print a list of available netlist backends
      --sab-context CTX   force the sab context to CTX
  -h, --help              help; this message
  -V, --version           show version information
  --                      treat all remaining arguments as filenames
"""))
    sys.stdout.write("\n")
    sys.stdout.write(_("Report %s bugs to %s\n")
                     % (xorn.config.PACKAGE_NAME,
                        xorn.config.PACKAGE_BUGREPORT))
    sys.exit(0)

## Print version info and exit.
#
# Print gEDA version and copyright/warranty notices to stdout and exit
# with status \c 0.

def version():
    sys.stdout.write("%s - gEDA netlister\n" % xorn.config.PACKAGE_STRING)
    sys.stdout.write(_("Copyright (C) 1998-2012 gEDA developers\n"))
    sys.stdout.write(_("Copyright (C) 2020 Roland Lutz\n"))
    sys.stdout.write("\n")
    sys.stdout.write(_(
"This program is free software; you can redistribute it and/or\n"
"modify it under the terms of the GNU General Public License\n"
"as published by the Free Software Foundation; either version 2\n"
"of the License, or (at your option) any later version.\n"))
    sys.stdout.write("\n")
    sys.stdout.write(_(
"This program is distributed in the hope that it will be useful,\n"
"but WITHOUT ANY WARRANTY; without even the implied warranty of\n"
"MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the\n"
"GNU General Public License for more details.\n"))
    sys.exit(0)


def enable_report_gui():
    global report_gui
    global report_gui_buf
    global report_gui_stderr

    if report_gui is not None:
        return

    try:
        import gaf.netlist.reportgui
    except:
        sys.stderr.write(
            _("%s: Can't set up GUI dialog; is pygtk installed?\n")
                % xorn.command.program_short_name)
        return

    sys.stderr.write(_("Redirecting warnings and errors to GUI dialog...\n"))
    sys.stderr.flush()

    report_gui = gaf.netlist.reportgui
    report_gui_stderr = sys.stderr
    report_gui_buf = cStringIO.StringIO()
    sys.stderr = report_gui_buf

def daemonize(fun):
    try:
        pid = os.fork()
        if pid > 0:
            # parent process: continue on
            return
    except OSError as e:
        sys.stderr.write("fork: %s\n" % e.strerror)
        # can't fork: just call fun() un-daemonized and continue on
        fun()
        return

    ### Don't return from here on! ###

    try:
        os.chdir('/')
        os.setsid()
        try:
            pid = os.fork()
            if pid > 0:
                # exit from intermediate process
                os._exit(0)
        except OSError, e:
            sys.stderr.write("fork: %s\n" % e.strerror)
            # can't fork second time: call fun() in intermediate process

        fun()
    finally:
        os._exit(0)

def main():
    try:
        inner_main()
        sys.exit(0)
    except SystemExit as e:
        if report_gui is not None:
            sys.stderr = report_gui_stderr
            log = report_gui_buf.getvalue()
            report_gui_buf.close()
            daemonize(lambda: report_gui.report_messages(e.code, log))
        raise
    except KeyboardInterrupt:
        raise
    except:
        if report_gui is not None:
            daemonize(lambda: report_gui.report_crash())
        raise

def inner_main():
    # TODO: this is totally hacky, re-do this correctly
    dirname = os.path.dirname(__file__)
    if dirname.endswith('/command'):
        # not installed
        # exploiting a bug in os.path.dirname
        gaf.netlist.backend.load_path.insert(
            0, os.path.dirname(dirname) + '/backend')
    else:
        # installed
        gaf.netlist.backend.load_path.insert(
            0, dirname + '/backends')

    # command line arguments

    output_filename = None
    backend_name = None

    # Parameters passed to the backend from the command line.
    ## List of arguments passed to the netlist backend via the
    ## `-O' command-line option.
    backend_params = []

    interactive_mode = False
    verbose_mode = False

    ## Specify which attribute, \c net or \c netname, has priority if
    ## a net is found with two names.  Any netname conflict will be
    ## resolved using the chosen attribute.
    #
    # This option is used when both net and netname attributes are
    # defined for some nets in your schematic and you want to specify
    # which one should define the net names that will be used for
    # netlisting.  See the \c net= attribute mini-HOWTO and Master
    # attributes list for more information on these attributes.
    #
    # By default, \c net= attributes beat \c netname= attributes.

    prefer_netname_attribute = False

    ## This is the default name used for nets for which the user has
    ## set no explicit name via the \c netname= or \c net= attributes.
    #
    # Define the default net name for the nets unnamed in the schematic.
    #
    # It is used to create netnames of the form "unnamed_netN" where N
    # is a number.

    default_net_name = 'unnamed_net'

    ## This is the default name used for buses for which the user has
    ## set no explicit name via the \c netname= or \c net= attributes.
    #
    # Define the default bus name for the buses unnamed in the schematic.

    default_bus_name = 'unnamed_bus'

    ## Decides if the hierarchy is traversed or not.  If this is
    ## disabled then Netlist.__init__ will not go down searching for
    ## any underlying sources.
    #
    # By default, hierarchy processing is enabled.

    traverse_hierarchy = True

    flat_package_namespace = False
    flat_netname_namespace = False
    flat_netattrib_namespace = False

    refdes_order = APPEND
    refdes_separator = '/'
    netname_order = APPEND
    netname_separator = '/'

    ignore_errors = False
    show_error_coordinates = False

    list_backends = False

    sab_context = []

    try:
        options, args = getopt.getopt(xorn.command.args, 'c:g:hil:L:m:o:O:vV',
            ['verbose',

             'symbol-library=',
             'symbol-library-search=',
             'symbol-library-command=',
             'symbol-library-funcs=',
             'reset-symbol-library',

             'source-library=',
             'source-library-search=',
             'reset-source-library',

             'net-naming-priority=',
             'default-net-name=', 'default-bus-name=',

             'traverse-hierarchy', 'dont-traverse-hierarchy',
             'hierarchy-refdes-mangle=',
             'hierarchy-netname-mangle=',
             'hierarchy-netattrib-mangle=',
             'hierarchy-refdes-separator=',
             'hierarchy-refdes-order=',
             'hierarchy-netname-separator=',
             'hierarchy-netname-order=',

             'ignore-errors', 'dont-ignore-errors',
             'report-gui',
             'show-error-coordinates', 'dont-show-error-coordinates',

             'list-backends', 'sab-context=', 'help', 'version'])
    except getopt.GetoptError as e:
        xorn.command.invalid_arguments(e.msg)

    for option, value in options:
        if option == '-o':
            output_filename = value
        elif option == '-g':
            backend_name = value
        elif option == '-L':
            # Argument is a directory to add to the backend load path.
            gaf.netlist.backend.load_path.insert(0, value)
        elif option == '-O':
            backend_params.append(value)
        elif option == '-c':
            eval(value, {})
        elif option == '-i':
            interactive_mode = True
        elif option == '-v' or option == '--verbose':
            verbose_mode = True

        elif option == '-l' or option == '-m':
            xorn.command.invalid_arguments(
                _("%s: invalid option: "
                  "executing scheme code is not supported") % option)

        elif option == '--symbol-library':
            symbol_library(value, False, '--symbol-library')
        elif option == '--symbol-library-search':
            symbol_library(value, True, '--symbol-library-search')
        elif option == '--symbol-library-command':
            symbol_library_command(value)
        elif option == '--symbol-library-funcs':
            symbol_library_funcs(value)
        elif option == '--reset-symbol-library':
            gaf.clib.reset()

        elif option == '--source-library':
            source_library(value)
        elif option == '--source-library-search':
            source_library_search(value)
        elif option == '--reset-source-library':
            del gaf.netlist.slib.slib[:]

        elif option == '--net-naming-priority':
            if value == 'net-attribute':
                prefer_netname_attribute = False
            elif value == 'netname-attribute':
                prefer_netname_attribute = True
            else:
                xorn.command.invalid_arguments(
                    _("'%s' is not a valid net-naming-priority value") % value)
        elif option == '--default-net-name':
            default_net_name = value
        elif option == '--default-bus-name':
            default_bus_name = value

        elif option == '--traverse-hierarchy':
            traverse_hierarchy = True
        elif option == '--dont-traverse-hierarchy':
            traverse_hierarchy = False

        elif option == '--hierarchy-refdes-mangle':
            flat_package_namespace = not parse_bool(value)
        elif option == '--hierarchy-netname-mangle':
            flat_netname_namespace = not parse_bool(value)
        elif option == '--hierarchy-netattrib-mangle':
            flat_netattrib_namespace = not parse_bool(value)
        elif option == '--hierarchy-refdes-separator':
            refdes_separator = value
        elif option == '--hierarchy-refdes-order':
            refdes_order = parse_order(value)
        elif option == '--hierarchy-netname-separator':
            netname_separator = value
        elif option == '--hierarchy-netname-order':
            netname_order = parse_order(value)

        elif option == '--ignore-errors':
            ignore_errors = True
        elif option == '--dont-ignore-errors':
            ignore_errors = False

        elif option == '--report-gui':
            enable_report_gui()

        elif option == '--show-error-coordinates':
            show_error_coordinates = True
        elif option == '--dont-show-error-coordinates':
            show_error_coordinates = False

        elif option == '--list-backends':
            list_backends = True
        elif option == '--sab-context':
            value = value.lower().split(',')
            if 'none' in value:
                sab_context = None
            else:
                sab_context.extend(value)
        elif option == '-h' or option == '--help':
            usage()
        elif option == '-V' or option == '--version':
            version()

    if list_backends:
        sys.stdout.write(_("List of available backends:\n\n"))
        for name in gaf.netlist.backend.list_backends():
            sys.stdout.write("    %s\n" % name)
        sys.stdout.write("\n")
        sys.exit(0)

    if not args:
        xorn.command.invalid_arguments(_(
            "No schematics files specified for processing"))

    if not interactive_mode and backend_name is None:
        xorn.command.invalid_arguments(_(
            "You gave neither backend to execute nor interactive mode!"))

    if backend_name is None and output_filename is not None:
        xorn.command.invalid_arguments(_(
            "Specified an output filename but no backend"))

    netlist = gaf.netlist.netlist.Netlist(
        toplevel_filenames = args,
        traverse_hierarchy = traverse_hierarchy,
        verbose_mode = verbose_mode,
        prefer_netname_attribute = prefer_netname_attribute,
        flat_package_namespace = flat_package_namespace,
        flat_netname_namespace = flat_netname_namespace,
        flat_netattrib_namespace = flat_netattrib_namespace,
        refdes_mangle_func = lambda basename, namespace:
            mangle(basename, namespace, refdes_separator, refdes_order,
                                        refdes_separator, refdes_order),
        netname_mangle_func = lambda basename, namespace:
            mangle(basename, namespace, refdes_separator, refdes_order,
                                        netname_separator, netname_order),
        default_net_name = default_net_name,
        default_bus_name = default_bus_name,
        show_error_coordinates = show_error_coordinates)

    if netlist.failed and not ignore_errors:
        # there were errors during netlist creation
        sys.exit(2)

    if backend_name is not None:
        try:
            m = gaf.netlist.backend.load(backend_name)
        except ImportError:
            sys.stderr.write(
                _("%s: Could not find backend `%s' in load path.\n")
                % (xorn.command.program_short_name, backend_name))
            sys.stderr.write(
                _("Run `%s --list-backends' for a full list of available "
                  "backends.\n") % xorn.command.program_name)
            sys.exit(1)

        if m.run.func_code.co_argcount == 2 and backend_params:
            sys.stderr.write(
                _("%s: The `%s' backend doesn't take arguments.\n")
                % (xorn.command.program_short_name, backend_name))
            sys.exit(1)

        # Note that both None and and empty list test as false
        # so can't use not
        if (sab_context == []
                and 'SAB_CONTEXT' in dir(m)
                and isinstance(m.SAB_CONTEXT, types.ListType)):
            sab_context = m.SAB_CONTEXT

    if netlist.failed and not ignore_errors:
        # there were netlist errors during backend loading (shouldn't happen)
        sys.exit(3)

    if interactive_mode:
        import code
        code.interact(local = {
            'netlist': netlist,
            # default values
            '__name__': '__console__',
            '__doc__': None
        })

    if backend_name is None:
        # We can exit here.  If the interactive session causes netlist
        # errors, this does *not* mean the command failed as a whole
        # (it's probably due to the user playing around) unless in
        # addition, a backend was specified.
        return

    if netlist.failed and not ignore_errors:
        # there were netlist errors during interactive session
        sys.stderr.write(_("Exiting due to previous errors.\n"))
        sys.exit(3)

    if sab_context:
        sab.process(netlist, sab_context)

    class NetlistFailedError(Exception):
        pass

    def write(f):
        if m.run.func_code.co_argcount == 2:
            m.run(f, netlist)
        else:
            m.run(f, netlist, backend_params)

        if netlist.failed and not ignore_errors:
            raise NetlistFailedError

    try:
        # If the output file name is "-", use stdout instead.
        if output_filename is None or output_filename == '-':
            write(sys.stdout)
        else:
            try:
                xorn.fileutils.write(output_filename, write, backup = False)
            except (IOError, OSError) as e:
                sys.stderr.write(_("%s: %s: %s\n") % (
                    xorn.command.program_short_name,
                    output_filename, e.strerror))
                sys.exit(1)
    except NetlistFailedError:
        sys.exit(3)
