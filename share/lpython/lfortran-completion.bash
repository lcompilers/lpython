#/usr/bin/env bash

complete \
    -W "-h --help -S -c -o -v -E -I --version --cpp --fixed-form --show-prescan --show-tokens --show-ast --show-asr --with-intrinsic-modules --show-ast-f90 --no-color --indent --pass --show-llvm --show-cpp --show-stacktrace --time-report --static --backend --openmp fmt kernel mod pywrap" \
    -df \
    lfortran
