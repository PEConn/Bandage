// RUN: ./runOn.sh %s | FileCheck %s
#include <stdio.h>

struct List {
  struct Patient         *patient;
  struct List            *back;
  struct List            *forward; 
};
struct Hosp {
  int                    personnel; 
  int                    free_personnel; 
  int                    num_waiting_patients; 
  struct List            waiting; 
  struct List            assess; 
  struct List            inside;
  struct List    up; 
};
struct Village {
  struct Village         *forward[4],
                         *back;
  struct List            returned;
  struct Hosp            hosp;   
  int                    label;
  long                   seed; 
};


int main(){
  struct Village *village;
  struct List *l = &(village->hosp.inside);
  printf("Compiled\n");
  // CHECK: Compiled
  return 0;
}

