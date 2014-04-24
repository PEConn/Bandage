#include <stdio.h>
#include "Timing.hpp"

int main(){
  printf("%f\n", TIME);
  for(unsigned long i=0; i< 400000000; i++){
    int a[5];
  }
  printf("%f\n", TIME);

  return 0;
}
