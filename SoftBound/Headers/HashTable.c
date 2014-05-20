#include "SoftBound.h"
#include "uthash.h"

typedef struct{
  void *key;
  void *base, *bound;
  UT_hash_handle hh;
} TableEntry;

TableEntry *HashTable = NULL;

inline void TableSetup(){
}

inline void TableTeardown(){
}

inline void TableLookup(void *key, void **base, void **bound){
  // base and bound are pointers to local variables to store the base and bound in
  // key is the address of the pointer for whom we are searching for bounds
  *base = NULL;
  *bound = NULL;
  TableEntry *TE = NULL;
  HASH_FIND_INT(HashTable, &key, TE);
  if(TE){
    *base = TE->base;
    *bound = TE->bound;
  }
}

inline void TableAssign(void *key, void *base, void *bound){
  TableEntry *TE = malloc(sizeof(TableEntry));
  TableEntry *old = NULL;
  TE->key = key;
  TE->base = base;
  TE->bound = bound;
  // Todo, check if this is already declared
  HASH_REPLACE_INT(HashTable, key, TE, old);
  if(old != NULL)
    free(old);
}
