set -e
set -v

base="LinkedList"
raw="${base}-raw"
ban="${base}-ban"
hash="${base}-hash"
mem="${base}-mem"

clang ${raw}.o -o ${raw}
clang ${ban}.o -o ${ban}
clang ${hash}.o HashTable.o -o ${hash}
clang ${mem}.o MemTable.o -o ${mem}

./${raw}
./${ban}
./${hash}
./${mem}

rm ${raw} ${ban} ${hash} ${mem}
