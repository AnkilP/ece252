#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <search.h>
#include <pthread.h>

typedef struct node{
    struct node * forward;
    struct node * backward;
    char * url;
} url_node;

url_node * create_new_stack();
static url_node * add_to_stack(url_node * previous, char * url, pthread_rwlock_t * frontier_lock);
char * pop_from_stack(url_node * htmlz, pthread_rwlock_t * frontier_lock);
int cleanup_stack(url_node * head);