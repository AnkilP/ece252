#include "hashmap.h"

int create_hash_map(Hashtable * t, int size){
    t = (Hashtable * )malloc(sizeof(Hashtable));
    if(size > 0){
        t->size = size;
    }
    else{
        return 0;
    }
    hcreate_r(size, &(t->htab));
    return 1;
}

int add_to_hashmap(Hashtable * t, char * url, pthread_rwlock_t * rwlock){
    t->e.key = url;
    t->e.data = (void *) 1;
    pthread_rwlock_wrlock( rwlock );
    hsearch_r(t->e, ENTER, &t->ep, &t->htab);
    pthread_rwlock_unlock( rwlock );
    if(t->ep == NULL){
        fprintf(stderr, "hashmap entry failed");
        return 0;
    }
    else{
        return 1; // new url found
    }
}

int lookup(Hashtable * t, char * url, pthread_rwlock_t * rwlock){
    pthread_rwlock_rdlock(rwlock);
    t->e.key = url;
    hsearch_r(t->e, FIND, &t->ep, &t->htab);
    if(t->ep){
        pthread_rwlock_unlock(rwlock);
        return 1;
    }
    else{
        pthread_rwlock_unlock(rwlock);
        return 0;
    }
}

int delete_hashmap(Hashtable * t){
    hdestroy_r(&t->htab);
    free(t);
    return 1;
}