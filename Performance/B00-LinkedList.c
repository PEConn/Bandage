// RUN: ./runOn.sh %s | FileCheck %s
#include <stdio.h>
#include <stdlib.h>
#include "../SoftBound/Headers/HashTable.h"

typedef struct Link{
  int val;
  struct Link *next;
} Link;
const int LinkSize = 28;

int main(){
  Link *first = malloc(LinkSize);
  Link *current = first;

  printf("Creating\n");
  for(int i=0; i<4; i++){
    current->next = malloc(LinkSize);
    current = current->next;
  }
  current->next = NULL;

  printf("Following\n");
  current = first;
  while(current->next != NULL){
    current = current->next;
  }
  // CHECK-NOT: OutOfBounds
  // CHECK-NOT: Null
}
