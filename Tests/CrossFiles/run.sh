#! /bin/bash

opt_bandage="opt -load ../../../Bandage_build/Basic/LLVMBandage.so"
opt_listfuncs="opt -load ../../../Bandage_build/ListFuncs/LLVMListFuncs.so"

rm functions.txt

clang -O0 -S -emit-llvm main.c -o main.bc
clang -O0 -S -emit-llvm def.c -o def.bc
${opt_listfuncs} -S -listfuncs -funcfile functions.txt main.bc > /dev/null
${opt_listfuncs} -S -listfuncs -funcfile functions.txt def.bc > /dev/null
${opt_bandage} -S -bandage -funcfile functions.txt main.bc > main_ban.bc
${opt_bandage} -S -bandage -funcfile functions.txt def.bc > def_ban.bc

cat functions.txt
