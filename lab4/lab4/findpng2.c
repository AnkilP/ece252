#include "curl_helper.h"
#include <sys/time.h>
#include <sys/wait.h>

void* retrieve_html(void* arg);

int totalRetrievedPng = 0;
int threadsFetching = 0;

pthread_cond_t thread_cond;
pthread_mutex_t thread_mutex;

Hashtable* all_visited_urls = NULL;
Hashtable* pngTable = NULL;
url_node* url_frontier = NULL;

void* retrieve_html(void * arg){
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
        if (totalRetrievedPng >= html_args->requiredPngs) {
            //printf("reached required pngs\n");
            // curl_easy_cleanup(curl);
            // recv_buf_cleanup(&recv_buf);
            pthread_exit(NULL);
        }

        //We still havent reached png limit
        //Get a new url from frontier
        memset(url, 0, 256);

        pthread_mutex_lock(&thread_mutex);
        while(isEmpty(url_frontier)) {
            pthread_cond_wait( &thread_cond, &thread_mutex );
            if (isEmpty(url_frontier) && threadsFetching == 0) {
                pthread_mutex_unlock(&thread_mutex);
                pthread_cond_broadcast( &thread_cond );
                //printf("no more threads fetching and null url\n");
                // curl_easy_cleanup(curl);
                // recv_buf_cleanup(&recv_buf);
                pthread_exit(NULL);
            }
        }
        pthread_mutex_unlock(&thread_mutex);

        pthread_mutex_lock(&html_args->frontier_lock);
        pop_from_stack(&url_frontier, &html_args->frontier_lock, url);
        pthread_mutex_unlock(&html_args->frontier_lock);

        //Frontier is empty and there's nothing being fetched, we leave this thread
        if (url[0] == 0) {
            if (threadsFetching == 0) {
                //printf("no more threads fetching and null url\n");
                // curl_easy_cleanup(curl);
                // recv_buf_cleanup(&recv_buf);
                pthread_exit(NULL);
            }
        }
        else {
            // check to see if the global hash table (has critical sections) has the url
            // Make sure no one is currently writting to the hash table
            pthread_rwlock_wrlock(&html_args->visitedStack);
            int n = lookup(&all_visited_urls, url);
            pthread_rwlock_unlock(&html_args->visitedStack);
            //printf("%d\n", n);
            if(n == 0) {
                //printf("fetching from %s\n", url);
                __sync_fetch_and_add(&threadsFetching, 1);
                curl = easy_handle_init(&recv_buf, url);
                res = curl_easy_perform(curl);
                res_data = process_data(curl, &recv_buf, html_args, &url_frontier, &pngTable, &all_visited_urls, url);
                if (res_data < 0) {
                    //printf("process data failed\n");
                }
                else if (res_data == 1) {
                    //unique png file
                    __sync_fetch_and_add(&totalRetrievedPng, 1);
                }
                add_to_hashmap(&all_visited_urls, url, &html_args->visitedStack);
                //print_stack(html_args->url_frontier, &html_args->frontier_lock);
                __sync_fetch_and_sub(&threadsFetching, 1);
                curl_easy_cleanup(curl);
                recv_buf_cleanup(&recv_buf);
                pthread_cond_broadcast( &thread_cond );
            }
        }
    }
}

int main(int argc, char** argv) {
    int c;
    int t = 1;
    int m = 50;
    char logFile[256];
    char url[256];
    int logUrls = 0;

    double times[2];
    struct timeval tv;

    //For accessing the url frontier hashmap
    pthread_rwlock_t pngStack; //For accessing the png url stack
    pthread_rwlock_t visitedStack; //For accessing the visited url stack
    pthread_mutex_t frontier_lock; // For accesssing the url frontier

    if (gettimeofday(&tv, NULL) != 0)
    {
        perror("gettimeofday");
        abort();
    }
    times[0] = (tv.tv_sec) + tv.tv_usec / 1000000.;

    pthread_rwlock_init(&pngStack, NULL);
    pthread_rwlock_init(&visitedStack, NULL);
    pthread_mutex_init(&frontier_lock, NULL);
    pthread_cond_init(&thread_cond, NULL);
    pthread_mutex_init(&thread_mutex, NULL);
    

    char * str = "option requires an argument";  
    curl_global_init(CURL_GLOBAL_DEFAULT);  

    // read input command line arguments
    while (argc > 1 && (c = getopt (argc, argv, "-t:m:v:")) != -1) {
        switch (c) {
            case 't':
                t = strtoul(optarg, NULL, 10);
                if (t <= 0) {
                    //fprintf(stderr, "%s: %s > 0 -- 't'\n", argv[0], str);
                    return -1;
                }
                else {
                    //printf("%s: %d\n", "t", t);
                }
                break;
            case 'm':
                m = strtoul(optarg, NULL, 10);
                if (m <= 0) {
                    //fprintf(stderr, "%s: %s 1, 2, or 3 -- 'm'\n", argv[0], str);
                    return -1;
                }
                else {
                    //printf("%s: %d\n", "m", m);
                }
                break;
            case 'v':
                strcpy(logFile, optarg);
                if (strlen(logFile) <= 0) {
                    //fprintf(stderr, "%s: Please supply log file name -- 'v'\n", argv[0]);
                    return -1;
                }
                else {
                   //printf("%s: %s\n", "v", logFile);
                    logUrls = 1;
                }
                break;
            case 1:
                strcpy(url, optarg);
                if (strlen(url) <= 0) {
                    //fprintf(stderr, "%s: Please supply a SEED URL as argument\n", argv[0]);
                    return -1;
                }
                else {
                    //printf("%s: %s\n", "seed", url);
                }
            default:
                break;
        }
    }

    pthread_t threads[t];
    int p_res[t];
    
    all_visited_urls = (Hashtable * )malloc(sizeof(*all_visited_urls));
    all_visited_urls->htab = &(struct hsearch_data ) { 0 };
    hcreate_r(1000, all_visited_urls->htab);
    all_visited_urls->size = 1000;
    //memset((void *)&all_visited_url->htab, 0, sizeof(all_visited_url->htab));//(struct hsearch_data) calloc(1, sizeof(t->htab));
    //add_to_hashmap(all_visited_url, url, &visitedStack); // add the seed url
    
    pngTable = (Hashtable * )malloc(sizeof(*pngTable));
    pngTable->htab = &(struct hsearch_data ) { 0 };
    hcreate_r(m, pngTable->htab);
    pngTable->size = m;
    //memset((void *)&pngTable->htab, 0, sizeof(pngTable->htab));//(struct hsearch_data) calloc(1, sizeof(t->htab));
    
    create_new_stack(&url_frontier, url);
    //populate html_struct
    html_struct * html_args = (html_struct *)malloc(sizeof(html_struct));
    html_args->seedurl = url;
    html_args->logUrls = logUrls;
    html_args->logFile = logFile;
    html_args->requiredPngs = m;
    html_args->all_visited_urls = all_visited_urls;
    html_args->png_urls = pngTable;
    html_args->url_frontier = url_frontier;
    html_args->frontier_lock = frontier_lock;
    html_args->pngStack = pngStack;
    html_args->visitedStack = visitedStack;

    // start threads
    for (int i = 0; i < t; i++) {
        pthread_create(&threads[i], NULL, retrieve_html, (void*) html_args);
    }

    for (int i = 0; i < t; i++) {
        pthread_join(threads[i], (void**) &(p_res[i]));
        //printf("pthread exitted\n");
    }

    //Thread cleanup

    //printf("png: %d\n", totalRetrievedPng);

    delete_hashmap(pngTable); // cleanup everything for hashmap
    delete_hashmap(all_visited_urls);
    cleanup_stack(url_frontier);

    pthread_rwlock_destroy( &pngStack );
    pthread_rwlock_destroy( &visitedStack);
    pthread_mutex_destroy( &frontier_lock );

    free(html_args);
    curl_global_cleanup();

    if (gettimeofday(&tv, NULL) != 0)
    {
        perror("gettimeofday");
        abort();
    }
    times[1] = (tv.tv_sec) + tv.tv_usec / 1000000.;
    printf("findpng2 execution time: %.6lf seconds\n", times[1] - times[0]);

    return 0;
}