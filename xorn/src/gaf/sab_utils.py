import sys
import imp
import types
from gettext import gettext as _

verbose = False

def verboseMsg(msg, indent):
    if not verbose:
        return

    print '    '*indent + msg

# Load an external script and call its sab_process function passing
# the netlist, component, and parameter string
def exec_extern(nets, context, component, param):
    script, c, param = param.partition(':')

    try:
        f, path, desc = imp.find_module(script, ['.'] + sys.path)
        script = imp.load_module(script, f, path, desc)
    except ImportError:
        sys.stderr.write(_("WARNING: Unable to load script %s\n") % (script))
        script = None
        f = None
    finally:
        if f is not None:
            f.close()

    try:
        if (script is not None
            and isinstance(script.sab_process, types.FunctionType)
            and script.sab_process.__code__.co_argcount == 4):
                script.sab_process(nets, context, component, param)
        elif script is None:
            return
        elif not isinstance(script.sab_process, types.FunctionType):
            sys.stderr.write(_("WARNING: %s.sab_process is not a function.\n")
                               % (script.__name__))
        else:
            sys.stderr.write(_('WARNING: %s.sab_process must take four parameters.\n')
                               % (script.__name__))
    except AttributeError:
        sys.stderr.write(_("WARNING: Script %s missing 'sab_process' function.\n")
                           % (script.__name__))

# Bypass a component. Cross connect the nets connected to the pins
# in each group from the shorting list 'shorts'. The id of the resulting
# net will be that if the first pin listed in each group, unless an 'as'
# is used. Only the nets are handled here.
def bypass(nets, component, shorts):
    verboseMsg(_("Starting bypass."), 3)

    #check to see if the component has already been discarded.
    #I'm only going to check the top level. If everything is working properly
    #then a discard should have removed it there. If a script has been mucking
    #about then the netlist may be in an unstable condition but there is only
    #so much we can check for.
    if component not in nets.components:
        verboseMsg(_('Component %s has already been discarded. Aborting bypass.')
                  % (component.refdes),3)
        return

    short_list = shorts.split(';')
    for short in short_list:
        parts = short.partition('as')
        if not parts[0].strip().replace(',', '').isdigit():
            sys.stderr.write(_("WARNING: Only digits and commas allowed in "
                             "shorting list. Ignoring.\n"
                             "         %s: %s\n")
                               % (component.blueprint.refdes, parts[0]))
            continue

        verboseMsg('Bypassing %s' % (parts[0]), 4)
        pins = parts[0].strip().split(',')
        if len(pins) < 2:
            sys.stderr.write(_("WARNING: Two or more pins needed in shorting "
                             "list for component %s (current list: %s)\n")
                               % (component.blueprint.refdes, parts[0]))

        dest_net = component.cpins_by_number[pins[0]].local_net.net

        if not parts[2] == '':
            dest_net.name = parts[2]
            dest_net.unnamed_counter = None

        src_nets = []
        for pin in pins[1:]:
            if pin not in component.cpins_by_number:
                sys.stderr.write(_('WARNING: Component %s does not have a pin '
                                 '%s. Ignoring.\n')
                                   % (component.blueprint.refdes, pin))
                continue

            new_net = component.cpins_by_number[pin].local_net.net
            if (dest_net is not new_net
                    and new_net not in src_nets
                    and not new_net.is_unconnected_pin):
                src_nets.append(new_net)

        for net in src_nets:
            net.merge_into(dest_net)

    verboseMsg(_('Bypass complete'), 3)



# discard the specified component from the netlist.
# This is a two stage process. First remove any connections,
# then remove the actual component.
# Philosophical question: Would it be better to leave the blueprint level
# of the hierarchy alone and only remove the component from the derived levels?
# Since I don't know that some backends don't use the blueprint level it would
# defeat the purpose of what we are doing to leave the components there. So
# I am going all the way to the bottom and working my way up.
def discard(nets, comp):
    verboseMsg(_("Starting discard."), 3)
    if comp.cpins:
        for pin in comp.cpins:
            if not pin.local_net.net.is_unconnected_pin:
                if pin.blueprint is not None:
                    pin.blueprint.net.pins.remove(pin.blueprint)
                    if not pin.blueprint.net.pins:
                        pin.blueprint.net.schematic.nets.remove(
                            pin.blueprint.net)
                else:
                    verboseMsg(_('Found a virtual pin.'), 4)

                pin.local_net.cpins.remove(pin)
                if not pin.local_net.cpins:
                    pin.local_net.net.local_nets.remove(pin.local_net)

                if pin in pin.local_net.net.component_pins:
                    pin.local_net.net.component_pins.remove(pin)
                if not pin.local_net.net.local_nets \
                        and not pin.local_net.net.component_pins:
                    netist.remove(pin.local_net.net)
                if len(pin.local_net.net.component_pins) == 1:
                    pin.local_net.net.is_unconnected_pin = True

    # it is possible that some other context has already been here...

    # remove it from the blueprint level first
    if comp.blueprint in comp.sheet.blueprint.components:
        comp.sheet.blueprint.components.remove(comp.blueprint)
    if comp.blueprint.ob in comp.sheet.blueprint.components_by_ob:
        comp.sheet.blueprint.components_by_ob.pop(comp.blueprint.ob)

    # now from the derived level sheet
    if comp in comp.sheet.components:
        comp.sheet.components.remove(comp)
    if comp.blueprint in comp.sheet.components_by_blueprint:
        comp.sheet.components_by_blueprint.pop(comp.blueprint)

    # and from the package
    if comp.refdes in nets.packages_by_refdes:
        package = nets.packages_by_refdes[comp.refdes]

        if comp in package.components:
            package.components.remove(comp)
            # which may leave an empty package
            if not package.components:
                verboseMsg(_('Package for %s is now empty, removing') % (comp.refdes), 4)
                nets.packages.remove(package)
                nets.packages_by_refdes.pop(comp.refdes)

    # and finally from the component list itself
    if comp in nets.components:
        nets.components.remove(comp)
    verboseMsg(_('Discard complete'), 3)
