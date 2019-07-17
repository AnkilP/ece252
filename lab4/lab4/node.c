#include "node.h"

static url_node * add_to_stack(url_node * previous, char * url){
    url_node * e;

    e = (url_node *) malloc(sizeof(url_node));
    if (e == NULL) {
        fprintf(stderr, "malloc() failed\n");
        exit(EXIT_FAILURE);
    }

    insque(e, previous);
    e->url = url;
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

char * pop_from_stack(url_node * htmlz, url_node * previous){
    char  * temp = htmlz->url;
    previous = htmlz->backward;
    remque(htmlz);
    return temp;
}



