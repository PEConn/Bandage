#! /bin/bash

set -e

opt_bandage="opt -load ../../Bandage_build/Basic/LLVMBandage.so"

clang -O0 -S -emit-llvm ${1%.c}.c -o ${1%.c}.bc
gdb --args ${opt_bandage} -S -bandage ${1%.c}.bc
