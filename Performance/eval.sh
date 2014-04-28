#! /bin/bash

set -e

opt="opt -load ../../Bandage_build/Basic/LLVMBandage.so"
llc="llc"
clang="clang"

time="/usr/bin/time"

${opt} -always-inline ${1}.bc > ${1}_inlined.bc
${llc} ${1}_inlined.bc -o ${1}.s
${clang} ${1}.s -o ${1} 

rm ${1}.s > /dev/null
rm times.txt > /dev/null || true

echo -n "Timing ${1}"
for i in {1..5}
do
  ./${1} > times.txt
  echo -n " ${i}"
done
echo

# Get rid of an error output
grep -e '[0-9]\.[0-9]' times.txt > Temp
cat Temp > times.txt

awk '{total += $1} END {print "" total/NR;}' times.txt
