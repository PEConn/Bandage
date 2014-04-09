// RUN: ./runOn.sh %s | FileCheck %s
#include <stdio.h>
#include <stdlib.h>

typedef struct LL{
  struct LL *n;
  int v;
} LL;

int main(){
  LL *a;
  a = malloc(sizeof(LL));
  a->v = 2;
  a->n = malloc(sizeof(LL));
  a->n->v = 4;
  a->n->n = malloc(sizeof(LL));
  a->n->n->v = 6;

  printf("Start\n");
  // CHECK-NOT: OutOfBounds
  // CHECK: Start

  printf("%d\n", a->v);
  printf("%d\n", a->n->v);
  printf("%d\n", a->n->n->v);

  printf("Dirty\n");
  // CHECK-NOT: OutOfBounds
  // CHECK: Start

  printf("%d\n", a->n->n->n->v);
  // CHECK: OutOfBounds

  printf("Compiles\n");
  // CHECK: Compiles
  return 0;
}
