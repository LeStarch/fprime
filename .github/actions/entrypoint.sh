#!/bin/sh

# Exit when any command fails
set -e
set -x
cd "$GITHUB_WORKSPACE"
 [ "$GITHUB_WORKFLOW" != "Autodocs" ]

    "$GITHUB_WORKSPACE/ci/master.bash" QUICK

    /autodoc.bash

