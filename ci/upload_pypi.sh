#!/bin/bash

set -e
set -x

if [[ $CI_COMMIT_TAG == "" ]]; then
    # Development version
    TWINE_REPOSITORY_URL="https://test.pypi.org/legacy/"
else
    # Release version
    TWINE_REPOSITORY_URL="https://upload.pypi.org/legacy/"
fi

lfortran_version=$(<version)

mkdir ~/.ssh
chmod 700 ~/.ssh
ssh-keyscan gitlab.com >> ~/.ssh/known_hosts
ssh-keyscan github.com >> ~/.ssh/known_hosts

eval "$(ssh-agent -s)"

set +x
TWINE_USERNAME=${PYPI_USERNAME}
TWINE_PASSWORD=${PYPI_PASSWORD}
set -x


twine upload dist/lfortran-${lfortran_version}.tar.gz
