#!/usr/bin/env bash

# run from root of project
SELF_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
cd "${SELF_DIR}/.."

cloc --exclude-list-file=cloc_exclude.txt --exclude-lang=Markdown,JSON,INI,Lisp,CMake,Bourne\ Shell .
