#!/usr/bin/env bash

set -e
set -x

if [[ $CI_COMMIT_REF_NAME != "master" ]]; then
    echo "Not on master, skipping mirroring"
    exit 0
else
    echo "On master, mirroring to GitHub"
fi

mkdir ~/.ssh
chmod 700 ~/.ssh
ssh-keyscan github.com >> ~/.ssh/known_hosts

eval "$(ssh-agent -s)"

set +x
if [[ "${SSH_PRIVATE_KEY_MIRROR}" == "" ]]; then
    echo "Error: SSH_PRIVATE_KEY_MIRROR is empty."
    exit 1
fi
# Generate the private/public key pair using:
#
#     ssh-keygen -f deploy_key -N ""
#
# then set the $SSH_PRIVATE_KEY_MIRROR environment variable in the GitLab-CI to
# the base64 encoded private key:
#
#     cat deploy_key | base64 -w0
#
# and add the public key `deploy_key.pub` into the target git repository (with
# write permissions).

ssh-add <(echo "$SSH_PRIVATE_KEY_MIRROR" | base64 -d)
set -x

pwd
git show-ref
git remote -v
git push git@github.com:lfortran/lfortran.git +origin/master:master --tags
