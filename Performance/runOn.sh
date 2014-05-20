#! /bin/bash

set -e

opt="opt -load ../../Bandage_build/Basic/LLVMBandage.so"
opt_soft="opt -load ../../Bandage_build/SoftBound/SoftBound.so"
llc="llc"
clang="clang"

time="/usr/bin/time"

r0="${1}-raw-o0"
r3="${1}-raw-o3"
b0="${1}-ban-o0"
b3="${1}-ban-o3"
s0="${1}-sft-o0"

${clang} -O0 -S -emit-llvm ${1}.c -o ${1}.bc

# Create the reference versions
echo "Creating r0"
${opt} -S ${1}.bc > ${r0}.bc
${llc} ${r0}.bc -o ${r0}.s
${clang} ${r0}.s -o ${r0} 

# Create the bandage versions
echo "Creating b0"
${opt} -S -bandage ${1}.bc > ${b0}.bc >& /dev/null
${llc} ${b0}.bc -o ${b0}.s
${clang} ${b0}.s -o ${b0} 

#echo "Creating s0"
#${opt_soft} -S -softbound ${1}.bc > ${s0}.bc >& /dev/null
#${llc} ${s0}.bc -o ${s0}.s
#${clang} ${s0}.s -o ${s0} 


rm ${r0}.s ${r3}.s ${b0}.s ${b3}.s 2> /dev/null || true
rm ${r0}.txt ${r3}.txt ${b0}.txt ${b3}.txt 2> /dev/null || true

echo `date` >> ${i}.log
for benchmark in $r0 $b0 # $r3 $b3
do
  echo -n "Timing ${benchmark}"
  for i in {1..5}
  do
    ./${benchmark} >> ${benchmark}.txt
    echo -n " ${i}"
  done
  echo
done

# Get rid of an error output
grep -e '[0-9]\.[0-9]' ${b0}.txt > Temp
cat Temp > ${b0}.txt

awk '{total += $1} END {print "Raw -O0: " total/NR;}' ${r0}.txt >> ${1}.log
#awk '{total += $1} END {print "Raw -O3: " total/NR;}' ${r3}.txt >> ${1}.log
awk '{total += $1} END {print "Ban -O0: " total/NR;}' ${b0}.txt >> ${1}.log
#awk '{total += $1} END {print "Ban -O3: " total/NR;}' ${b3}.txt >> ${1}.log

tail -n 2 ${1}.log
ministat -s ${r0}.txt ${b0}.txt
