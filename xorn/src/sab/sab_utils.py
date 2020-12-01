import sys
import imp
import types

verbose = False

def verboseMsg(msg, indent):
    if not verbose:
        return

    print ''.zfill(4*indent).replace('0',' ') + msg

#Load an external script and call its sab_process function passing
#the netlist, component, and parameter string
def exec_extern(nets, component, param):
    script,c,param = param.partition(':')

    try:
        f,path,desc = imp.find_module(script,['.'] + sys.path)
        script = imp.load_module(script,f,path,desc)
    except ImportError:
        sys.stderr.write("WARNING: Unable to load script %s" % (script))
        script = None
    finally:
        if f is not None:
            f.close()

    if(script is not None and
       'sab_process' in dir(script) and
       isinstance(script.sab_process,types.FunctionType) and
       script.sab_process.__code__.co_argcount == 3):
        script.sab_process(nets,component,param)

    elif script is None:
        return
    elif 'sab_process' not in dir(script):
        sys.stderr.write("WARNING: Script %s missing 'sab_process' function." % (script.__name__))
    elif not isinstance(script.sab_process,types.FunctionType):
        sys.stderr.write("WARNING: %s.sab_process is not a function." % (script.__name__))
    else:
        sys.stderr.write('WARNING: %s.sab_process must take three parameters.' % (script.__name__))

#Bypass a component. Cross connect the nets connected to the pins
#in each group from the shorting list 'shorts'. The id of the resulting
#net will be that if the first pin listed in each group, unless an 'as'
#is used. Only the nets are handled here.
def bypass(nets, component, shorts):
    verboseMsg("Starting bypass.",3)
    short_list = shorts.split(';')
    for short in short_list:
        parts = short.partition('as')
        if not parts[0].strip().replace(',','').isdigit():
            sys.stderr.write('''WARNING: Only digits and commas allowed in shorting list. Ignoring.
         %s: %s\n''' % (component.blueprint.refdes,parts[0]))
            continue

        verboseMsg('Bypassing %s' % (parts[0]),4)
        pins=parts[0].strip().split(',')
        if len(pins)<2:
            sys.stderr.write('''WARNING: Two or more pins needed in shorting list for component %s (current list: %s)\n'''
                % (component.blueprint.refdes,parts[0]))

        dest_net = component.cpins_by_number[pins[0]].local_net.net

        if not parts[2] == '':
            dest_net.name = parts[2]
            dest_net.unnamed_counter = None

        src_nets = []
        for pin in pins[1:]:
            if pin not in component.cpins_by_number:
                sys.stderr.write('WARNING: Component %s does not have a pin %s. Ignoring.\n'
                    % (component.blueprint.refdes,pin))
                continue

            new_net = component.cpins_by_number[pin].local_net.net
            if(dest_net is not new_net and
               new_net not in src_nets and
               not new_net.is_unconnected_pin):
                src_nets.append(new_net)

        for net in src_nets:
            net.merge_into(dest_net)

    verboseMsg('Bypass complete',3)



#discard the specified component from the netlist.
#This is a two stage process. First remove any connections,
#then remove the actual component.
#Philosophical question: Would it be better to leave the blueprint level
#of the hierarchy alone and only remove the component from the derived levels?
#Since I don't know that some backends don't use the blueprint level it would
#defeat the purpose of what we are doing to leave the components there. So
#I am going all the way to the bottom and working my way up.
def discard(nets, comp):
    verboseMsg("Starting discard.",3)
    if comp.cpins:
        for pin in comp.cpins:
            if not pin.local_net.net.is_unconnected_pin:
                if pin.blueprint is not None:
                    pin.blueprint.net.pins.remove(pin.blueprint)
                    if not pin.blueprint.net.pins:
                        pin.blueprint.net.schematic.nets.remove(pin.blueprint.net)
                else:
                    verboseMsg('Found a virtual pin.',4)

                pin.local_net.cpins.remove(pin)
                if not pin.local_net.cpins:
                    pin.local_net.net.local_nets.remove(pin.local_net)

                if pin in pin.local_net.net.component_pins:
                    pin.local_net.net.component_pins.remove(pin)
                if not pin.local_net.net.local_nets and not pin.local_net.net.component_pins:
                    netist.remove(pin.local_net.net)
                if len(pin.local_net.net.component_pins) == 1:
                    pin.local_net.net.is_unconnected_pin = True

    #remove it from the blueprint level first
    comp.sheet.blueprint.components.remove(comp.blueprint)
    comp.sheet.blueprint.components_by_ob.pop(comp.blueprint.ob)

    #now from the derived level sheet
    comp.sheet.components.remove(comp)
    comp.sheet.components_by_blueprint.pop(comp.blueprint)

    #and from the package
    package = nets.packages_by_refdes[comp.refdes]
    package.components.remove(comp)
    #which may leave an empty package
    if not package.components:
        verboseMsg('Package for %s is now empty, removing' % (comp.refdes),4)
        nets.packages.remove(package)
        nets.packages_by_refdes.pop(comp.refdes)

    #and finally from the component list itself
    nets.components.remove(comp)
    verboseMsg('Discard complete',3)
