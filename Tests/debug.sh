#! /bin/bash

set -e

opt_bandage="opt -load ../../Bandage_build/Basic/LLVMBandage.so"
opt_softbound="opt -load ../../Bandage_build/SoftBound/SoftBound.so"

clang -O0 -S -emit-llvm ${1%.c}.c -o ${1%.c}.bc
#gdb --args ${opt_bandage} -S -bandage ${1%.c}.bc
gdb --args ${opt_softbound} -S -softbound ${1%.c}.bc
