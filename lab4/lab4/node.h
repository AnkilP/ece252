#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <search.h>
#include <pthread.h>
#include <string.h>

typedef struct node{
    struct node * forward;
    struct node * backward;
    char url[256];
} url_node;

url_node * create_new_stack();
<<<<<<< HEAD
static url_node * add_to_stack(url_node * previous, char * url);
char * pop_from_stack(url_node * htmlz);
int cleanup_stack(url_node * head);
int num_of_elements();
=======
url_node * add_to_stack(url_node * previous, char * url, pthread_rwlock_t * frontier_lock);
void pop_from_stack(url_node * htmlz, pthread_rwlock_t * frontier_lock, char * url);
int cleanup_stack(url_node * head);
>>>>>>> 5a707beb8515e3c48b2d6d3277df27f6ccb7bc54
