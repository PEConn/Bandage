// RUN: ./pointerAnalysis.sh %s 2>&1 | FileCheck %s

#include <stdio.h>
#include <stdlib.h>

int main(){
	int *x = malloc(sizeof(int));

  *x = 4;
  printf("%d\n", *x);

	return 0;
}

void Function(){
  char *y;
}

// CHECK: (x, 0): Safe
// CHECK: (y, 0): Safe
