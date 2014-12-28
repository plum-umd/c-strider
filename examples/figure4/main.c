#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <cstrider_api.h>
#include "serial.h"
extern int TYPE_STRUCT__dbinfo_i;
extern struct traversal ser_funs;
extern struct traversal deser_funs;
extern FILE * ser; 

//////////
typedef struct _dbinfo{
  struct _dbinfo * next;
  char p;
}dbinfo;
char * fname;

dbinfo * listptr;

/* CODE FOR PAPER */
void checkpoint(void){
  ser = fopen("ser.txt", "wb"); 
  init(&ser_funs, 0);
  visit(&listptr, TYPE_STRUCT__dbinfo_i, &listptr);
  finish();
  fclose(ser); 
}

int do_deserialize(int argc, char **argv){
   assert(argc==3); //TODO better processing
   if((char)argv[1][0] == 'D'){
       printf("reading");
       ser = fopen("ser.txt", "rb"); 
       init(&deser_funs, 0);
       visit(&listptr, TYPE_STRUCT__dbinfo_i, &listptr);
       finish();
       fclose(ser); 
       return 1;
   }
   return 0;

}

/* CODE FOR PROGRAM */
int main(int argc, char **argv){

  if(!do_deserialize(argc, argv)){
     listptr = calloc(1, sizeof(dbinfo));
     listptr->p = 'c';
  }
  else{
     assert('c'==listptr->p);
     printf(">>>>>>>>>>>>>Passed assertion<<<<<<<<<<<<<<\n");
  }
  int cnt=0;
  while(1){
     cnt++;
     if((cnt%99999)==0){
         checkpoint();
         break;
     }
  }
  
  return 0;
}
