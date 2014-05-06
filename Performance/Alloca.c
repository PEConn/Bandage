#include <stdlib.h>
#include <stdio.h>
#include "Timing.h"

void Fun(){
  int *x;
}

int main(){
	printf("");
  int x;
  int *y;
	
  float t0 = TIME;
	for(int i=0; i<100000; i++){
		for(int j=0; j<10000; j++){
      Fun();
		}
	}
  printf("%f\n", TIME - t0);
	return x;
}
