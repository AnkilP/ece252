// this is a wrapper "thing" (im not sure what non-oop guys call it)
// to help augment the glibc hashmap thing to be thread safety

//NOTE: this is only made for a single hashmap - cannot be extended to multiple parallel hashmaps

#include <serach.h>
#include "curl_helper.h"

typedef struct hashmapz{
    int size;
    ENTRY e, *ep;
} Hashtable;

int create_hash_map(hashmapz * t, int size);
int add_to_hashmap(hashmapz * t, char * url, sem_t * web_protect, int * iter);
int delete_hashmap(hashmapz * t);
int lookup(hashmapz * t, char * url);
int get_data();
