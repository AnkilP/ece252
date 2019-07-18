#include "curl_helper.h"

typedef struct html{
    char * seedurl;
    Hashtable * t;
    url_node * sentinel;
    url_node * temp_previous;
    Hashtable * pngTable;
} html_struct;

int retrieve_html(void* arg);

//For accessing the url frontier hashmap
pthread_rwlock_t pngStack; //For accessing the png url stack
pthread_rwlock_t visitedStack; //For accessing the visited url stack
pthread_mutex_t frontier_lock; // For accesssing the url frontier

Hashtable* all_visited_url;
Hashtable* pngTable;
url_node* url_frontier;

int totalRetrievedPng = 0;
int threadsFetching = 0;

int retrieve_html(void * arg){
    // arg should have url, condition var, pointer to hashmap

    CURL *curl;
    CURLcode res;
    char url[256];
    RECV_BUF recv_buf;
    html_struct* html_args = (html_struct*) arg;
    int res_data;

    //while(html_data->iter < html_data->m || html_data->iterant != NULL) {
    while(1) {
        //Check to see if we have reached our required png amount
        __sync_fetch_and_add(&totalRetrievedPng, 0);
        if (totalRetrievedPng == html_args->t->size) {
            break;
        }

        //We still havent reached png limit
        //Get a new url from frontier
        pop_from_stack(url_frontier, &frontier_lock, url);
        //Frontier is empty and there's nothing being fetched, we leave this thread
        if (url == NULL) {
            __sync_fetch_and_add(&threadsFetching, 0);
            if (threadsFetching == 0) {
                break;
            }
        }
        else {
            // check to see if the global hashmap (has critical sections) has the url
            int n = lookup(all_visited_url, url, &visitedStack);
            if(n == 0) {
                
                __sync_fetch_and_add(&threadsFetching, 1);
                curl = easy_handle_init(&recv_buf, url);
                res = curl_easy_perform(curl);

                res_data = process_data(curl, &recv_buf, html_args->temp_previous, &frontier_lock, html_args->pngTable, &pngStack);
                add_to_hashmap(all_visited_url, url, &visitedStack);
                __sync_fetch_and_sub(&threadsFetching, 1);
            }
        }
    }

    cleanup(curl, &recv_buf);

    // each thread starts with a specific url in its frontier

    // it then visits that webpage and adds urls to its global url frontier

    pthread_exit(NULL);
}

int main(int argc, char** argv) {
    int c;
    int t = 1;
    int m = 50;
    char logFile[256];
    char url[256];

    pthread_rwlock_init(&pngStack, NULL);
    pthread_rwlock_init(&visitedStack, NULL);
    pthread_mutex_init(&frontier_lock, NULL);
    

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
                else {
                    printf("%s: %d\n", "t", t);
                }
                break;
            case 'm':
                m = strtoul(optarg, NULL, 10);
                if (m <= 0) {
                    fprintf(stderr, "%s: %s 1, 2, or 3 -- 'm'\n", argv[0], str);
                    return -1;
                }
                else {
                    printf("%s: %d\n", "m", m);
                }
                break;
            case 'v':
                strcpy(logFile, optarg);
                if (strlen(logFile) <= 0) {
                    fprintf(stderr, "%s: Please supply log file name -- 'v'\n", argv[0]);
                    return -1;
                }
                else {
                    printf("%s: %s\n", "v", logFile);
                }
                break;
            case 1:
                strcpy(url, optarg);
                if (strlen(url) <= 0) {
                    fprintf(stderr, "%s: Please supply a SEED URL as argument\n", argv[0]);
                    return -1;
                }
                else {
                    printf("%s: %s\n", "seed", url);
                }
            default:
                break;
        }
    }

    pthread_t threads[t];
    int* p_res = malloc(sizeof(int) * t);
    
    all_visited_url = (Hashtable * )malloc(sizeof(*all_visited_url));
    all_visited_url->htab = *(struct hsearch_data *) calloc(1, sizeof(all_visited_url->htab));
    all_visited_url->size = m;
    memset((void *)&all_visited_url->htab, 0, sizeof(all_visited_url->htab));//(struct hsearch_data) calloc(1, sizeof(t->htab));
    hcreate_r(m, &(all_visited_url->htab));
    //add_to_hashmap(all_visited_url, url, &visitedStack); // add the seed url
    
    pngTable = (Hashtable * )malloc(sizeof(*pngTable));
    pngTable->htab = *(struct hsearch_data *) calloc(1, sizeof(pngTable->htab));
    pngTable->size = m;
    memset((void *)&pngTable->htab, 0, sizeof(pngTable->htab));//(struct hsearch_data) calloc(1, sizeof(t->htab));
    hcreate_r(m, &(pngTable->htab));
    
    url_node * sentinel = create_new_stack();
    url_frontier = add_to_stack(sentinel, url, &frontier_lock);
    //populate html_struct
    html_struct * html_args = (html_struct *)malloc(sizeof(html_struct));
    html_args->seedurl = url;
    html_args->sentinel = sentinel;
    html_args->t = all_visited_url;
    html_args->temp_previous = url_frontier;
    html_args->pngTable = pngTable;

    // start threads
    for (int i = 0; i < t; i++) {
        pthread_create(&threads[i], NULL, retrieve_html, (void*) html_args);
    }

    for (int i = 0; i < t; i++) {
        pthread_join(threads[i], (void**) &(p_res[i]));
    }

    //Thread cleanup
    free(threads);
    for (int i = 0; i < t; i++) {
        free(&p_res[i]);
    }

    pthread_rwlock_destroy( &pngStack );
    pthread_rwlock_destroy( &visitedStack);
    pthread_mutex_destroy( &frontier_lock );
    delete_hashmap(pngTable); // cleanup everything for hashmap
    delete_hashmap(all_visited_url);
    cleanup_stack(sentinel); // this takes care of temp_previous as well
    free(html_args);

    return 0;
}