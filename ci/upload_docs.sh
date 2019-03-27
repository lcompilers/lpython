#!/bin/bash

set -e
set -x

if [[ $CI_COMMIT_REF_NAME != "master" ]]; then
    # Development version
    dest_branch=${CI_COMMIT_REF_NAME}
    deploy_repo="git@gitlab.com:lfortran/web/docs.lfortran.org-testing.git"
else
    # Release version
    dest_branch="master"
    deploy_repo="git@github.com:lfortran/docs.lfortran.org.git"
fi

mkdir ~/.ssh
chmod 700 ~/.ssh
ssh-keyscan gitlab.com >> ~/.ssh/known_hosts
ssh-keyscan github.com >> ~/.ssh/known_hosts

eval "$(ssh-agent -s)"

set +x
if [[ "${SSH_PRIVATE_KEY_DOCS}" == "" ]]; then
    echo "Note: SSH_PRIVATE_KEY_DOCS is empty, skipping..."
    exit 0
fi
# Generate the private/public key pair using:
#
#     ssh-keygen -f deploy_key -N ""
#
# then set the $SSH_PRIVATE_KEY_DOCS environment variable in the GitLab-CI to
# the base64 encoded private key:
#
#     cat deploy_key | base64 -w0
#
# and add the public key `deploy_key.pub` into the target git repository (with
# write permissions).

ssh-add <(echo "$SSH_PRIVATE_KEY_DOCS" | base64 -d)
set -x


D=`pwd`

mkdir $HOME/repos
cd $HOME/repos

git clone ${deploy_repo} docs-deploy
cd docs-deploy
rm -rf docs
mkdir docs
echo "docs.lfortran.org" > docs/CNAME
cp -r $D/site/* docs/

git config user.name "Deploy"
git config user.email "noreply@deploy"
COMMIT_MESSAGE="Deploying on $(date "+%Y-%m-%d %H:%M:%S")"

git add .
git commit -m "${COMMIT_MESSAGE}"

git push origin +master:$dest_branch
