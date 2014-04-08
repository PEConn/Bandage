// RUN: ./runOn.sh %s | FileCheck %s
#include <stdio.h>
#include <stdlib.h>

typedef struct Point{
  int x;
  int y;
  int z;
} Point;

typedef struct Triangle{
  Point *a;
  Point *b;
  Point *c;
}

int main(){
  Point *a = malloc(sizeof(Point));
  a->x = 1;
  a->y = 2;
  a->z = 0;

  Triangle *t = malloc(sizeof(Triangle));
  t->a = a;

  printf("%d\n", t->a->x);
  // CHECK: 1
  printf("%d\n", t->a->y);
  // CHECK: 2
  printf("%d\n", t->a->z);
  // CHECK: 0

  printf("Compiles\n");
  // CHECK: Compiles
  return 0;
}
