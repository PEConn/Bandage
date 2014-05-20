#! /bin/bash

set -e

opt_bandage="opt -load ../../../Bandage_build/Basic/LLVMBandage.so"
opt_softbound="opt -load ../../../Bandage_build/SoftBound/SoftBound.so"

clang -O0 -S -emit-llvm ${1%.c}.c -o ${1%.c}.bc
${opt_bandage} -p -S -bandage ${1%.c}.bc > ${1%.c}_ban.bc
#${opt_softbound} -S -softbound ${1%.c}.bc > ${1%.c}_ban.bc
llc ${1%.c}_ban.bc -o ${1%.c}_ban.s
clang ${1%.c}_ban.s -o ${1%.c} 
