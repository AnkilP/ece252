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

void create_new_stack(url_node ** stack, char * url);
void add_to_stack(url_node ** stack, char * url);
int pop_from_stack(url_node ** stack, char * url);
int cleanup_stack(url_node * stack);
int fetch_from_stack(url_node* stack, char * url);
void print_stack(url_node* stack);
int isEmpty(url_node* stack);