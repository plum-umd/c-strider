4 types of hashtbls: 
=======================================
*** uthash.h - standard issue UT hash.  Expands as necessary. (used for thread local storage, does not need thread-capabilities). 
NOT thread-safe, must use ktthread_lock() or other mutex:
Manual: http://uthash.sourceforge.net/userguide.pdf


---------------------------------------
3 Thread-capable versions for ADD and FIND operations ONLY.  (Other operations explained below).  Locks bucket-by bucket.  Must call "RESV" functions to lock row prior to modifying a bucket in that row.  Each bucket has its own lock. 



*** uthash_nxform.h ***  (used for main map)
* non-expanding, thread-enabling version of UTHASH. Experimenting optimized at 32768 (2^15) rows for both Snort and Redis, each with own mutex. 
* Hash table is a fixed size...warning...if hash table becomes too full, performance will dive in an extreme way.  
* For speed, does not have a "link-lock" on the entire table to provide between-row correctness, which means that "table" information is not accurate (entire tbl count, pointers between rows, etc)...but bucket information will always be correct and has all the information necessary (just not as efficient to get counts, etc).
** information outside the rows is not reliable, which breaks some functions such as the iterator which relies on between-row links (ITERATOR)
* provides "FREE_ALL_MALLOCED_ENTRIES" to do freeing at the end (slower), since traditional iterator is broken (due to not locking tbl, faster).  This  frees all malloced data.  Then call CLEAR on the tbl to wip the table's structures.
* MUCH FASTER than xhash in most situations with threads as the entire table is never locked.
* Also provides thread-safe delete: "NXHASH_THR_DELETE", but must uncomment link_lock inside NXHASH_ADD_KEYPTR to use reliably.


*** uthash_xform.h *** (used for type table (see xfgen.ml), this is linked with the dsu.c file)
* Expanding hash table.  
* Locks bucket by bucket...up to 1024 buckets. (after that, the buckets share mutexes mod-1024 which seems to still have very low contention even in very fast situations.)  
* Has an entire lock "link-lock" which is locked very breifly on each ADD because the "count" is decremented and a pointer is modified.  
* deletions not implemented with mutexes yet.
* This is faster in some situations and nice when you have no idea what size to start the table at.  
* However, it is MUCH SLOWER in some situations....in order to expand the table, the entire table must be locked, which stalls all threads.  
* This table has an additional "expand" mutex which locks when all threads are stalled.*  The expand operation itself is very rapid/low overhead, but pausing all threads is killer.

*** uthash_bgxform.h *** ///currently unused, but could be used to replace nxhash if we need an expander for the main hashtable
This is the same file as uthash_xform.h except that it started with much larger buckets, and has the FREE_ALL_MALLOCED which allows the threads to help teardown teh table.


Example Usage:
   XHASH_WRLOCK_RESV(freelater_hash, &addr);
   freelater_entry *fl_item;
   XHASH_FIND_PTR(freelater_hash, &addr, fl_item);
   if (!fl_item) {
     fl_item = malloc(sizeof(freelater_entry));
     fl_item->addr = addr;
     fl_item->closure_with_args = closure_with_args;
     XHASH_ADD_PTR(freelater_hash, addr, fl_item);
   }
   XHASH_UNLOCK_RESV(freelater_hash, &addr);

If you will ONLY "find" and entry, then you can use RDLOCK.
Remember that if there is a chance you will write after "find", then use WRLOCK for the entire critical section.


