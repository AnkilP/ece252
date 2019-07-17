#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <sync.h>
#include <curl/curl.h>
#include <pthread.h>
#include "hashmap.h"

typedef struct html{
    int m;
    int iter;
    char * seedurl;
    Hashtable * t;
    url_node * sentinel;
    url_node * temp_previous;
} html_struct;

void* retrieve_html(void* arg);

//For accessing the url frontier hashmap
pthread_rwlock_t pngStack; //For accessing the png url stack
pthread_rwlock_t visitedStack; //For accessing the visited url stack
pthread_rwlock_t frontier_lock;

Hashtable* all_visited_url;
Hashtable* png_url;
url_node* url_frontier;

int totalRetrievedPng = 0;

void * retrieve_html(void * arg){
    // arg should have url, condition var, pointer to hashmap

    CURL *curl;
    CURLcode res;
    char url[256];
    RECV_BUF recv_buf;
    int required_png = (int) arg;
    int pngnum;
    //while(html_data->iter < html_data->m || html_data->iterant != NULL) {
    while(1) {
        //Check to see if we have reached our required png amount
        __sync_fetch_and_add(&totalRetrievedPng, 0);
        if (totalRetrievedPng == required_png) {
            break;
        }

        //We still havent reached png limit
        //Get a new url from frontier
        pop_from_stack(url_frontier, &frontier_lock, url);

        //Frontier is empty, we leave this thread
        if (url == NULL) {
            break;
        }

        // check to see if the global hashmap (has critical sections) has the url
        if(add_to_hashmap(all_visited_url, url, &visitedStack, &(html_data->iter)) == 1) {
            curl = easy_handle_init(&recv_buf, url);
            res = curl_easy_perform(curl);

            process_data(curl, &recv_buf, html_data->temp_previous, &frontier_lock); // adds to the local stack
        }
    }

    cleanup(curl, &recv_buf);

    // each thread starts with a specific url in its frontier

    // it then visits that webpage and adds urls to its global url frontier

    return;
}

int writeToFile() {

}


int main(int argc, char** argv) {
    int c;
    int t = 1;
    int m = 50;
    char logFile[256];
    char url[256];

    pthread_rwlock_init(&pngStack, NULL);
    pthread_rwlock_init(&visitedStack, NULL);
    pthread_rwlock_init(&frontier_lock, NULL);

    char * str = "option requires an argument";  
    curl_global_init(CURL_GLOBAL_NOTHING);  

    // read input command line arguments
    while (argc > 1 && (c = getopt (argc, argv, "-t:m:v:")) != -1) {
        switch (c) {
            case 't':
                t = strtoul(optarg, NULL, 10);
                if (t <= 0) {
                    fprintf(stderr, "%s: %s > 0 -- 't'\n", argv[0], str);
                    return -1;
                }
                break;
            case 'm':
                m = strtoul(optarg, NULL, 10);
                if (m <= 0) {
                    fprintf(stderr, "%s: %s 1, 2, or 3 -- 'm'\n", argv[0], str);
                    return -1;
                }
                break;
            case 'v':
                strcpy(logFile, optarg);
                if (strlen(logFile) <= 0) {
                    fprintf(stderr, "%s: Please supply log file name -- 'v'\n", argv[0]);
                    return -1;
                }
                break;
            case 1:
                strcpy(url, optarg);
                if (strlen(url) <= 0) {
                    fprintf(stderr, "%s: Please supply a SEED URL as argument\n", argv[0]);
                    return -1;
                }
            default:
                break;
        }
    }

    pthread_t* threads = malloc(sizeof(pthread_t) * t);
    int* p_res = malloc(sizeof(int) * t);
    
    create_hash_map(tableau, m); // create global hashmap
    add_to_hashmap(tableau, url, &rwlock, &iter);

    url_node * sentinel = (url_node * )malloc(sizeof(url_node));
    url_node * temp_previous = add_to_stack(sentinel, url, &frontier_lock);
    //populate html_struct
    html_struct * html_args = (html_struct *)malloc(sizeof(html_struct));
    html_args->iter = 1;
    html_args->m = m;
    html_args->seedurl = url;
    html_args->sentinel = sentinel;
    html_args->t = tableau;
    html_args->temp_previous = temp_previous;

    // start threads
    for (int i = 0; i < t; i++) {
        pthread_create(&threads[i], NULL, retrieve_html, (void*) html_args);
    }

    for (int i = 0; i < t; i++) {
        pthread_join(&threads[i], (void**) &(p_res[i]));
    }

    //Thread cleanup
    free(threads);
    for (int i = 0; i < t; i++) {
        free(&p_res[i]);
    }

    delete_hashmap(url_frontier); // cleanup everything for hashmap
    free(pngStack);
    free(visitedStack);
    free(html_args);
    cleanup_stack(sentinel);
}