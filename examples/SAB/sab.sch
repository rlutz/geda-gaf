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
T 40100 45300 9 12 1 0 0 0 2
This device has pins but no connections.
It will be removed in the 'sim' context.
C 40700 44400 1 0 0 pwrjack-1.sym
{
T 40800 44900 5 10 0 0 0 0 1
device=PWRJACK
T 40700 44900 5 10 1 1 0 0 1
refdes=CONN1
T 40700 44200 5 10 1 0 0 0 1
sab-param=sim:discard
}
T 40100 42900 9 12 1 0 0 0 2
This circuit fragment will have the middle gate
bypassed in the 'sim'context.
C 41300 41300 1 0 0 7400-1.sym
{
T 41800 42200 5 10 0 0 0 0 1
device=7400
T 41600 42200 5 10 1 1 0 0 1
refdes=U1
T 41800 43550 5 10 0 0 0 0 1
footprint=DIP14
T 41300 41300 5 10 0 0 0 0 1
slot=1
}
C 43500 41100 1 0 0 7400-1.sym
{
T 44000 42000 5 10 0 0 0 0 1
device=7400
T 43800 42000 5 10 1 1 0 0 1
refdes=U1
T 44000 43350 5 10 0 0 0 0 1
footprint=DIP14
T 44000 42200 5 10 0 0 0 0 1
slot=2
T 42800 42300 5 10 1 0 0 0 1
sab-param=sim:bypass:4,5,6
}
C 45400 41100 1 0 0 7400-1.sym
{
T 45900 42000 5 10 0 0 0 0 1
device=7400
T 45700 42000 5 10 1 1 0 0 1
refdes=U1
T 45900 43350 5 10 0 0 0 0 1
footprint=DIP14
T 45900 42200 5 10 0 0 0 0 1
slot=3
}
N 45400 41800 45400 41400 4
N 44800 41600 45400 41600 4
N 42600 41800 43500 41800 4
N 46700 41600 46700 40500 4
N 40700 40500 46700 40500 4
N 40700 40500 40700 42000 4
N 40700 42000 41300 42000 4
N 43500 41400 43200 41400 4
N 43200 41400 43200 40500 4
