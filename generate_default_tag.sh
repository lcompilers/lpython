#!/usr/bin/env bash

# Run this script when your local repo 
# #doesn't automatically fetch tags from upstream
# It allows local development with a mocked tag
echo "Generating default tag..."
set -ex
version="v0.0.0=0"
git tag $version

