#ifndef MAIN_H
#define MAIN_H

#define DESERIALIZE 0
#define SERIALIZE 1

struct tagged_union
{
   int tag;
   union
   {
      int x;
      char c;
   } u; /* selected by tag */
};
#endif
