#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <search.h>

typedef struct node{
    struct node * forward;
    struct node * backward;
    char * url;
} url_node;

url_node * create_new_stack();
static url_node * add_to_stack(url_node * previous, char * url);
char * pop_from_stack(url_node * htmlz, url_node * previous);
int cleanup_stack(url_node * head);