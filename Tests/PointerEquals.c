// RUN: ./runOn.sh PointerEquals | /pool/users/pc424/llvm_build/bin/FileCheck %s
#include <stdio.h>
#include <stdlib.h>

int main(){
  printf("Clean\n");
  // CHECK: Clean

  char *w = "bat";
  char *word = w;

  printf("%c\n", *word);
  // CHECK-NOT: OutOfBounds

  printf("Dirty\n");
  // CHECK: Dirty

  word = word + 4;
  printf("%c\n", *word);
  // CHECK: OutOfBounds

  printf("Finished\n");
  // CHECK: Finished
  return 0;

}
