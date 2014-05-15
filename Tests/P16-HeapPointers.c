// RUN: ./runOn.sh %s | FileCheck %s
#include <stdio.h>
#include <stdlib.h>

#include "../SoftBound/Headers/HashTable.h"

int main(){
	printf("Start\n");
	// CHECK: Start

	int **x;
	x = malloc(sizeof(int*));
  *x = malloc(5*sizeof(int));

  **x = 1;
  int **y;
  y = x;

  **y = 3;

  free(*x);
	free(x);

	return 0;
}
