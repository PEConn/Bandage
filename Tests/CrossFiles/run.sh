#!/bin/bash
set -e
set -v

opt_bandage="opt -load ../../../Bandage_build/Basic/LLVMBandage.so"
opt_listfuncs="opt -load ../../../Bandage_build/ListFuncs/LLVMListFuncs.so"
opt_softbound="opt -load ../../../Bandage_build/SoftBound/SoftBound.so"

rm functions.txt

clang -O0 -S -emit-llvm main.c -o main.bc
clang -O0 -S -emit-llvm def.c -o def.bc
${opt_listfuncs} -S -listfuncs -funcfile functions.txt main.bc >& /dev/null
${opt_listfuncs} -S -listfuncs -funcfile functions.txt def.bc >& /dev/null
#${opt_bandage} -S -bandage -funcfile functions.txt main.bc > main_ban.bc
#${opt_bandage} -S -bandage -funcfile functions.txt def.bc > def_ban.bc
${opt_softbound} -S -softbound -funcfile functions.txt main.bc > main_ban.bc 
${opt_softbound} -S -softbound -funcfile functions.txt def.bc > def_ban.bc 

clang -c main.bc -o main.o
clang -c def.bc -o def.o

clang main.o def.o

#cat functions.txt
