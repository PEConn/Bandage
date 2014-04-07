// RUN: ./runOn.sh %s | FileCheck %s
#include <stdio.h>
#include <stdlib.h>

typedef struct Pointless{
  struct Pointless *p;
} Pointless;

int main(){
  Pointless *a = malloc(sizeof(Pointless));
  Pointless *a->p = malloc(sizeof(Pointless));
  Pointless *a->p->p = malloc(sizeof(Pointless));

  printf("Compiles\n");
  // CHECK: Compiles
  return 0;
}
