#!/usr/bin/env bash

set -e

./scripts/build_makefiles.sh

prefix=build/makefiles

run_args=""

if [ -f ./run_args.txt ]; then
    run_args=$(cat run_args.txt) #|tr -d "\n"
    echo run_args=$run_args
else
    echo no run args!
fi

project_name=$(cat $prefix/CMakeCache.txt | grep CMAKE_PROJECT_NAME:STATIC | sed s/CMAKE_PROJECT_NAME:STATIC=//g)
exe_name=$prefix/$project_name

lldb -o 'b main' -o 'run' $exe_name $run_args
