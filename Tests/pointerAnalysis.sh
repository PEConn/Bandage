#! /bin/bash

opt_bandage="opt -load ../../Bandage_build/PointerAnalysis/LLVMBandageAnalysis.so"

clang -O0 -g3 -S -emit-llvm ${1%.c}.c -o ${1%.c}.bc
${opt_bandage} -S -pointer-analysis ${1%.c}.bc > ${1%.c}_ban.bc
