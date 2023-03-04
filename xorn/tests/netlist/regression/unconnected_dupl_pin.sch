v 20150930 2
C 0 0 1 0 0 component.sym
{
T 0 200 5 10 1 0 0 0 1
refdes=A
}
C 2000 0 1 0 0 component.sym
{
T 2000 200 5 10 1 0 0 0 1
refdes=A
}
C 0 500 1 0 0 component.sym
{
T 0 700 5 10 1 0 0 0 1
refdes=B
}
N 0 500 1000 500 4
{
T 1000 500 5 10 1 0 0 0 1
netname=b
}
C 2000 500 1 0 0 component.sym
{
T 2000 700 5 10 1 0 0 0 1
refdes=B
}
C 0 1000 1 0 0 component.sym
{
T 0 1200 5 10 1 0 0 0 1
refdes=C
}
N 0 1000 1000 1000 4
{
T 1000 1000 5 10 1 0 0 0 1
netname=c
}
C 2000 1000 1 0 0 component.sym
{
T 2000 1200 5 10 1 0 0 0 1
refdes=C
}
N 2000 1000 3000 1000 4
{
T 3000 1000 5 10 1 0 0 0 1
netname=c
}
C 0 1500 1 0 0 component.sym
{
T 0 1700 5 10 1 0 0 0 1
refdes=D
}
N 0 1500 1000 1500 4
C 1000 1500 1 0 0 power.sym
{
T 1000 1700 5 10 1 0 0 0 1
net=d:1
}
C 2000 1500 1 0 0 component.sym
{
T 2000 1700 5 10 1 0 0 0 1
refdes=D
}
C 0 2000 1 0 0 component.sym
{
T 0 2200 5 10 1 0 0 0 1
refdes=E
}
N 0 2000 1000 2000 4
C 1000 2000 1 0 0 power.sym
{
T 1000 2200 5 10 1 0 0 0 1
net=e:1
}
C 2000 2000 1 0 0 component.sym
{
T 2000 2200 5 10 1 0 0 0 1
refdes=E
}
N 2000 2000 3000 2000 4
C 3000 2000 1 0 0 power.sym
{
T 3000 2200 5 10 1 0 0 0 1
net=e:1
}
C 0 2500 1 0 0 component.sym
{
T 0 2700 5 10 1 0 0 0 1
refdes=F
}
N 0 2500 1000 2500 4
{
T 1000 2500 5 10 1 0 0 0 1
netname=f
}
C 2000 2500 1 0 0 component.sym
{
T 2000 2700 5 10 1 0 0 0 1
refdes=F
}
N 2000 2500 3000 2500 4
C 3000 2500 1 0 0 power.sym
{
T 3000 2700 5 10 1 0 0 0 1
net=f:1
}
