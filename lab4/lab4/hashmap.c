#include "hashmap.h"

int create_hash_map(hashmapz * t, int size){
    if(size > 0){
        t->size = size;
    }
    else{
        return 0;
    }
    hcreate(size); // should use hcreate if we want to scale
    return 1;
}

int add_to_hashmap(hashmapz * t, chat * url, sem_t * web_protect, int * iter){
    t->e.key = url;
    sem_wait(web_protect);
    t->ep = hsearch(t->e, FIND);
    // sem_post(web_protect);
    if(!ep){
        this->e.data = (void *) 1;
        // sem_wait(web_protect);
        t->ep = hsearch(this->e, ENTER);
        
        sem_post(web_protect);
        if(t->ep == NULL){
            fprintf(stderr, "hashmap entry failed");
            return 0;
        }
        else{
            return 1; // new url found
        }
    }
    else{
        return 2; // url already in hashmap
    }
}

int delete_hashmap(hashmapz * t){
    hdestroy();
    free(t);
}