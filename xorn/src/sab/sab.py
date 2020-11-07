import sys, types
from collections import OrderedDict
import gaf.netlist.netlist
import sab_utils

VALID_ACTIONS=('discard','bypass','exec')

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

    sys.stderr.write('WARNING: The %s action is not valid in sab-param for component %s in context %s\n' % (parts[1],refdes,parts[0]))
    return (None, None, None)

# The top level SAB processing is done here.
# Turns out we can't use the verbose_made flag from xorn-netlist
# because gnetlist defaults to sending the -v flag. Maybe someday...
def process(nets, context, be_verbose=True):
    sab_utils.verbose=be_verbose

    sab_utils.verboseMsg('\nStarting SAB processing',0)

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
            sab_utils.verboseMsg('Processing %s context...' % (context),1)
            if not ctx[context]:
                if verbose:
                    sys.stderr.write("INFORMATION: Nothing to be done for context %s\n" % (context))
                continue

            for component in ctx[context]:
                sab_utils.verboseMsg('Processing component %s...' % (component),2)
                if ctx[context][component][0] == 'exec': #the external script is responsible for the whole shebang
                    pass
                else:
                    if ctx[context][component][0] == 'bypass': #first bypass then discard whatever is left
                        sab_utils.bypass(nets, ctx[context][component][2], ctx[context][component][1])

                    sab_utils.discard(nets, ctx[context][component][2])

    sab_utils.verboseMsg('SAB processing complete\n',0)
