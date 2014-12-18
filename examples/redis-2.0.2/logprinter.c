#include <stdlib.h>
#include <string.h>
#include <stdio.h>




int main(void){


    FILE * prof = fopen("prof.txt", "rb"); 

    int count;
    long typ;
    long maintyp;
    int sz;
    int total = 0;
    while(!feof(prof)){
       
       fread(&count, sizeof(int), 1, prof);
       fread(&typ, sizeof(long), 1, prof);
       fread(&maintyp, sizeof(long), 1, prof);
       fread(&sz, sizeof(int), 1, prof);
       if(typ != maintyp)
         printf("type: %ld (maintype %ld),  instances: %d, total size = %d\n", typ, maintyp, count, (count*sz));
       else
         printf("type: %ld,  instances: %d, total size = %d\n", typ, count, (count*sz));
       total+=(count*sz);
    }

    printf("total heap structures allocated: %d\n", total);
}
