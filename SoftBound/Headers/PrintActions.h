#include <stdio.h>

void TableSetup(){
  printf("Table Setup\n");
}

void TableTeardown(){
  printf("Table Teardown\n");
}

void TableLookup(void *key, void **base, void **bound){
  // base and bound are pointers to local variables to store the base and bound in
  // key is the address of the pointer for whom we are searching for bounds
  *base = NULL;
  *bound = NULL;
  printf("Table Lookup: %p\n", key);
}

void TableAssign(void *key, void **base, void **bound){
  printf("Table Assign: %p\n", key);
  printf("Lower Bound : %p\n", base);
  printf("Upper Bound : %p\n", bound);
}
