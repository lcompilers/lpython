#!/bin/bash
#
# Compile the latest master and a given MR in Release mode from scratch.
#
# Example:
#
# CC=gcc-10 CXX=g++-10 ./bench.sh 448
# bench/master/src/bin/parse
# bench/mr/src/bin/parse
# bench/master/src/bin/parse2
# bench/mr/src/bin/parse2

set -ex

MR=$1
if [[ $MR == "" ]]; then
    echo "Specify the MR number as an argument"
    exit 1
fi

echo "Benchmarking the latest master against the MR !$1"

rm -rf bench
mkdir bench
cd bench

git clone https://gitlab.com/lfortran/lfortran
cd lfortran
./build0.sh
cd ..

mkdir master
cd master
#cmake -DCMAKE_CXX_FLAGS_RELEASE="-Wall -Wextra -O3 -funroll-loops -DNDEBUG" ../lfortran
cmake -DWITH_FMT=yes ../lfortran
make -j
cd ..

cd lfortran
git clean -dfx
git fetch origin merge-requests/$MR/head:mr-origin-$MR
git checkout mr-origin-$MR
./build0.sh
cd ..

mkdir mr
cd mr
#cmake -DCMAKE_CXX_FLAGS_RELEASE="-Wall -Wextra -O3 -funroll-loops -DNDEBUG" ../lfortran
cmake -DWITH_FMT=yes ../lfortran
make -j
cd ..
