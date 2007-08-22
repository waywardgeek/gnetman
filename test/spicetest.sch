v 20040111 1
N 25400 43600 25400 43700 4
N 25400 43700 26800 43700 4
{
T 25400 43700 5 10 1 1 0 0 1
netname=Vin
}
N 27700 43700 28600 43700 4
{
T 28200 43700 5 10 1 1 0 0 1
netname=Vout
}
N 28600 43700 28600 43400 4
N 28600 42500 28600 42100 4
N 28600 42100 25400 42100 4
N 25400 42400 25400 42100 4
N 27000 42100 27000 41700 4
C 26800 41300 1 0 0 vss.sym
C 25100 42400 1 0 0 vpwl-1.sym
{
T 25800 43050 5 10 1 1 0 0 1
refdes=V1
T 25800 42850 5 10 1 1 0 0 1
voltage=pwl 1ns 0 2ns 1
}
C 28400 42500 1 0 0 capacitor-1.sym
{
T 28900 43000 5 10 1 1 0 0 1
refdes=C1
T 28900 42800 5 10 1 1 0 0 1
value=100fF
}
C 26800 43500 1 0 0 resistor-1.sym
{
T 27200 43500 5 10 1 1 0 5 1
refdes=R1
T 27200 43900 5 10 1 1 0 3 1
value=10K
}
T 26700 44500 9 10 1 0 0 0 2
^.tran 10ns
Vss vss 0 0