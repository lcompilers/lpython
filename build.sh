#!/bin/bash

set -e
set -x

antlr4="java org.antlr.v4.Tool"
grun="java org.antlr.v4.gui.TestRig"

$antlr4 fortran.g4
javac fortran*.java

# Test:
$grun fortran module -tree examples/m1.f90
