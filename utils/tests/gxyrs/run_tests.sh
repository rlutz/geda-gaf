#!/bin/sh

# Copyright (C) 2008 Carlos Nieves Onega
#    adapted from refdes_renum test suite by Dan McMahill.
# Copyright (C) 2008 other contributors
#                        (see ChangeLog or SCM history for details)
 
# This file is part of gxyrs.

# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License
# as published by the Free Software Foundation, version 2
# of the License.

# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.

# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  
# 02110-1301, USA.

usage() {
cat << EOF

$0 -- Testsuite program for gxyrs

Usage

  $0 [-h | --help]
  $0 [-r | --regen] [test1 [test2 [....]]]

Options

  -h | --help     Prints this help message and exits.

  -r | --regen    Regenerates the reference files.  If you use
                  this option, YOU MUST HAND VERIFY THE RESULTS
                  BEFORE COMMITTING to the repository.

Description

$0 reads a file, tests.list,  describing tests to run on geda_filter.
If no specific test is specified on the $0 command line, then all 
tests are run.

Examples

$0
$0 --regen new_test 

EOF
}
while test -n "$1"
do
    case "$1"
    in

    -h|--help)
	usage
	exit 0
	;;

    -r|--regen)
	# regenerate the 'golden' output files.  Use this with caution.
	# In particular, all differences should be noted and understood.
	REGEN=1
	shift
	;;

    -*)
	echo "unknown option: $1"
	usage
	exit 1
	;;

    *)
	break
	;;

    esac
done


# make sure we have the right paths when running this from inside the
# source tree and also from outside the source tree.
here=`pwd`
srcdir=${srcdir:-$here}
srcdir=`cd $srcdir && pwd`

top_srcdir=${top_srcdir:-$here/../..}
top_srcdir=`cd $top_srcdir && pwd`
top_builddir=${top_builddir:-$here/../..}
top_builddir=`cd $top_builddir && pwd`
gxyrs_srcdir=`cd $top_srcdir/gxyrs && pwd`

# the perl program
PERL=${PERL:-perl}

GOLDEN_DIR=${srcdir}/outputs
INPUT_DIR=${srcdir}/inputs


TESTLIST=${srcdir}/tests.list

if test ! -f $TESTLIST ; then
    echo "ERROR: ($0)  Test list $TESTLIST does not exist"
    exit 1
fi

GXYRS_SCRIPT=${top_builddir}/gxyrs/gxyrs
if test ! -f $GXYRS_SCRIPT ; then
    echo "ERROR: ($0)  gxyrs script $GXYRS_SCRIPT does not exist"
    exit 1
fi

# Check if gxyrs.pm module exists
eval "${PERL} -I${gxyrs_srcdir} -Mgxyrs -e 1"
if test $? -ne 0; then
    echo "ERROR: ($0)  gxyrs module gxyrs.pm does not exist"
    exit 1
fi


# fail/pass/total counts
fail=0
pass=0
skip=0
tot=0

if test -z "$1" ; then
    all_tests=`awk 'BEGIN{FS="|"} /^#/{next} /^[ \t]*$/{next} {print $1}' $TESTLIST | sed 's; ;;g'`
else
    all_tests=$*
fi

cat << EOF

Starting tests in $here
srcdir:     $srcdir
top_srcdir: $top_srcdir
gxyrs srcdir: $gxyrs_srcdir
INPUT_DIR:  ${INPUT_DIR}
GOLDEN_DIR: ${GOLDEN_DIR}
script to test: ${top_srcdir}/gxyrs/gxyrs
all_tests:

${all_tests}

EOF

for t in $all_tests ; do

    # strip any leading garbage
    t=`echo $t | sed 's;^\*;;g'`

    tot=`expr $tot + 1`

    REGEN="${REGEN}" ${srcdir}/run-test "${t}"

    case "$?" in
	0) pass=`expr $pass + 1` ;;
	1) fail=`expr $fail + 1` ;;
	77) skip=`expr $skip + 1` ;;
    esac
done

echo "Passed $pass, failed $fail, skipped $skip out of $tot tests."

rc=0
if test $pass -ne $tot ; then
    rc=`expr $tot - $pass`

fi

exit $rc
