#!/bin/bash

set -e
set -x

if [[ $CI_COMMIT_TAG == "" ]]; then
    # Development version
    export TWINE_REPOSITORY_URL="https://test.pypi.org/legacy/"
else
    # Release version
    export TWINE_REPOSITORY_URL="https://upload.pypi.org/legacy/"
fi

lfortran_version=$(<version)

mkdir ~/.ssh
chmod 700 ~/.ssh
ssh-keyscan gitlab.com >> ~/.ssh/known_hosts
ssh-keyscan github.com >> ~/.ssh/known_hosts

eval "$(ssh-agent -s)"

set +x
export TWINE_USERNAME=${PYPI_USERNAME}
export TWINE_PASSWORD=${PYPI_PASSWORD}
set -x


twine upload dist/lfortran-${lfortran_version}.tar.gz
