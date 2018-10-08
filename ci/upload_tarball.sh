#!/bin/bash

set -e
set -x

if [[ $COMMIT_TAG == "" ]]; then
    # Development version
    dest_branch=${CI_COMMIT_REF_NAME}
    deploy_repo="https://gitlab-ci-token:${CI_JOB_TOKEN}@gitlab.com/lfortran/packages/testing.git"
else
    # Release version
    dest_branch="master"
    deploy_repo="https://gitlab-ci-token:${CI_JOB_TOKEN}@gitlab.com/lfortran/packages/releases.git"
fi

lfortran_version=$(<version)

D=`pwd`

mkdir $HOME/repos
cd $HOME/repos

git clone ${deploy_repo} lfortran-deploy
cd lfortran-deploy
cd $D
cp dist/lfortran-${lfortran_version}.tar.gz $HOME/repos/lfortran-deploy
cd $HOME/repos/lfortran-deploy

git config user.name "Deploy"
git config user.email "noreply@deploy"
COMMIT_MESSAGE="Deploying on $(date "+%Y-%m-%d %H:%M:%S")"

git add .
git commit -m "${COMMIT_MESSAGE}"

git push origin +master:$dest_branch
