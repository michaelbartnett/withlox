#!/usr/bin/env bash

# run from root of project
SELF_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
cd "${SELF_DIR}/.."

set -e

wd=$(pwd)
mkdir -p build/makefiles
pushd build/makefiles

if [ ! -f Makefile ]; then
    echo running CMake...
    cmake -G "Unix Makefiles" "$wd"
fi

echo running make
make


popd
