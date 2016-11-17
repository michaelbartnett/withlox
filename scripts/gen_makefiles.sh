#!/usr/bin/env bash

# run from root of project
SELF_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
cd "${SELF_DIR}/.."

set -e

wd=$(pwd)
mkdir -p build/makefiles
pushd build/makefiles

echo running CMake...
cmake -G "Unix Makefiles" "$wd"

popd
