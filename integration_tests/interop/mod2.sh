#!/usr/bin/env bash

set -ex

FC=gfortran
#FC=ifort

$FC -o mod2.o -c mod2.f90
$FC -o test_mod2.o -c test_mod2.f90
$FC -o gfort_interop.o -c gfort_interop.f90
$FC -o test_mod2 test_mod2.o mod2.o gfort_interop.o -L. -lmod1 -Wl,-rpath -Wl,.
./test_mod2
