// RUN: ./runOn.sh %s | FileCheck %s

#include <stdio.h>
#include <stdlib.h>

void MyFunc(int x){
  if(x)
    printf("A\n");
  else
    printf("B\n");
}

int main(){
  MyFunc(1);

  printf("Compiled\n");
  // CHECK: Compiled

	return 0;
}
