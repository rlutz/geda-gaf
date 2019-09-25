#!/bin/sh

gaf="$(which gaf)"
if [ "x${gaf}" = x ]; then
	echo "regen-postscript.sh: can't find the \"gaf\" executable"
	exit 1
fi
echo "Using \"${gaf}\""

if [ \! -f gTAG.sch ]; then
	echo "regen-postscript.sh: must be run in the examples/gTAG source directory"
	exit 1
fi

for stem in gTAG gTAG-consio gTAG-jtagio gTAG-psu gTAG-ucont; do
	${gaf} export -p iso_a4 -m "1cm;1cm;1cm;1cm" -o "${stem}.tmp.ps" \
		"${stem}.sch" &&
	  sed -e '/^%%CreationDate: /d' -i "${stem}.tmp.ps" &&
	  mv "${stem}.tmp.ps" "${stem}.ps"
done

rm -f *.tmp.ps
