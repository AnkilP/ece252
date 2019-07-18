#include "node.h"

void add_to_stack(url_node ** stack, char * url, pthread_mutex_t * frontier_lock){
    url_node * e = NULL;
    e = (url_node *) malloc(sizeof(url_node));
    if (e == NULL) {
        fprintf(stderr, "malloc() failed\n");
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

url_node * create_new_stack(char* url){
    url_node * e = (url_node *) malloc(sizeof(url_node));
    if (e == NULL) {
        fprintf(stderr, "malloc() failed\n");
        exit(EXIT_FAILURE);
    }
    char* nodeurl = malloc(strlen(url)  + 1);
    strcpy(nodeurl, url);
    e->url = nodeurl;
    e->next = NULL;
    return e;
}

int cleanup_stack(url_node * head) {
    if(head == NULL){
        return 0;
    }
    url_node *elem = head;
    while(head->next != NULL) {
        head = head->next;
        free(elem->url);
        free(elem);
        elem = head;
    }
    free(head->url);
    free(head);
    return 1;
}

int pop_from_stack(url_node ** stack, pthread_mutex_t * frontier_lock, char * url){
    //char  * temp = htmlz->url;
    if (*stack == NULL) {
        return 0;
    }
    strcpy(url, (*stack)->url);
    url_node * temp = *stack;
    pthread_mutex_lock(frontier_lock);
    *stack = (*stack)->next;
    free(temp->url);
    free(temp);
    pthread_mutex_unlock(frontier_lock);
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
        printf("%s -> ", temp->url);
        temp = temp->next;
    }
    printf("\n");
}