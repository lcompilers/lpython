#!/usr/bin/env bash

set -ex

lpython_version=$1
export dest=lpython-$lpython_version
bash -x -o errexit ci/create_source_tarball0.sh
