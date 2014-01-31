opt="/pool/users/pc424/llvm_build/bin/opt -load ../../Bandage_build/Basic/LLVMBandage.so"
llc="/pool/users/pc424/llvm_build/bin/llc"
clang="/pool/users/pc424/llvm_build/bin/clang"

echo -en "\033[33m"
rm ${1}_ban.bc
${opt} -S -bandage ${1}.bc > ${1}_ban.bc

rm ${1}_ban.s
${llc} ${1}_ban.bc -o ${1}_ban.s

rm ${1}
${clang} ${1}_ban.s -o ${1} 

echo -en "\033[0m"

./${1}
