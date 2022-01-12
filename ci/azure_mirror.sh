#!/usr/bin/env bash

set -e
set -x

mkdir ~/.ssh
chmod 700 ~/.ssh
ssh-keyscan ssh.dev.azure.com >> ~/.ssh/known_hosts

eval "$(ssh-agent -s)"

set +x
if [[ "${SSH_PRIVATE_KEY_AZURE}" == "" ]]; then
    echo "Error: SSH_PRIVATE_KEY_AZURE is empty."
    exit 1
fi
# Generate the private/public key pair using:
#
#     ssh-keygen -f deploy_key -N ""
#
# then set the $SSH_PRIVATE_KEY_AZURE environment variable in the GitLab-CI to
# the base64 encoded private key:
#
#     cat deploy_key | base64 -w0
#
# and add the public key `deploy_key.pub` into the target git repository (with
# write permissions).

ssh-add <(echo "$SSH_PRIVATE_KEY_AZURE" | base64 -d)
set -x

pwd
bname="branch-$CI_COMMIT_REF_NAME"
git branch -D ${bname} || echo ok
git checkout $CI_COMMIT_SHA
git checkout -b ${bname}
git show-ref
git remote -v
git push git@ssh.dev.azure.com:v3/lfortran/lfortran/lfortran +${bname}:${bname} --tags
