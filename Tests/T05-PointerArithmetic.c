// RUN: ./pointerAnalysis.sh %s 2>&1 | FileCheck %s

#include <stdio.h>
#include <stdlib.h>

int main(){
  int *a;
  int *b;

  b = a + 4;

  int *c;
  c++;

	return 0;
}
