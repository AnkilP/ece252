#include "node.h"

void add_to_stack(url_node * stack, char * url, pthread_mutex_t * frontier_lock){
    url_node * e;

    e = (url_node *) malloc(sizeof(url_node));
    if (e == NULL) {
        fprintf(stderr, "malloc() failed\n");
        exit(EXIT_FAILURE);
    }

    // e->url = url;
    e->url = (char * ) malloc(strlen(url) + 1); // mem leak?
    strcpy(e->url, url);
    pthread_mutex_lock(frontier_lock);
    e->next = stack;
    stack = e;
    pthread_mutex_unlock(frontier_lock);
}

url_node * create_new_stack(char* url){
    url_node * e = (url_node *) malloc(sizeof(url_node));
    if (e == NULL) {
        fprintf(stderr, "malloc() failed\n");
        exit(EXIT_FAILURE);
    }
    char* nodeurl = malloc(strlen(url)+1);
    strcpy(nodeurl, url);
    e->url = nodeurl;
    e->next = NULL;
    return e;
}

int cleanup_stack(url_node * head){
    url_node * elem = head;
    if(head == NULL){
        return 0;
    }
    do {
        elem = elem->next;
        free(head->url);
        free(head);
        head = elem;
    } while (elem != NULL);

    return 1;
}

int pop_from_stack(url_node * htmlz, pthread_mutex_t * frontier_lock, char * url){
    //char  * temp = htmlz->url;
    if (htmlz == NULL) {
        return 0;
    }
    strcpy(url, htmlz->url);
    url_node * temperoo = htmlz;
    pthread_mutex_lock(frontier_lock);
    htmlz = htmlz->next;
    free(temperoo->url);
    free(temperoo);
    pthread_mutex_unlock(frontier_lock);

    return 1;
}