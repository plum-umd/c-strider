#include <stdlib.h>
#include <stdio.h>
#include <cstrider_api.h>
#include "main.h"

extern struct traversal funs;

/* global listptr*/
dbinfo * listptr;

void do_traversal(void){
  init(&funs,0);
  visit_all();
  finish();
}

void test_locals(void){
  /* local listptr*/
  dbinfo * loc_listptr;
  loc_listptr = calloc(1, sizeof(dbinfo));
  loc_listptr->p = 'l';

  register_root(&loc_listptr, TYPE_dbinfo_PTR_i);
  do_traversal();
  deregister_root(&loc_listptr);
  free(loc_listptr);
}

/* CODE FOR PROGRAM */
int main(int argc, char **argv){

  listptr = calloc(1, sizeof(dbinfo));
  listptr->p = 'g';

  printf("Traversing with only one global\n");
  do_traversal();

  printf("Traversing with global and local\n");
  test_locals();

  printf("Traversing with only one global\n");
  do_traversal();
  return 0;
}
