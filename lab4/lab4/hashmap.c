#include "hashmap.h"

int create_hash_map(Hashtable * t, int size){
    t = (Hashtable * )malloc(sizeof(*t));
    if(size > 0){
        t->size = size;
    }
    else{
        return 0;
    }

    memset((void *)&t->htab, 0, sizeof(t->htab));
    return hcreate_r(size, &(t->htab));
}

int add_to_hashmap(Hashtable * t, char * url, pthread_rwlock_t * rwlock){
    
    ENTRY e, *ep;
    unsigned n = 0;
    e.key = url;
    e.data = (int *) 1;
    pthread_rwlock_wrlock( rwlock );
    n = hsearch_r(e, ENTER, &ep, &(t->htab));
    pthread_rwlock_unlock( rwlock );
    if(ep == NULL){
        fprintf(stderr, "hashmap entry failed");
        return 0;
    }
    else{
        return 1; // new url found
    }
}

int lookup(Hashtable * t, char * url, pthread_rwlock_t * rwlock){
    
    ENTRY e, *ep;

    pthread_rwlock_rdlock(rwlock);
    e.key = url;
    hsearch_r(e, FIND, &ep, &t->htab);
    if(ep){
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
    t = NULL;
    return 1;
}