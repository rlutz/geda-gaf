import sys, types
from collections import OrderedDict
import gaf.netlist.netlist

VALID_ACTIONS=('discard','bypass','exec')

verbose = False

def verboseMsg(msg, indent):
    if not verbose:
        return

    print ''.zfill(4*indent).replace('0',' ')+msg

#Bypass a component. Cross connect the nets connected to the pins
#in each group from the shorting list 'shorts'. The id of the resulting
#net will be that if the first pin listed in each group. Only the nets
#are handled here. The component will be discarded later.
def bypass(nets, component, shorts):
    pass

#discard the specified component from the netlist.
#This is a two stage process. First remove any connections,
#then remove the actual component.
#Philosophical question: Would it be better to leave the blueprint level
#of the hierarchy alone and only remove the component from the derived levels?
#Since I don't know that some backends don't use the blueprint level it would
#defeat the purpose of what we are doing to leave the components there. So
#I am going all the way to the bottom and working my way up.
def discard(nets, comp):
    if comp.cpins:
        for pin in comp.cpins:
            if not pin.local_net.net.is_unconnected_pin:
                if pin.blueprint is not None:
                    pin.blueprint.net.pins.remove(pin.blueprint)
                    if not pin.blueprint.net.pins:
                        pin.blueprint.net.schematic.nets.remove(pin.blueprint.net)
                else:
                    verboseMsg('Found a virtual pin.',3)

                pin.local_net.cpins.remove(pin)
                if not pin.local_net.cpins:
                    pin.local_net.net.local_nets.remove(pin.local_net)

                pin.local_net.net.component_pins.remove(pin)
                if not pin.local_net.net.local_nets and not pin.local_net.net.component_pins:
                    netist.remove(pin.local_net.net)
                if len(pin.local_net.net.component_pins) == 1:
                    pin.local_net.net.is_unconnected_pin=True

    #remove it from the blueprint level first
    comp.sheet.blueprint.components.remove(comp.blueprint)
    comp.sheet.blueprint.components_by_ob.pop(comp.blueprint.ob)

    #now from the derived level sheet
    comp.sheet.components.remove(comp)
    comp.sheet.components_by_blueprint.pop(comp.blueprint)

    #and from the package
    package=nets.packages_by_refdes[comp.refdes]
    package.components.remove(comp)
    #which may leave an empty package
    if not package.components:
        verboseMsg('package for %s is now empty, removing' % (comp.refdes),3)
        nets.packages.remove(package)
        nets.packages_by_refdes.pop(comp.refdes)

    #and finally from the component list itself
    nets.components.remove(comp)
    verboseMsg('Discard complete',3)

# break up a sab-param into the context, action, and action_parameters
# send it back as a tuple
def parse_param(param, refdes):
    if not param or not isinstance(param,types.StringType):
        return (None,None,None)

    parts=param.split(':')
    if len(parts)<2:
        sys.stderr.write('WARNING: Malformed sab-param for component %s: %s\n         Did you forget the action?\n' % (refdes,param))
        return (None, None, None)

    parts[0]=parts[0].lower()
    parts[1]=parts[1].lower()
    if len(parts)<3:
        parts.append(None)
    else:
        action_parameters=':'.join(parts[2:])
        parts[2]=action_parameters

    if parts[1] in VALID_ACTIONS:
        return (parts[0], parts[1], parts[2])

    sys.stderr.write('WARNING: The %s action is not valid in sab-param for component %s\n' % (parts[1],refdes))
    return (None, None, None)

# The top level SAB processing is done here.
# Turns out we can't use the verbose_made flag from xorn-netlist
# because gnetlist defaults to sending the -v flag. Maybe someday...
def process(nets, context, be_verbose=True):
    global verbose
    verbose=be_verbose

    verboseMsg('\nStarting SAB processing',0)

    ctx=OrderedDict()
    for c in context:
        if c:  # skip any contexts which are the empty string
            ctx[c]={}

    if ctx:
        for c in nets.components:
            for attr in c.blueprint.get_attributes('sab-param'):
                (context, action, parms)=parse_param(attr,c.refdes)
                if context in ctx:
                    refdes=c.refdes
                    slot=c.blueprint.get_attribute('slot',None)
                    if slot is not None:
                        refdes= refdes + ':' + slot
                    if refdes not in ctx[context]:
                        ctx[context][refdes]=(action,parms,c)
                    else:
                        sys.stderr.write('''WARNING: Component %s defines multiple sab-param for the %s context.
         The extras will be ignored.\n''' % (refdes,context))

        for context in ctx:
            verboseMsg('Processing %s context...' % (context),1)
            if not ctx[context]:
                if verbose:
                    sys.stderr.write("INFORMATION: Nothing to be done for context %s\n" % (context))
                continue

            for component in ctx[context]:
                verboseMsg('Processing component %s...' % (component),2)
                if ctx[context][component][0] == 'exec': #the external script is responsible for the whole shebang
                    pass
                else:
                    if ctx[context][component][0] == 'bypass': #first bypass then discard whatever is left
                        bypass(nets, ctx[context][component][2], ctx[context][component][1])

                    discard(nets, ctx[context][component][2])

    verboseMsg('SAB processing complete\n',0)
