#! /bin/bash

set -e

opt="opt -load ../../Bandage_build/Basic/LLVMBandage.so"
llc="llc"
clang="clang"

time="/usr/bin/time"

r0="${1}-raw-o0"
r3="${1}-raw-o3"
b0="${1}-ban-o0"
b3="${1}-ban-o3"

${clang} -S -emit-llvm ${1}.c -o ${1}.bc

# Create the reference versions
${llc} ${1}.bc -o ${r0}.s
${clang} ${r0}.s -o ${r0} 

${llc} ${1}.bc -o ${r3}.s
${clang} ${r3}.s -o ${r3} 

# Create the bandage versions
${opt} -S -bandage ${1}.bc > ${b0}.bc
${llc} ${b0}.bc -o ${b0}.s
${clang} ${b0}.s -o ${b0} 

${opt} -S -bandage -O3 ${1}.bc > ${b3}.bc
${llc} ${b3}.bc -o ${b3}.s
${clang} ${b3}.s -o ${b3} 

rm ${r0}.s ${r3}.s ${b0}.s ${b3}.s > /dev/null
rm ${r0}.txt ${r3}.txt ${b0}.txt ${b3}.txt > /dev/null || true

echo `date` >> ${i}.log
for benchmark in $r0 $r3 $b0 $b3
do
  echo -n "Timing ${benchmark}"
  for i in {1..10}
  do
    ${time} -a -o ${benchmark}.txt ./${benchmark} > /dev/null
    echo -n " ${i}"
  done
  echo
done

awk '{total += $3} END {print "Raw -O0: " total/NR;}' ${r0}.txt >> ${1}.log
awk '{total += $3} END {print "Raw -O3: " total/NR;}' ${r3}.txt >> ${1}.log
awk '{total += $3} END {print "Ban -O0: " total/NR;}' ${b0}.txt >> ${1}.log
awk '{total += $3} END {print "Ban -O3: " total/NR;}' ${b3}.txt >> ${1}.log

tail -n 4 ${1}.log
