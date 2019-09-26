v 20080127 1
C 40000 40000 0 0 0 title-A3-2.sym
{
T 52300 41300 5 30 1 1 0 4 1
Title=duplicate inout test
T 51400 40300 5 16 1 1 0 4 1
filename=inouttest.sch
T 55850 41500 5 16 1 1 0 4 1
revision=1
T 56050 40100 5 16 1 1 0 6 1
page=1
T 56200 40100 5 16 1 1 0 0 1
number_of_pages=2
T 55850 40850 5 12 1 1 0 4 1
date=08.08.08
T 54450 40450 5 16 1 1 0 4 1
author=-<(kmk)>-
}
C 43700 43300 1 0 0 inouttest.sym
{
T 44000 47600 5 10 1 1 0 0 1
refdes=SUB1
T 45200 44300 5 10 1 1 0 4 1
source=inouttest_sub.sch
T 45200 47300 5 20 1 1 270 1 1
description=inouttest
T 44000 49200 5 8 0 0 0 0 1
device=none
}
C 42800 47100 1 0 0 resistor-2.sym
{
T 43200 47450 5 10 0 0 0 0 1
device=RESISTOR
T 43000 47400 5 10 1 1 0 0 1
refdes=R5
T 43400 47400 5 10 1 1 0 0 1
footprint=0805
}
C 42800 46600 1 0 0 resistor-2.sym
{
T 43200 46950 5 10 0 0 0 0 1
device=RESISTOR
T 43000 46400 5 10 1 1 0 0 1
refdes=R6
T 43400 46400 5 10 1 1 0 0 1
footprint=0805
}
C 42700 46900 1 0 0 gnd-1.sym
C 42700 46400 1 0 0 gnd-1.sym
N 43700 46700 43900 46700 4
