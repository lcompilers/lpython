#!/usr/bin/env bash

set -ex

lfortran_version=$1
export dest=lfortran-$lfortran_version
bash -x -o errexit ci/create_source_tarball0.sh
