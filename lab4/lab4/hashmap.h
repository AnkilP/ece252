// this is a wrapper "thing" (im not sure what non-oop guys call it)
// to help augment the glibc hashmap thing to be thread safety

#include "node.h" // this has search.h in it - I dont like polluting the namespace
#include <sys/types.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>

typedef struct hashtable{
    struct hsearch_data htab;
    int size;
} Hashtable;

#define HASHTABLE_INITIALIZER                                                    \
  (Hashtable)                                                                    \
  {                                                                            \
    .htab = (struct hsearch_data){ 0 }, .size = 0                              \
  }

int create_hash_map(Hashtable * t, int size);
int add_to_hashmap(Hashtable * t, char * url, pthread_rwlock_t * rwlock);
int delete_hashmap(Hashtable * t);
int lookup(Hashtable * t, char * url, pthread_rwlock_t * rwlock);
