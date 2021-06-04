#!/bin/sh

# Exit when any command fails
set -e
set -x
cd "$GITHUB_WORKSPACE"
if [ "$1" != "docs" ]
then
    "$GITHUB_WORKSPACE/ci/master.bash" QUICK
    "$GITHUB_WORKSPACE/ci/master.bash" STATIC
else
    /autodoc.bash
fi
