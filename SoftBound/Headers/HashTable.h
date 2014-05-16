#include <stdio.h>
#include "uthash.h"

typedef struct{
  void *key;
  void *base, *bound;
  UT_hash_handle hh;
} TableEntry;

TableEntry *HashTable = NULL;

void TableSetup(){
}

void TableTeardown(){
}

void TableLookup(void *key, void **base, void **bound){
  // base and bound are pointers to local variables to store the base and bound in
  // key is the address of the pointer for whom we are searching for bounds
  *base = NULL;
  *bound = NULL;
  TableEntry *TE = NULL;
  //printf("Search: %p\n", key);
  HASH_FIND_INT(HashTable, &key, TE);
  if(TE){
    *base = TE->base;
    *bound = TE->bound;
  }
}

void TableAssign(void *key, void *base, void *bound){
  TableEntry *TE = malloc(sizeof(TableEntry));
  TableEntry *old = NULL;
  TE->key = key;
  TE->base = base;
  TE->bound = bound;
  //printf("Assign: %p\n", key);
  // Todo, check if this is already declared
  HASH_REPLACE_INT(HashTable, key, TE, old);
  if(old != NULL)
    free(old);
}
