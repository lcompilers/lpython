#!/bin/bash

set -e
set -x

antlr4="java org.antlr.v4.Tool"
grun="java org.antlr.v4.gui.TestRig"

$antlr4 fortran.g4
javac fortran*.java

# Test:
$grun fortran module -tree examples/m1.f90
echo
$grun fortran module -tree examples/subroutine1.f90
echo
$grun fortran module -tree examples/expr1.f90

# Visualize
#grun fortran root -gui examples/m1.f90
