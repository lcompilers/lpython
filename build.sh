#!/bin/bash

set -e
set -x

antlr4="java org.antlr.v4.Tool"

$antlr4 -Dlanguage=Python3 fortran.g4
