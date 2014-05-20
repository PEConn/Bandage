// RUN: ./runOn.sh %s | FileCheck %s

#include <stdio.h>

typedef struct{
  //int x[5], y[5], z[5];
  int a,b,c,d,e,f,g,h;
} BigStruct;

BigStruct ReturnPoint(){
  BigStruct a;
  return a;
}
int main(){
  BigStruct b;
  b = ReturnPoint();
  printf("Compiled\n");
  // CHECK: Compiled
  return 0;
}
