v 20030901
C 70600 75200 1 0 0 capacitor-1.sym
{
T 71100 75700 5 10 1 1 0 0
refdes=C?
T 71100 75500 5 10 1 1 0 0
value=1uF
}
C 73300 75500 1 0 0 idc-1.sym
{
T 74000 76150 5 10 1 1 0 0
refdes=I?
T 74000 75950 5 10 1 1 0 0
current=DC 1mA
}
C 76000 75900 1 0 0 inductor-1.sym
{
T 76400 75900 5 10 1 1 0 0
refdes=L?
T 76400 76200 5 10 1 1 0 0
inductance=1H
}
C 81200 75600 1 0 0 nmos.sym
{
T 82000 76200 5 10 1 1 0 0
refdes=M?
T 82000 75800 5 6 1 0 0 0
w=1u
}
C 78500 75500 1 0 0 nmos4.sym
{
T 79300 76100 5 10 1 1 0 0
refdes=M?
T 79300 75700 5 6 1 0 0 0
w=1u
}
C 84000 75600 1 0 0 pmos.sym
{
T 84800 76200 5 10 1 1 0 0
refdes=M?
T 84800 75800 5 6 1 0 0 0
w=1u
}
C 86000 75600 1 0 0 pmos4.sym
{
T 86800 76200 5 10 1 1 0 0
refdes=M?
T 86800 75800 5 6 1 0 0 0
w=1u
}
C 70500 72900 1 0 0 resistor-1.sym
{
T 70900 72900 5 10 1 1 0 5
refdes=R?
T 70900 73300 5 10 1 1 0 3
value=1K
}
C 72900 72700 1 0 0 vccs-1.sym
{
T 73500 73600 5 12 1 1 0 0
refdes=G?
T 73500 73100 5 10 1 1 0 0
transconductance=1
}
C 75900 72700 1 0 0 vcvs-1.sym
{
T 76500 73600 5 12 1 1 0 0
refdes=E?
T 76600 73000 5 10 1 1 0 0
gain=1
}
C 78500 72600 1 0 0 vdc-1.sym
{
T 79200 73250 5 10 1 1 0 0
refdes=V?
T 79200 73050 5 10 1 1 0 0
voltage=DC 1V
}
C 81300 72600 1 0 0 vpulse-1.sym
{
T 82000 73250 5 10 1 1 0 0
refdes=V?
T 82000 73050 5 10 1 1 0 0
voltage=pulse 0 1 10n 10n 100n 1u 2u
}
C 86100 72700 1 0 0 vpwl-1.sym
{
T 86800 73350 5 10 1 1 0 0
refdes=V?
T 86800 73150 5 10 1 1 0 0
voltage=pwl 100n 0 200n 1
}
N 70800 76100 70800 77200 4
N 70800 77200 86600 77200 4
N 86600 77200 86600 76400 4
N 84600 76300 84600 77200 4
N 81800 76400 81800 77200 4
N 79100 76300 79100 77200 4
N 76900 76000 76900 77200 4
N 73600 76700 73600 77200 4
N 70500 73100 70500 71600 4
N 70500 71600 86400 71600 4
N 86400 72700 86400 71600 4
N 81600 72600 81600 71600 4
N 71400 73100 71400 74300 4
N 71400 74300 70800 74300 4
N 70800 74300 70800 75200 4
N 73600 75500 73600 74700 4
N 73600 74700 72900 74700 4
N 72900 74700 72900 73400 4
N 72900 72800 72900 71600 4
N 75900 72800 75900 71600 4
N 74400 72800 74400 71600 4
N 74400 73400 75900 73400 4
N 77400 72800 77400 71600 4
N 77800 71600 77800 71200 4
N 77400 73400 77400 74800 4
N 76000 74800 76000 76000 4
N 76000 74800 78500 74800 4
N 79100 75500 79100 73800 4
N 79100 73800 78800 73800 4
N 81600 73800 81600 75100 4
N 81800 75100 81800 75600 4
N 81200 76000 80600 76000 4
N 80600 76000 80600 75200 4
N 80600 75200 79100 75200 4
N 79100 75900 80000 75900 4
N 80000 75900 80000 77200 4
N 78500 74800 78500 75900 4
N 84600 75600 84600 71600 4
N 84000 76000 83600 76000 4
N 83600 76000 83600 75100 4
N 81600 75100 83600 75100 4
N 86600 75600 86600 73900 4
N 86600 73900 86400 73900 4
N 86000 75900 86000 74900 4
N 86000 74900 84600 74900 4
C 77600 70800 1 0 0 vss.sym
C 78000 77800 1 0 0 vdd.sym
N 78200 77800 78200 77200 4
N 78800 71600 78800 72600 4