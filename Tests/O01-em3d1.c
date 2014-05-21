// RUN: ./runOn.sh %s | FileCheck %s
#include <stdio.h>

typedef struct node_t{
  double value;
} node_t;

typedef struct graph_t{
  node_t *e;
  node_t *h;
} graph_t;

void print_graph(graph_t graph){

}

int main(){
  printf("Compiled\n");
  // CHECK: Compiled
}
