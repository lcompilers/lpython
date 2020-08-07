#!/bin/bash

set -ex

cmake -E make_directory $dest
cmake -E copy src cmake examples CMakeLists.txt README.md LICENSE version $dest
cmake -E make_directory dist
cmake -E tar cfz dist/$dest.tar.gz $dest
cmake -E remove_directory $dest
