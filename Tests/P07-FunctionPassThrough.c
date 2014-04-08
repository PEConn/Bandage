// RUN: ./runOn.sh %s | FileCheck %s
#include <stdio.h>
#include <stdlib.h>

char* Func(char* x){
  return x;
}

int main(){
  printf("Clean\n");
  // CHECK: Clean

  char *word = "bat";
  word = Func(word);
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
