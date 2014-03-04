// RUN: ./runOn.sh ConstStrings | /pool/users/pc424/llvm_build/bin/FileCheck %s
#include <stdio.h>
#include <stdlib.h>

int main(){
  printf("Clean\n");
  // CHECK: Clean

  char *w1 = "ball";
  char *w2 = "bat";
  printf("%s\n", w1);
  // CHECK: ball
  printf("%s\n", w2);
  // CHECK: bat

  for(; *w1 == *w2; w1++, w2++){
    if (*w1 == '\0')
      break;
  }
  // CHECK-NOT: OutOfBounds
  printf("Dirty\n");
  // CHECK: Dirty

  w2++;
  w2++;
  printf("%c\n", *w2);
  // CHECK: OutOfBounds

  printf("Finished\n");
  // CHECK: Finished
  return 0;

}
