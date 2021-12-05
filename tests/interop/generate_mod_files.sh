#!/usr/bin/env bash

set -ex

gfortran-7 -c mod1.f90 -o /tmp/a.o
mv mod1.mod mod1-14.mod
cp mod1-14.mod mod1-14-unpacked.mod.gz
gzip -df mod1-14-unpacked.mod.gz

gfortran-10 -c mod1.f90 -o /tmp/a.o
mv mod1.mod mod1-15.mod
cp mod1-15.mod mod1-15-unpacked.mod.gz
gzip -df mod1-15-unpacked.mod.gz
