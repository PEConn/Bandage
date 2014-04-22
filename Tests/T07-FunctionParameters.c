// RUN: ./pointerAnalysis.sh %s 2>&1 | FileCheck %s

#include <stdio.h>
#include <stdlib.h>

void X(int *a){
  int *x = a;
}

void Y(int **b){
  int *y = *b;
}

void Z(int *c){
  int z = *c;
}

int main(){
  int *e;
  X(e);

  Y(&e);

  Z(e);
	return 0;
}
