#!/usr/bin/env bash

set -e

wd=$(pwd)
mkdir -p build/makefiles
pushd build/makefiles

# if [ ! -f build/makefiles/Makefile ]; then
echo running CMake...
    cmake -G "Unix Makefiles" "$wd"
# fi

echo running make
make

popd
