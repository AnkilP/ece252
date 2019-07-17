#include "hashmap.h"

int create_hash_map(hashmapz * t, int size){
    if(size > 0){
        t->size = size;
    }
    else{
        return 0;
    }
    hcreate_r(size, &(t->htab));
    return 1;
}

int add_to_hashmap(hashmapz * t, char * url, pthread_rwlock_t * rwlock, int * iter){
    t->e.key = url;
    pthread_rwlock_wrlock( rwlock );
    hsearch_r(t->e, FIND, &t->ep, &t->htab); // populate ep with the value
    // sem_post(web_protect);
    if(!ep){
        this->e.data = (void *) 1;
        // sem_wait(web_protect);
        hsearch_r(t->e, ENTER, &t->ep, &t->htab);
        *iter++;
        pthread_rwlock_unlock( rwlock );
        if(t->ep == NULL){
            fprintf(stderr, "hashmap entry failed");
            return 0;
        }
        else{
            return 1; // new url found
        }
    }
    else{
        pthread_rwlock_unlock( rwlock );
        return 2; // url already in hashmap
    }
}

int lookup(hashmapz * t, char * url, pthread_rwlock_t * rwlock){
    pthread_rwlock_rdlock(rwlock);
    t->e.key = url;
    hsearch_r(t->e, FIND, &t->ep, &t->htab);
    if(ep){
        pthread_rwlock_unlock(rwlock);
        return 1;
    }
    else{
        pthread_rwlock_unlock(rwlock);
        return 0;
    }
}

int delete_hashmap(hashmapz * t){
    hdestroy_r(&t->htab);
    free(t);
}