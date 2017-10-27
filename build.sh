#!/bin/bash

set -e
set -x

antlr4="java org.antlr.v4.Tool"

$antlr4 -Dlanguage=Python3 -no-listener -visitor fortran.g4
