#!/bin/bash

set -e
set -v

opt="opt -load ../../../Bandage_build/Basic/LLVMBandage.so"
opt_soft="opt -load ../../../Bandage_build/SoftBound/SoftBound.so"
clang="clang"

#target="-c -target armv6--freebsd10.0-gnueabi"

base="LinkedList"
raw="${base}-raw"
ban="${base}-ban"
hash="${base}-hash"
mem="${base}-mem"

# Run basic
${clang} -O0 ${target} -S -emit-llvm LinkedList.c -o LinkedList.bc
${opt} -S LinkedList.bc > ${raw}.bc
${clang} ${target} -c ${raw}.bc -o ${raw}.o
rm ${raw}.bc
rm LinkedList.bc

# Run bandage
${clang} -O0 ${target} -S -emit-llvm LinkedList.c -o LinkedList.bc
${opt} -S -bandage LinkedList.bc > ${ban}.bc 2> /dev/null
${clang} ${target} -c ${ban}.bc -o ${ban}.o
rm ${ban}.bc
rm LinkedList.bc

# Run Softbound with hashtable
${clang} -O0 ${target} -D SOFTBOUND -S -emit-llvm LinkedList.c -o LinkedList.bc
${opt_soft} -S -softbound LinkedList.bc > ${hash}.bc 2> /dev/null
${clang} ${target} -c ${hash}.bc -o ${hash}.o
rm ${hash}.bc
rm LinkedList.bc

# Create the hashtable object file
${clang} ${target} -c ../../SoftBound/Headers/HashTable.c -o HashTable.o

# Run Softbound with memtable
${clang} -O0 ${target} -S -D SOFTBOUND -emit-llvm LinkedList.c -o LinkedList.bc
${opt_soft} -S -softbound LinkedList.bc > ${mem}.bc 2> /dev/null
${clang} ${target} -c ${mem}.bc -o ${mem}.o
rm ${mem}.bc
rm LinkedList.bc

# Create the memtable object file
${clang} ${target} -c ../../SoftBound/Headers/Memtable.c -o MemTable.o
