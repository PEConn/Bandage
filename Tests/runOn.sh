#! /bin/bash

opt="/pool/users/pc424/llvm_build/bin/opt -load ../../Bandage_build/Basic/LLVMBandage.so"
llc="/pool/users/pc424/llvm_build/bin/llc"
clang="/pool/users/pc424/llvm_build/bin/clang"

echo -en "\033[33m"
${clang} -S -emit-llvm ${1%.c}.c -o ${1%.c}.bc
echo -en "\033[0m"
${opt} -S -bandage ${1%.c}.bc > ${1%.c}_ban.bc
echo -en "\033[33m"
${clang} -S -emit-llvm ${1%.c}.c -o ${1%.c}.bc
${llc} ${1%.c}_ban.bc -o ${1%.c}_ban.s
${clang} ${1%.c}_ban.s -o ${1%.c} 
echo -en "\033[0m"

./${1%.c}
