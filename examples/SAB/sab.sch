v 20191008 2
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
T 40100 44600 9 12 1 0 0 0 7
This device has pins but no connections.
It will be removed in the 'sim' context.
It will be the second component processed
in the 'sim' context because I have intentionally
skip 2 to show that not all order indices
need to be included, i.e. there can be gaps.
'doc' context present only for testing purposes.
T 46600 49800 9 12 1 0 0 0 2
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
slot=2
T 47800 49400 5 10 1 0 0 0 1
sab-param=sim:bypass:4,5,6 as ring
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
T 52400 44100 9 16 1 0 0 0 1
These are wrong and will produce warnings.
C 52800 43500 1 0 0 resistor-1.sym
{
T 53100 43900 5 10 0 0 0 0 1
device=RESISTOR
T 53000 43800 5 10 1 1 0 0 1
refdes=R10
T 52700 43200 5 10 1 0 0 0 1
sab-param=err
}
C 54400 43500 1 0 0 resistor-1.sym
{
T 54700 43900 5 10 0 0 0 0 1
device=RESISTOR
T 54600 43800 5 10 1 1 0 0 1
refdes=R12
T 54300 43200 5 10 1 0 0 0 1
sab-param=err:foo
}
C 56200 43500 1 0 0 resistor-1.sym
{
T 56500 43900 5 10 0 0 0 0 1
device=RESISTOR
T 56400 43800 5 10 1 1 0 0 1
refdes=R14
T 56100 43200 5 10 1 0 0 0 1
sab-param=err:discard
T 56100 43000 5 10 1 0 0 0 1
sab-param=err:bypass:1,2
}
C 40800 40500 1 0 0 7400-1.sym
{
T 41300 41400 5 10 0 0 0 0 1
device=7400
T 41100 41400 5 10 1 1 0 0 1
refdes=U1
T 41300 42750 5 10 0 0 0 0 1
footprint=DIP14
T 41300 41600 5 10 0 0 0 0 1
slot=3
T 40600 40300 5 10 1 0 0 0 1
sab-param=sim:#1:discard
}
C 41000 43700 1 0 0 pwrjack-1.sym
{
T 41100 44200 5 10 0 0 0 0 1
device=PWRJACK
T 41000 44200 5 10 1 1 0 0 1
refdes=CONN1
T 41000 43400 5 10 1 0 0 0 1
sab-param=sim:#3:discard
T 41000 43100 5 10 1 0 0 0 1
sab-param=doc:discard
}
T 40100 41700 9 12 1 0 0 0 5
This device has unconnected pins
and hidden pins, i.e. net attributes.
It also tests slotting. It will be discarded
in the 'sim' context. It will be the first
component processed.
C 46800 45800 1 0 0 resistor-1.sym
{
T 47100 46200 5 10 0 0 0 0 1
device=RESISTOR
T 47000 46100 5 10 1 1 0 0 1
refdes=R2
}
C 46800 45400 1 0 0 resistor-1.sym
{
T 47100 45800 5 10 0 0 0 0 1
device=RESISTOR
T 47000 45100 5 10 1 1 0 0 1
refdes=R3
}
C 49100 45800 1 0 0 resistor-1.sym
{
T 49400 46200 5 10 0 0 0 0 1
device=RESISTOR
T 49300 46100 5 10 1 1 0 0 1
refdes=R5
}
C 49100 45000 1 0 0 resistor-1.sym
{
T 49400 45400 5 10 0 0 0 0 1
device=RESISTOR
T 49300 45300 5 10 1 1 0 0 1
refdes=R7
}
C 49100 44200 1 0 0 resistor-1.sym
{
T 49400 44600 5 10 0 0 0 0 1
device=RESISTOR
T 49300 44500 5 10 1 1 0 0 1
refdes=R8
}
N 49100 44700 49100 44300 4
C 47700 42900 1 0 0 header16-1.sym
{
T 47750 41850 5 10 0 1 0 0 1
device=HEADER16
T 48300 46200 5 10 1 1 0 0 1
refdes=J1
T 45500 42600 5 10 1 0 0 0 1
sab-param=ab:bypass:1,2;3,4 as blocked;5,6;9,8 as forked1;13,14 as forked2
T 47500 42300 5 10 1 0 0 0 1
sab-param=sim:discard
}
C 49100 43400 1 0 0 resistor-1.sym
{
T 49400 43800 5 10 0 0 0 0 1
device=RESISTOR
T 49300 43700 5 10 1 1 0 0 1
refdes=R9
}
C 46800 43800 1 0 0 resistor-1.sym
{
T 47100 44200 5 10 0 0 0 0 1
device=RESISTOR
T 47000 44100 5 10 1 1 0 0 1
refdes=R4
}
N 47700 43500 47700 43900 4
T 47100 46500 9 12 1 0 0 0 2
Various bypass combinations
looking for trouble makers
C 46500 46000 1 270 0 gnd-1.sym
C 46500 45400 1 270 1 gnd-1.sym
C 46500 43800 1 270 1 gnd-1.sym
C 50300 43400 1 90 0 gnd-1.sym
C 50300 44200 1 90 0 gnd-1.sym
C 50300 45000 1 90 0 gnd-1.sym
C 50300 45800 1 90 0 gnd-1.sym
C 46800 43000 1 0 0 resistor-1.sym
{
T 47100 43400 5 10 0 0 0 0 1
device=RESISTOR
T 47000 43300 5 10 1 1 0 0 1
refdes=R6
}
C 46500 43000 1 270 1 gnd-1.sym
C 52800 42500 1 0 0 resistor-1.sym
{
T 53100 42900 5 10 0 0 0 0 1
device=RESISTOR
T 53000 42800 5 10 1 1 0 0 1
refdes=R11
T 52700 42200 5 10 1 0 0 0 1
sab-param=err:bypass:1,w
}
C 55500 42500 1 0 0 resistor-1.sym
{
T 55800 42900 5 10 0 0 0 0 1
device=RESISTOR
T 55700 42800 5 10 1 1 0 0 1
refdes=R15
T 55400 42200 5 10 1 0 0 0 1
sab-param=err:bypass:1
}
C 52700 41400 1 0 0 resistor-1.sym
{
T 53000 41800 5 10 0 0 0 0 1
device=RESISTOR
T 52900 41700 5 10 1 1 0 0 1
refdes=R13
T 52700 41100 5 10 1 0 0 0 1
sab-param=err:bypass:1,10
}
C 53000 48000 1 0 0 7402-1.sym
{
T 53600 48900 5 10 0 0 0 0 1
device=7402
T 53300 48900 5 10 1 1 0 0 1
refdes=U2
T 53600 50300 5 10 0 0 0 0 1
footprint=DIP14
T 53000 48000 5 10 0 0 0 0 1
slot=1
}
C 55100 48000 1 0 0 7402-1.sym
{
T 55700 48900 5 10 0 0 0 0 1
device=7402
T 55400 48900 5 10 1 1 0 0 1
refdes=U2
T 55700 50300 5 10 0 0 0 0 1
footprint=DIP14
T 54400 47800 5 10 1 0 0 0 1
sab-param=sim:exec:modify_refdes:A,PCB,True
T 55100 48000 5 10 0 0 0 0 1
slot=2
}
T 53000 49300 9 12 1 0 0 0 4
Here are two gates from the same package.
The second one has a call to an external
script in the 'sim' context. See modify_refdes.py
for a description of the parameters.
C 55800 41500 1 0 0 resistor-1.sym
{
T 56100 41900 5 10 0 0 0 0 1
device=RESISTOR
T 56000 41800 5 10 1 1 0 0 1
refdes=R16
T 55700 41400 5 10 1 0 0 0 1
sab-param=err:#2:discard
}
C 55800 40900 1 0 0 resistor-1.sym
{
T 56100 41300 5 10 0 0 0 0 1
device=RESISTOR
T 56000 41200 5 10 1 1 0 0 1
refdes=R17
T 55700 40800 5 10 1 0 0 0 1
sab-param=err:#2:discard
}
