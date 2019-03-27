#!/bin/bash

set -e
set -x

if [[ $CI_COMMIT_TAG == "" ]]; then
    # Development version
    dest_branch=${CI_COMMIT_REF_NAME}
    deploy_repo="git@gitlab.com:lfortran/packages/testing.git"
else
    # Release version
    dest_branch="master"
    deploy_repo="git@gitlab.com:lfortran/packages/releases.git"
fi

lfortran_version=$(<version)

mkdir ~/.ssh
chmod 700 ~/.ssh
ssh-keyscan gitlab.com >> ~/.ssh/known_hosts

eval "$(ssh-agent -s)"

set +x
if [[ "${SSH_PRIVATE_KEY}" == "" ]]; then
    echo "Note: SSH_PRIVATE_KEY is empty, skipping..."
    exit 0
fi
# Generate the private/public key pair using:
#
#     ssh-keygen -f deploy_key -N ""
#
# then set the $SSH_PRIVATE_KEY environment variable in the GitLab-CI to the
# base64 encoded private key:
#
#     cat deploy_key | base64 -w0
#
# and add the public key `deploy_key.pub` into the target git repository (with
# write permissions).

ssh-add <(echo "$SSH_PRIVATE_KEY" | base64 -d)
set -x


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
