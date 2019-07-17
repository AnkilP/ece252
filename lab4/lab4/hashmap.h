// this is a wrapper "thing" (im not sure what non-oop guys call it)
// to help augment the glibc hashmap thing to be thread safety

//NOTE: this is only made for a single hashmap - cannot be extended to multiple parallel hashmaps

#include <search.h>
#include "curl_helper.h"

typedef struct hashtable{
    struct hsearch_data htab;
    int size;
    ENTRY e, *ep;
} Hashtable;

int create_hash_map(Hashtable * t, int size);
int add_to_hashmap(Hashtable * t, char * url, pthread_rwlock_t * rwlock, int * iter);
int delete_hashmap(Hashtable * t);
int lookup(Hashtable * t, char * url, pthread_rwlock_t * rwlock);
