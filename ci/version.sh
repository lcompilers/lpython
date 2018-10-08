#!/bin/bash

set -e
set -x

if [[ $CI_COMMIT_TAG == "" ]]; then
    # The commit is not tagged, use the first 7 chars of git commit hash
    version=${CI_COMMIT_SHA:0:7}
else
    # The commit is tagged, convert tag version properly:v1.0.1 -> "1.0.1"
    version=${CI_COMMIT_TAG:1}
fi

echo ${version} > ../version
