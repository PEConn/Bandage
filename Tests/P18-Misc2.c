// RUN: ./runOn.sh %s | FileCheck %s
#include <stdio.h>
#include <stdlib.h>

typedef struct node_t {
  struct node_t **from_nodes; /* array of nodes data comes from */
} node_t;

int main(){
  node_t *nodelist = malloc(24);
  nodelist->from_nodes = malloc(24);
  // Access with constant
  node_t *other_node = nodelist->from_nodes[0];

  // Access with variable
  int i = 0;
  other_node = nodelist->from_nodes[i];
  printf("Ran\n");
  // CHECK: Ran
  return 0;
}
