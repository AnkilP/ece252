#include "curl_helper.h"

void* retrieve_html(void * arg) {
    // arg should have url, condition var, pointer to hashmap

    CURL *curl;
    CURLcode res;
    char url[256];
    RECV_BUF recv_buf;
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
            if (threadsFetching > 0) {
                pthread_cond_wait( &thread_cond, &thread_mutex );
            } else {
                pthread_mutex_unlock(&thread_mutex);
                pthread_cond_broadcast( &thread_cond );
                //printf("no more threads fetching and null url\n");
                // curl_easy_cleanup(curl);
                // recv_buf_cleanup(&recv_buf);
                pthread_exit(NULL);
            }
        }
        pthread_mutex_unlock(&thread_mutex);
        
        __sync_fetch_and_add(&threadsFetching, 1);
        pthread_mutex_lock(&html_args->frontier_lock);
        pop_from_stack(&url_frontier, &html_args->frontier_lock, url);
        pthread_mutex_unlock(&html_args->frontier_lock);
        //Frontier is empty and there's nothing being fetched, we leave this thread
        if (url[0] == 0) {
            __sync_fetch_and_sub(&threadsFetching, 1);
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
                curl_easy_cleanup(curl);
                recv_buf_cleanup(&recv_buf);
                pthread_cond_broadcast( &thread_cond );
            }
            __sync_fetch_and_sub(&threadsFetching, 1);
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

    CURLM* curlm = NULL;
    CURLMsg* curlmsg = NULL;
    CURLcode return_code = 0;
    int msg_left = 0;
    int http_status_code;
    const char *szUrl;

    int totalRetrievedPng = 0;
    int stillFetching = 0;

    pthread_cond_t thread_cond;
    pthread_mutex_t thread_mutex;

    Hashtable* all_visited_urls = NULL;
    Hashtable* pngTable = NULL;
    url_node* url_frontier = NULL;

    if (gettimeofday(&tv, NULL) != 0)
    {
        perror("gettimeofday");
        abort();
    }
    times[0] = (tv.tv_sec) + tv.tv_usec / 1000000.;

    char * str = "option requires an argument";

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

    curl_global_init(CURL_GLOBAL_ALL);
    curlm = curl_multi_init();
    
    all_visited_urls = (Hashtable * )malloc(sizeof(*all_visited_urls));
    all_visited_urls->htab = &(struct hsearch_data ) { 0 };
    hcreate_r(1000, all_visited_urls->htab);
    all_visited_urls->size = 1000;
    
    pngTable = (Hashtable * )malloc(sizeof(*pngTable));
    pngTable->htab = &(struct hsearch_data ) { 0 };
    hcreate_r(m, pngTable->htab);
    pngTable->size = m;
    
    create_new_stack(&url_frontier, url);

    //Refresh the previous png_url.txt or log file
    FILE *fp = NULL;
    fp = fopen("png_urls.txt", "wb+");
    if (fp == NULL) {
        //perror("fopen");
        return -2;
    }
    fclose(fp);
    if (logUrls) {
        fp = fopen(logFile, "wb+");
        if (fp == NULL) {
            //perror("fopen");
            return -2;
        }
        fclose(fp);
    }

    //Seed URL CURL Kickstart
    RECV_BUF recv_buf;
    easy_handle_multi_init()

    while(1) {
        //Exit conditions
        /*--------------------------------------*/
        if (totalRetrievedPng >= m) {
            break;
        }

        if (isEmpty(url_frontier) && stillFetching == 0) {
            break;
        }
        /*--------------------------------------*/

        if (stillFetching < t) {
            //Start another Curl Multi
            char* url = calloc(256, sizeof(char));
            pop_from_stack(&url_frontier, url);
        }

        if (msg = curl_multi_info_read(curlm, &msgs_left)) {

        }
    }

    //Thread cleanup

    //printf("png: %d\n", totalRetrievedPng);

    delete_hashmap(pngTable); // cleanup everything for hashmap
    delete_hashmap(all_visited_urls);
    cleanup_stack(url_frontier);

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