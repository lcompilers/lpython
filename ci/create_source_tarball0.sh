#!/usr/bin/env bash

set -ex

cmake -E make_directory $dest

# Copy Directories:
cmake -E copy_directory src $dest/src
cmake -E copy_directory share $dest/share
cmake -E copy_directory cmake $dest/cmake
cmake -E copy_directory examples $dest/examples
cmake -E copy_directory lfortran/src/libasr $dest/lfortran/src/libasr

# Copy Files:
cmake -E copy CMakeLists.txt README.md LICENSE lp_version $dest

# Create the tarball
cmake -E make_directory dist
cmake -E tar cfz dist/$dest.tar.gz $dest
cmake -E remove_directory $dest
