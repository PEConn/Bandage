#include <stdlib.h>
#include <stdio.h>
#include "Timing.h"

void Fun1(){}
void Fun2(){
  int *a, *b, *c, *d, *e, *f, *g, *h, *i, *j;
}

int main(){
  int lim_i = 50000;
  int lim_j = 10000;
	
  float t0 = TIME;
	for(int i=0; i<lim_i; i++)
		for(int j=0; j<lim_j; j++)
      Fun1();

  float t1 = TIME;
	for(int i=0; i<lim_i; i++)
		for(int j=0; j<lim_j; j++)
      Fun2();
  float t2 = TIME;

  printf("%f\n",(t2 - t1) - (t1 - t0));
	return 0;
}
