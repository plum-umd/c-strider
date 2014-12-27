#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <cstrider_api.h>
#include "main.h"
#include "prog_specific.h"
extern FILE * ser; 
char * fname;
extern struct traversal * union_funs;


struct tagged_union * intu;
struct tagged_union * charu;



int current_service;

/* CODE FOR PAPER */
void checkpoint(void){
  current_service = SERIALIZE;
  ser = fopen("ser.txt", "wb"); 
  init(union_funs, 0);
  visit_all();
  finish();
  fclose(ser); 
}

int do_deserialize(int argc, char **argv){
   assert(argc==3); //TODO better processing
   if((char)argv[1][0] == 'D'){
       current_service = DESERIALIZE;
       printf("reading");
       ser = fopen("ser.txt", "rb"); 
       init(union_funs, 0);
       visit_all();
       finish();
       fclose(ser); 
       return 1;
   }
   return 0;

}

/* CODE FOR PROGRAM */
int main(int argc, char **argv){

  union_funs = union_funs_init();
  if(!do_deserialize(argc, argv)){
     intu = calloc(1, sizeof(struct tagged_union));
     intu->tag = 1;
     intu->u.x = 9;
     charu = calloc(1, sizeof(struct tagged_union));
     charu->tag = 0;
     charu->u.c = 'a';
  }
  else{
     assert(intu->tag == 1);
     assert(intu->u.x == 9);
     assert(charu->tag == 0);
     assert(charu->u.c == 'a');
     printf(">>>>>>>>>>>>>Passed assertions<<<<<<<<<<<<<<\n");
  }
  int cnt=0;
  while(1){
     cnt++;
     if((cnt%99999)==0){
         checkpoint();
         break;
     }
  }
  
  free(union_funs);
  return 0;
}
