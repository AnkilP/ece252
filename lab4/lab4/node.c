#include "node.h"

url_node * add_to_stack(url_node * previous, char * url, pthread_mutex_t * frontier_lock){
    url_node * e;

    e = (url_node *) malloc(sizeof(url_node));
    if (e == NULL) {
        fprintf(stderr, "malloc() failed\n");
        exit(EXIT_FAILURE);
    }

    // e->url = url;
    
    strcpy(e->url, url);
    pthread_mutex_lock(frontier_lock);
    insque(e, previous);
    pthread_mutex_lock(frontier_lock);
    previous = e;
    return e; // incrementing stack pointer
}

url_node * create_new_stack(){
    url_node * e;

    e = (url_node *) malloc(sizeof(url_node));
    if (e == NULL) {
        fprintf(stderr, "malloc() failed\n");
        exit(EXIT_FAILURE);
    }

    insque(e, NULL);
    return e;
}

int cleanup_stack(url_node * head){
    url_node * elem = head;
    if(head == NULL){
        return 0;
    }
    do {
        elem = elem->forward;
        remque(head);
        head = elem;
    } while (elem != NULL);

    return 1;
}

void pop_from_stack(url_node * htmlz, pthread_mutex_t * frontier_lock, char * url){
    //char  * temp = htmlz->url;
    strcpy(url, htmlz->url);
    url_node * temperoo = htmlz;
    htmlz = htmlz->backward;
    pthread_mutex_lock(frontier_lock);
    remque(temperoo);
    pthread_mutex_unlock(frontier_lock);
}



