#!/bin/bash

set -e 

FN=${1%.c}
echo "Remember, this needs to have paths set"
cd ${1}
echo "Compiling locally"
clang -c -target armv6--freebsd10.0-gnueabi -D PLAIN -D SS_PLAIN ${FN}.c
echo "Transferring to RPi"
scp ${FN}.o 131.111.245.209:/usr/home/pc424/PerformanceTests/
echo "Compiling on RPi"
CD="cd /usr/home/pc424/PerformanceTests/"
LD_COMMAND="ld --eh-frame-hdr -dynamic-linker /libexec/ld-elf.so.1 --hash-style=both --enable-new-dtags -o ${FN} /usr/lib/crt1.o /usr/lib/crti.o /usr/lib/crtbegin.o ${FN}.o -lgcc --as-needed -lgcc_s --no-as-needed -lc -lgcc --as-needed -lgcc_s --no-as-needed /usr/lib/crtend.o /usr/lib/crtn.o"
ssh -t pc424@131.111.245.209 << EOF
${CD}
${LD_COMMAND}
EOF
echo "Compiled successfully"
