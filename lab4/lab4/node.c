#include "node.h"

void add_to_stack(url_node ** stack, char * url, pthread_mutex_t * frontier_lock){
    url_node * e = NULL;
    e = (url_node *) malloc(sizeof(url_node));
    if (e == NULL) {
        //fprintf(stderr, "malloc() failed\n");
        exit(EXIT_FAILURE);
    }

    // e->url = url;
    char* tempurl = (char * ) malloc(strlen(url) + 1); // mem leak?
    strcpy(tempurl, url);
    e->url = tempurl;
    pthread_mutex_lock(frontier_lock);
    e->next = *stack;
    *stack = e;
    pthread_mutex_unlock(frontier_lock);
}

void create_new_stack(url_node ** stack, char* url) {
    url_node * e = NULL;
    e = (url_node *) malloc(sizeof(url_node));
    if (e == NULL) {
        //fprintf(stderr, "malloc() failed\n");
        exit(EXIT_FAILURE);
    }

    char* nodeurl = (char*) malloc(strlen(url) + 1);
    strcpy(nodeurl, url);
    e->url = nodeurl;
    e->next = NULL;
    (*stack) = e;
}

int cleanup_stack(url_node * stack) {
    if(stack == NULL){
        return 0;
    }

    url_node * temp = NULL;
    while(stack != NULL) {
        temp = stack;
        stack = stack->next;
        free(temp->url);
        free(temp);
    }
    return 1;
}

int pop_from_stack(url_node ** stack, pthread_mutex_t * frontier_lock, char * url){
    //char  * temp = htmlz->url;
    if ((*stack) == NULL) {
        return 0;
    }
    strcpy(url, (*stack)->url);
    url_node * temp = NULL;
    //pthread_mutex_lock(frontier_lock);
    temp = (*stack);
    (*stack) = (*stack)->next;
    free(temp->url);
    free(temp);
    //pthread_mutex_unlock(frontier_lock);
    return 1;
}

int fetch_from_stack(url_node * stack, char* url) {
    if (stack == NULL) {
        return 0;
    }
    strcpy(url, stack->url);
    return 1;
}

void print_stack(url_node* stack, pthread_mutex_t * frontier_lock) {
    if (stack == NULL) {
        return;
    }
    url_node* temp = stack;
    while(temp != NULL) {
        //printf("%s -> ", temp->url);
        temp = temp->next;
    }
    //printf("\n");
}

int isEmpty(url_node* stack) {
    return stack == NULL;
}