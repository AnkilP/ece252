#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <search.h>
#include <pthread.h>
#include <string.h>

typedef struct node {
    struct node * forward;
    struct node * backward;
    char url[256];
} url_node;

url_node * create_new_stack();
url_node * add_to_stack(url_node * previous, char * url, pthread_mutex_t * frontier_lock);
void pop_from_stack(url_node * htmlz, pthread_mutex_t * frontier_lock, char * url);
int cleanup_stack(url_node * head);
