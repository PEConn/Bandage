#! /bin/bash

set -e

opt_bandage="opt -load ../../../Bandage_build/Basic/LLVMBandage.so"

clang -O0 -S -emit-llvm ${1%.c}.c -o ${1%.c}.bc
${opt_bandage} -p -S -bandage ${1%.c}.bc > ${1%.c}_ban.bc
llc ${1%.c}_ban.bc -o ${1%.c}_ban.s
clang ${1%.c}_ban.s -o ${1%.c} 
