#include "hashmap.h"

sem_t web_protect; // used to block access to the hashmap

typedef struct html_struct{
    char * seedurl;
    pthread_cond_t cancellation_token;
    hashmapz * t;
};

void * retrieve_html(void * arg){
    // arg should have url

    CURL *curl;
    CURLcode res;
    char url[256];
    RECV_BUF recv_buf;
    
    html_struct * html_data = (html_struct *)arg;

    //local buffer to hold 50 urls - this performs a dfs search
    url_node * sentinel = (url_nonde * )malloc(sizeof(url_node));
    url_node * temp_previous = add_to_stack(sentinel, html->url);
    url_node * iterant;

    // cancellation token is triggered when the frontier is empty
    while(!html_data->cancellation_token){

        // pop from frontier
        url = pop_from_stack(temp_previous, iterant);

        // check to see if the global hashmap (has critical sections) has the url
        if(add_to_hashmap(html_data->t, url, web_protect) == 1){
            curl = easy_handle_init(&recv_buf, url);
            res = curl_easy_perform(curl);

            process_data(curl, recv_buf, iterant); // adds to the local stack
        }

        temp_previous = iterant;

    }

    cleanup_stack(sentinel);
    cleanup(curl, &recv_buf);

    // each thread starts with a specific url in its frontier

    // it then visits that webpage and adds urls to its own thread-frontier
    
    
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

    create_hash_map(tableau, m); // create global hashmap
    add_to_hashmap(tableau, url, web_protect);
    // start threads


    delete_hashmap(tableau); // cleanup everything for hashmap

}