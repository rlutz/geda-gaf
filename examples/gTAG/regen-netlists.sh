#!/bin/sh

gnetlist="$(which gnetlist)"
if [ "x${gnetlist}" = x ]; then
	echo "regen-netlists.sh: can't find the \"gnetlist\" executable"
	exit 1
fi
echo "Using \"${gnetlist}\""

if [ \! -f gTAG.sch ]; then
	echo "regen-netlists.sh: must be run in the examples/gTAG source directory"
	exit 1
fi

${gnetlist} -g PCB -o gTAG-pcb.net gTAG.sch
${gnetlist} -g bom -o gTAG.bom gTAG.sch
