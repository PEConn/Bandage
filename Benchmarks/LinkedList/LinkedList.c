#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include "../../SoftBound/Headers/SoftBound.h"

typedef unsigned long long ull;

typedef struct Link{
  int val;
  struct Link *next;
} Link;
const int LinkSize = 28;

void Run(ull items){
  Link *first = malloc(LinkSize);
  Link *current = first;

  for(int i=0; i<items; i++){
    current->next = malloc(LinkSize);
    current = current->next;
  }
  current->next = NULL;

  clock_t t0 = clock();
  for(int i=0; i<1000; i++){
    current = first;
    while(current->next != NULL){
      current = current->next;
    }
  }
  clock_t t1 = clock();

  printf("%llu:\t%d\n", items, t1-t0);
  current = first;
  while(current->next != NULL){
    Link *prev = current;
    current = current->next;
    free(prev);
  }
  free(current);
}

int main(){
#ifdef SOFTBOUND
  TableSetup();
#endif
  ull inc = 1000;
  for(ull i=inc; i<=10*inc; i+=inc){
    Run(i);
  }
#ifdef SOFTBOUND
  TableTeardown();
#endif
}

#ifdef SOFTBOUND
void Dummy(){
  TableLookup(NULL, NULL, NULL);
  TableAssign(NULL, NULL, NULL);
}
#endif
