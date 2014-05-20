#include <stdio.h>
#include <assert.h>
#include <sys/mman.h>

typedef struct TableEntry{
  void *base;
  void *bound;
} TableEntry;

void *memtable;
int setup = 0;

#define ENTRY_SIZE sizeof(TableEntry)
#define ENTRY_COVERAGE sizeof(void *)
#define TABLE_START (1ULL<<44)
#define TABLE_SIZE  (3ULL<<45)
#define TABLE_HALF (TABLE_START + TABLE_SIZE)

void TableSetup(){
  memtable = mmap((void *)TABLE_START, TABLE_SIZE, PROT_READ|PROT_WRITE,
     MAP_PRIVATE|MAP_ANONYMOUS|MAP_NORESERVE|MAP_FIXED, -1, 0); 
  printf("Start:\t\t%p\n", (void *)TABLE_START);
  printf("End:\t\t%p\n", (void *)(TABLE_START + TABLE_SIZE));
  setup = 1;
}

void TableTeardown(){
  munmap(memtable, TABLE_SIZE);
  setup = 0;
}

void TableAssign(void *key, void *base, void *bound){
  //assert(setup);
  //printf("Assign: \t%p\n", key);
  char *addr;
  if(key < (void *) TABLE_START)
    addr = (char *) key;
  else
    addr = (char *) (key - (void *) TABLE_SIZE);

  unsigned long long elemNo = (unsigned long long) addr / ENTRY_COVERAGE;
  TableEntry *elemPtr = (TableEntry *) ((elemNo * ENTRY_SIZE) + TABLE_START);
  //printf("Addr: \t\t%p\n", elemPtr);
  //assert(((unsigned long long) elemPtr) >= TABLE_START);
  //assert(((unsigned long long) elemPtr) < TABLE_START + TABLE_SIZE);

  elemPtr->base = base;
  elemPtr->bound = bound;
}

void TableLookup(void *key, void **base, void **bound){
  //assert(setup);
  //printf("Lookup: \t%p\n", key);
  char *addr;
  if(key < (void *) TABLE_START)
    addr = (char *) key;
  else
    addr = (char *) (key - (void *) TABLE_SIZE);

  unsigned long long elemNo = (unsigned long long) addr / ENTRY_COVERAGE;
  TableEntry *elemPtr = (TableEntry *) ((elemNo * ENTRY_SIZE) + TABLE_START);
  //printf("Addr: \t\t%p\n", elemPtr);
  //assert(((unsigned long long) elemPtr) >= TABLE_START);
  //assert(((unsigned long long) elemPtr) < TABLE_START + TABLE_SIZE);
  //assert(bound != NULL);
  //assert(base != NULL);

  *base = elemPtr->base;
  *bound = elemPtr->bound;
}
