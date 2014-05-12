#include "stdio.h"

void SayHello(){
  printf("Hello\n");
}

void External();

int main(){
  External();
  return 0;
}
