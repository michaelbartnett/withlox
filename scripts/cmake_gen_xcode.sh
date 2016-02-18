#!/usr/bin/env bash

set -e

wd=$(pwd)
mkdir -p build/xcode
pushd build/xcode

cmake -G Xcode "$wd"

open jsoneditor.xcodeproj

popd
