import sys, types
from collections import OrderedDict
import gaf.netlist.netlist
import sab_utils

VALID_ACTIONS=('discard','bypass','exec')

#locate a refdes in the supplied context list
#return None if not found
def find_refdes(refdes,ctx):
    for i in range(len(ctx)):
        if ctx[i][0] == refdes:
            return i

    return None


#add the tuple comp (which represents a component) to
#the context list ctx in ordered postition
def add_refdes(ctx,comp):
    for i in range(len(ctx)):
        if comp[1] < 1000000000 and ctx[i][1] == comp[1]:
            sys.stderr.write("WARNING: Both %s and %s specify the same order for the same context.\n"
                % (ctx[i][0],comp[0]))
            continue
        if ctx[i][1] > comp[1]:
            ctx.insert(i,comp)
            return
    ctx.append(comp)

# break up a sab-param into the context, optional order #, action, and action_parameters
# send it back as a tuple
def parse_param(param, refdes):
    if not param or not isinstance(param,types.StringType):
        return (None,None,None,None)

    parts = param.split(':')
    if len(parts) < 2:
        sys.stderr.write('''WARNING: Malformed sab-param for component %s: %s
         Did you forget the action?\n''' % (refdes,param))
        return (None, None, None, None)

    parts[0] = parts[0].lower()
    if parts[1][0] == '#':
        parts[1]=int(parts[1][1:])
    else:
        parts.insert(1,1000000000) #hopefully no one ever uses a schematic with a billion ordered SAB components
    parts[2] = parts[2].lower()
    if len(parts) < 4:
        parts.append(None)
    else:
        action_parameters = ':'.join(parts[3:])
        parts[3] = action_parameters

    if parts[2] in VALID_ACTIONS:
        return tuple(parts[:4])

    sys.stderr.write('WARNING: The %s action is not valid in sab-param for component %s in context %s\n'
        % (parts[2],refdes,parts[0]))
    return (None, None, None, None)

# The top level SAB processing is done here.
# Turns out we can't use the verbose_mode flag from xorn-netlist
# because gnetlist defaults to sending the -v flag. Maybe someday...
def process(nets, context, be_verbose = False):
    sab_utils.verbose = be_verbose

    sab_utils.verboseMsg('\nStarting SAB processing',0)

    ctx = OrderedDict()
    for c in context:
        if c and isinstance(c,types.StringType):  # skip any contexts which are the empty string or not strings
            ctx[c] = []

    if ctx:
        for c in nets.components:
            for attr in c.blueprint.get_attributes('sab-param'):
                (context, order, action, parms)=parse_param(attr,c.refdes)
                if context in ctx:
                    refdes = c.refdes
                    slot = c.blueprint.get_attribute('slot',None)
                    if slot is not None:
                        refdes = refdes + ':' + slot
                    if find_refdes(refdes,ctx[context]) is None:
                        add_refdes(ctx[context],(refdes,order,action,parms,c))
                    else:
                        sys.stderr.write('''WARNING: Component %s defines multiple sab-param for the %s context.
         The extras will be ignored.\n''' % (refdes,context))

        for context in ctx:
            sab_utils.verboseMsg('Processing %s context...' % (context),1)
            if not ctx[context]:
                if verbose:
                    sys.stderr.write("INFORMATION: Nothing to be done for context %s\n" % (context))
                continue

            for component in ctx[context]:
                sab_utils.verboseMsg('Processing component %s...' % (component[0]),2)
                if component[2] == 'exec': #the external script is responsible for the whole shebang
                    sab_utils.exec_extern(nets, component[4], component[3])
                else:
                    if component[2] == 'bypass': #first bypass then discard whatever is left
                        sab_utils.bypass(nets, component[4], component[3])

                    sab_utils.discard(nets, component[4])

    sab_utils.verboseMsg('SAB processing complete\n',0)
