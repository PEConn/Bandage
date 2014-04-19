// RUN: ./pointerAnalysis.sh %s 2>&1 | FileCheck %s

#include <stdio.h>
#include <stdlib.h>

int main(){
  int len = 10;
	int *x = malloc(len * sizeof(int));

  for(int i=0; i<len; i++){
    *x = i;
    x++;
  }

  // CHECK: Sequential: x
	return 0;
}
