#!/bin/sh

srcdir="`dirname $1`"
stem="`basename $1 .output`"

in="${srcdir}/${stem}.sym"
ref="${srcdir}/${stem}.output"
new="${stem}.new"

mkdir -p logs

GEDALOG=logs \
../src/gsymcheck -vv "${in}" |
	grep -v "gEDA/gsymcheck version" |
	grep -v "ABSOLUTELY NO WARRANTY" |
	grep -v "This is free software" |
	grep -v "the COPYING file" |
	grep -v "Checking: " |
	grep -v '^$' > "${new}"

diff "${ref}" "${new}" || exit 2
rm "${new}"
