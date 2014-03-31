// RUN: ./runOn.sh FunctionControlFlow | /pool/users/pc424/llvm_build/bin/FileCheck %s

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
