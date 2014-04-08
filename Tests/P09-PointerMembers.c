// RUN: ./runOn.sh %s | FileCheck %s
#include <stdio.h>
#include <stdlib.h>

typedef struct Point{
  int *x;
  int *y;
  int *z;
} Point;

int main(){
  printf("Start\n");
  // CHECK: Start

  Point a;
  a.x = malloc(sizeof(int)); *a.x = 1;
  a.y = malloc(sizeof(int)); *a.y = 2;
  a.z = malloc(sizeof(int)); *a.z = 0;
  // CHECK-NOT: OutOfBounds

  printf("%d\n", *a.x); 
  // CHECK: 1
  printf("%d\n", *a.y); 
  // CHECK: 2
  printf("%d\n", *a.z); 
  // CHECK-NOT: OutOfBounds
  // CHECK: 0

  return 0;
}
