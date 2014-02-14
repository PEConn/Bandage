#! /bin/bash

opt="/pool/users/pc424/llvm_build/bin/opt -load ../../Bandage_build/Basic/LLVMBandage.so"
llc="/pool/users/pc424/llvm_build/bin/llc"
clang="/pool/users/pc424/llvm_build/bin/clang"

time="/usr/bin/time"

${clang} -S -emit-llvm ${1}.c -o ${1}.bc
${opt} -S -bandage ${1}.bc > ${1}_ban.bc
${llc} ${1}_ban.bc -o ${1}_ban.s
${clang} ${1}_ban.s -o ${1}_ban 

${clang} ${1}.c -o ${1}

rm ${1}.bc ${1}_ban.bc ${1}_ban.s > /dev/null
rm ${1}-times.txt ${1}-ban-times.txt > /dev/null

for i in {1..10}
do
	${time} -a -o ${1}-times.txt ./${1} > /dev/null
	${time} -a -o ${1}-ban-times.txt ./${1}_ban > /dev/null
done

echo `date` >> ArrayWrites.log
awk '{total += $3} END {print "Raw: " total/NR;}' ArrayWrites-times.txt >> ArrayWrites.log
awk '{total += $3} END {print "Ban: " total/NR;}' ArrayWrites-ban-times.txt >> ArrayWrites.log

tail -n 2 ArrayWrites.log
