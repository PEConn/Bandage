// RUN: ./runOn.sh BufferOverflow | /pool/users/pc424/llvm_build/bin/FileCheck %s
// RUN: rm BufferOverflow.bc BufferOverflow_ban.bc BufferOverflow_ban.s BufferOverflow
#include <stdio.h>
#include <stdlib.h>

int main(){
  printf("Clean\n");
  // CHECK: Clean

  char *w1 = malloc(sizeof(char)*4);
  char *w2 = malloc(sizeof(char)*2);

  //*(w1+3) = '\0';
  w1 = w1+3;
  *w1 = '-';
  w1 = w1-3;

  for(; /* *w1 == *w2 */ ; w1++, w2++){
    if (*w1 != '\0')
      break;
  }

  free(w1);
  free(w2);
  printf("Finished\n");
  // CHECK: Finished
  return 0;

}
