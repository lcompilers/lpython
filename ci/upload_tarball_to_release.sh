#!/usr/bin/env bash

set -ex

lpython_version=$(<version)

cd ./dist


RELEASE_ID=$(\
curl -s 'https://api.github.com/repos/lcompilers/lpython/releases/latest' | \
python -c "import sys, json; print(json.load(sys.stdin)['id'], end='')")

echo "RELEASE_ID=$RELEASE_ID"

curl --fail -L \
-X POST \
-H "Accept: application/vnd.github+json" \
-H "Authorization: Bearer $GITHUB_TOKEN" \
-H "X-GitHub-Api-Version: 2022-11-28" \
-H "Content-Type: application/octet-stream" \
"https://uploads.github.com/repos/lcompilers/lpython/releases/$RELEASE_ID/assets?name=lpython-${lpython_version}.tar.gz" \
--data-binary "@lpython-${lpython_version}.tar.gz"