v 20191008 2
C 40000 40000 0 0 0 title-B.sym
T 44200 50600 9 18 1 0 0 0 1
Below are various use/test cases for the SAB system.
T 40100 49700 9 12 1 0 0 0 2
This circuit fragment will be left untouched
because it has no sab-param attributes.
C 41200 48800 1 90 0 vcc-1.sym
C 42400 48900 1 90 0 gnd-1.sym
C 41200 48900 1 0 0 resistor-1.sym
{
T 41500 49300 5 10 0 0 0 0 1
device=RESISTOR
T 41400 49200 5 10 1 1 0 0 1
refdes=R1
T 41800 49200 5 10 1 1 0 0 1
value=1K
}
T 40100 47700 9 12 1 0 0 0 2
This device has no pins and therfore no connections.
It will be removed in the 'ab' context.
C 40600 46700 1 0 0 spice-title-1.sym
{
T 40700 47000 5 10 0 1 0 0 1
device=spice-title
T 40700 47100 5 10 1 1 0 0 1
refdes=A1
T 40700 46800 5 10 1 1 0 0 1
value=sab_example
T 40700 46600 5 10 1 0 0 0 1
sab-param=ab:discard
}
T 40100 45300 9 12 1 0 0 0 3
This device has pins but no connections.
It will be removed in the 'sim' context.
'doc' context present only for testing purposes.
T 45700 49800 9 12 1 0 0 0 2
This circuit fragment will have the middle gate
bypassed in the 'sim' context.
C 46300 48400 1 0 0 7400-1.sym
{
T 46800 49300 5 10 0 0 0 0 1
device=7400
T 46600 49300 5 10 1 1 0 0 1
refdes=U1
T 46800 50650 5 10 0 0 0 0 1
footprint=DIP14
T 46300 48400 5 10 0 0 0 0 1
slot=1
}
C 48500 48200 1 0 0 7400-1.sym
{
T 49000 49100 5 10 0 0 0 0 1
device=7400
T 48800 49100 5 10 1 1 0 0 1
refdes=U1
T 49000 50450 5 10 0 0 0 0 1
footprint=DIP14
T 49000 49300 5 10 0 0 0 0 1
slot=3
T 47800 49400 5 10 1 0 0 0 1
sab-param=sim:bypass:8,9,10
}
C 50400 48200 1 0 0 7400-1.sym
{
T 50900 49100 5 10 0 0 0 0 1
device=7400
T 50700 49100 5 10 1 1 0 0 1
refdes=U1
T 50900 50450 5 10 0 0 0 0 1
footprint=DIP14
T 50900 49300 5 10 0 0 0 0 1
slot=4
}
N 50400 48900 50400 48500 4
N 49800 48700 50400 48700 4
N 47600 48900 48500 48900 4
N 51700 48700 51700 47600 4
N 45700 47600 51700 47600 4
N 45700 47600 45700 49100 4
N 45700 49100 46300 49100 4
N 48500 48500 48200 48500 4
N 48200 48500 48200 47600 4
T 43500 41500 9 16 1 0 0 0 1
These are wrong and will produce warnings.
C 43700 40800 1 0 0 resistor-1.sym
{
T 44000 41200 5 10 0 0 0 0 1
device=RESISTOR
T 43900 41100 5 10 1 1 0 0 1
refdes=R2
T 43600 40500 5 10 1 0 0 0 1
sab-param=ab
}
C 45200 40800 1 0 0 resistor-1.sym
{
T 45500 41200 5 10 0 0 0 0 1
device=RESISTOR
T 45400 41100 5 10 1 1 0 0 1
refdes=R3
T 45100 40500 5 10 1 0 0 0 1
sab-param=ab:foo
}
C 46900 40800 1 0 0 resistor-1.sym
{
T 47200 41200 5 10 0 0 0 0 1
device=RESISTOR
T 47100 41100 5 10 1 1 0 0 1
refdes=R4
T 46800 40500 5 10 1 0 0 0 1
sab-param=ab:discard
T 46800 40300 5 10 1 0 0 0 1
sab-param=ab:bypass:1,2
}
C 40800 41000 1 0 0 7400-1.sym
{
T 41300 41900 5 10 0 0 0 0 1
device=7400
T 41100 41900 5 10 1 1 0 0 1
refdes=U1
T 41300 43250 5 10 0 0 0 0 1
footprint=DIP14
T 41300 42100 5 10 0 0 0 0 1
slot=2
T 40600 40800 5 10 1 0 0 0 1
sab-param=sim:discard
}
C 41000 44200 1 0 0 pwrjack-1.sym
{
T 41100 44700 5 10 0 0 0 0 1
device=PWRJACK
T 41000 44700 5 10 1 1 0 0 1
refdes=CONN1
T 41000 43900 5 10 1 0 0 0 1
sab-param=sim:discard
T 41000 43600 5 10 1 0 0 0 1
sab-param=doc:discard
}
T 40100 42400 9 12 1 0 0 0 4
This device has unconnected pins
and hidden pins, i.e. net attributes.
It also tests slotting. It will be discarded
in the 'sim' context.
