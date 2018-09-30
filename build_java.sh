#!/bin/bash

set -e
set -x

antlr4="java org.antlr.v4.Tool"
grun="java org.antlr.v4.gui.TestRig"

$antlr4 grammar/fortran.g4
javac fortran*.java

# Test:
$grun fortran module -tree examples/expr2.f90

# Visualize
#grun fortran root -gui examples/expr2.f90
