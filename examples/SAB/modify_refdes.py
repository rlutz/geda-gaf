'''
Parameters:
<mask>,<levels>,<all>

<mask> - defines how the refdes is to be modified.
            Currently it is just a prefix
            Default - ''

<levels> - which levels of the component tree to modify
            'P' package level
            'C' Component level
            'B' Blueprint level
            Default - 'C'

<all> - If the component is part of a multi-slot package this
        boolean flag specifies whether to modify all the components
        in the package or only the one tagged with the sab-param
        attribute. Note: If the 'P' level is specified in <levels>
        then all components may be modified, via the package refdes,
        regardless of this value, depending on where the backend in use
        gets its refdes from (spice-sdb uses the blueprint level exclusively.)
        Default - False
'''

import types
from sab.sab_utils import verboseMsg
import sys

def sab_process(nets,comp,param):
    verboseMsg("Starting refdes modification",3)
#    print sys.path
    if param and isinstance(param,types.StringType):
        prefix,levels,all=param.split(',')
        levels=list(levels.upper())
        if all.lower() == 'true':
            all=True
        else:
            all=False
    else:
        prefix,levels,all=['','C',False]


    p=nets.packages_by_refdes[comp.refdes]
    if 'P' in levels:
        p.refdes=prefix+p.refdes
    if 'C' in levels:
        comp.refdes=prefix+comp.refdes
    if 'B' in levels:
        comp.blueprint.refdes=prefix+comp.blueprint.refdes

    if all and ('C' in levels or 'B' in levels):
        for c in p.components:
            if not c == comp:
                if 'C' in levels:
                    c.refdes=prefix+c.refdes
                if 'B' in levels:
                    c.blueprint.refdes=prefix+c.blueprint.refdes
    verboseMsg("refdes modification complete.",3)
