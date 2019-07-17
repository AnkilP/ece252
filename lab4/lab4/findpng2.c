#include "hashmap.h"

pthread_rwlock_t rwlock;

typedef struct html{
    int m;
    int iter;
    char * seedurl;
    hashmapz * t;
    url_node * sentinel;
    url_node * temp_previous;
} html_struct;

void * retrieve_html(void * arg){
    // arg should have url, condition var, pointer to hashmap

    CURL *curl;
    CURLcode res;
    char url[256];
    RECV_BUF recv_buf;
    
    html_struct * html_data = (html_struct *)arg;

    //local buffer to hold 50 urls - this performs a dfs search

    // cancellation token is triggered when the frontier is empty
    while(html_data->iter < html_data->m || html_data->iterant != NULL){

        // pop from frontier
        url = pop_from_stack(html_data->temp_previous);

        // check to see if the global hashmap (has critical sections) has the url
        if(add_to_hashmap(html_data->t, url, &rwlock, &(html_data->iter)) == 1){
            curl = easy_handle_init(&recv_buf, url);
            res = curl_easy_perform(curl);

            process_data(curl, recv_buf, html_data->temp_previous); // adds to the local stack
        }
    }

    cleanup(curl, &recv_buf);

    // each thread starts with a specific url in its frontier

    // it then visits that webpage and adds urls to its global url frontier
    
    
}


int main(int argc, char** argv) {
    int c;
    int t = 1;
    int m = 50;
    char logFile[256];
    char* seedurl;

    CURL *curl_handle;
    CURLcode res;
    char url[256];
    RECV_BUF recv_buf;

    pthread_rwlock_init( &rwlock, NULL );

    char * str = "option requires an argument";  
    curl_global_init(CURL_GLOBAL_NOTHING);  

    hashmapz * tableau = (hashmapz *)malloc(sizeof(hashmapz));

    // read input command line arguments
    while (argc>1 && (c = getopt (argc, argv, "t:m:v:")) != -1) {
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
                    fprintf(stderr, "%s: Please supply log file name -- 'v'\n", argv[0], str);
                    return -1;
                }
                break;
            default:
                break;
        }
    }

    if (argv[optind] == NULL) {
        printf("SEED URL missing\n");
        return -1;
    }
    else {
        strcpy(url, argv[optind]);
    }

    pthread_t* threads = malloc(sizeof(pthread_t) * t);
    int* p_res = malloc(sizeof(int) * t);
    
    create_hash_map(tableau, m); // create global hashmap
    add_to_hashmap(tableau, url, web_protect);

    url_node * sentinel = (url_nonde * )malloc(sizeof(url_node));
    url_node * temp_previous = add_to_stack(sentinel, html->url);
    //populate html_struct
    html_struct * html_args = (html_struct *)malloc(sizeof(html_struct));
    html_args->iter = 0;
    html_args->m = m;
    html_args->seedurl = url;
    html_args->sentinel = sentinel;
    html_args->t = tableau;
    html_args->temp_previous = temp_previous;

    // start threads
    for (int i = 0; i < t; i++) {
        pthread_create(threads[i], NULL, retrieve_html, (void*) html_args);
    }

    for (int i = 0; i < t; i++) {
        pthread_join(threads[i], (void**) &(p_res[i]);
    }

    //Thread cleanup
    free(threads);
    for (int i = 0; i < t; i++) {
        free(p_res[i]);
    }

    delete_hashmap(tableau); // cleanup everything for hashmap
    free(html_args);
    cleanup_stack(sentinel);
}