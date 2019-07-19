#include "hashmap.h"

#define _GNU_SOURCE

int add_to_hashmap(Hashtable ** t, char * url, pthread_rwlock_t * rwlock){
    ENTRY e, *ep;
    e.key = malloc(strlen(url) + 1);
    strcpy(e.key, url);
    e.data = (int *) 1;
    pthread_rwlock_wrlock( rwlock );
    hsearch_r(e, ENTER, &ep, (*t)->htab);
    if(ep == NULL) {
        pthread_rwlock_unlock( rwlock );
        //fprintf(stderr, "hashmap entry failed");
        return 0;
    }
    else {
        pthread_rwlock_unlock( rwlock );
        return 1; // new url found
    }
}

int lookup(Hashtable ** t, char* url) {
    ENTRY e, *ep;

    e.key = url;
    unsigned n = hsearch_r(e, FIND, &ep, (*t)->htab);
    if(n == 0){
        return 0;
    }
    else {
        return 1;
    }
}

int delete_hashmap(Hashtable * t){
    hdestroy_r(t->htab);
    free(t);
    t = NULL;
    return 1;
}