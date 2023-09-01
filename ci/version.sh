#!/usr/bin/env bash

# This script extracts the project's current version from git using
# `git describe`, which determines the version based on the latest tag, such as:
#
#     $ git describe --tags --dirty
#     v0.6.0-37-g3878937f-dirty
#
# Each tag starts with "v", so we strip the "v", and the final version becomes:
#
#     0.6.0-37-g3878937f-dirty
#

git_describe_output=$(git describe --tags --dirty 2>&1)
exit_code=$?

set -ex 
if [ $exit_code -ne 0 ]; then
  echo "There were errors while getting the tags. Defaulting to 0.0.0-0"
  version="0.0.0-0"
  echo $version > version
  exit 1
else
  echo "Tags captured successfully."
  version="${git_describe_output:1}"
  echo $version > version
  exit 0
fi