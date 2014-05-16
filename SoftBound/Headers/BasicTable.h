#include <stdio.h>

void *table[5][3];

void TableSetup(){
  for(int j=0; j<5; j++){
    table[j][0] = NULL;
    table[j][1] = NULL;
    table[j][2] = NULL;
  }
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
  for(int i=0; i<5; i++){
    printf("%p: (%p, %p)\n", table[i][0], table[i][1], table[i][2]);
    if(table[i][0] == key){
      *base = table[i][1];
      *bound = table[i][2];
    }
  }
}

void TableAssign(void *key, void *base, void *bound){
  printf("Table Assign: %p\n", key);
  //printf("Lower Bound : %p\n", base);
  //printf("Upper Bound : %p\n", bound);
  for(int i=0; i<5; i++){
    if(table[i][0] == NULL){
      table[i][0] = key;
      table[i][1] = base;
      table[i][2] = bound;
      break;
    }
  }
}
