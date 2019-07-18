#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <search.h>
#include <pthread.h>
#include <string.h>

typedef struct node {
    struct node * next;
    char * url;
} url_node;

url_node * create_new_stack();
void add_to_stack(url_node ** stack, char * url, pthread_mutex_t * frontier_lock);
int pop_from_stack(url_node ** stack, pthread_mutex_t * frontier_lock, char * url);
int cleanup_stack(url_node * stack);
int fetch_from_stack(url_node* stack, char * url);
void print_stack(url_node* stack, pthread_mutex_t * frontier_lock);