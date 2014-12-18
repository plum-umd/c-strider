This example profiles the types of redis.
It dumps them all into a binary file "prof.txt".

To run redis:
```sh
$ make
$ ./redis-server
```

To read the file, do:
```sh
$ ./logprinter prof.txt
```
(You can reference which types they are in dsu.h.  It's a TODO to match these up automatically...)


Changes made for redis:

- The main change is that since "redisServer", the main state-holding variable is "static", we had to "unstatic" it so we could traverse it (and so that the appropritate type traversal information is generated).  (C-strider does not support static roots).
- We had to add generic annoations to several data structures, including adlist.h (struct list), dict.h (struct dictEntry, struct dictht), and redis.c (instances of dicts and lists).
- We added a "checkpoint" function to fire a profiling traversal every ~10 seconds (in redis.c).  Note that this currently overwrites the previous profile's file...


Compiling bug:
- The way that we are genearting type information for global variable registration has a bug when you recompile one file only.  For best results, do "make clean; make" when recompiling anything.  *This is a major TODO.*
