#include "curl_helper.h"

int main(int argc, char** argv) {
    int c;
    int t = 1;
    int m = 50;
    char logFile[256];
    char url[256];
    int logUrls = 0;
    int running = 0;

    double times[2];
    struct timeval tv;

    CURLM* curlm = NULL;
    CURLMsg* curlmsg = NULL;
    CURLcode return_code = 0;
    int msg_left = 0;
    int http_status_code;
    char *szUrl;
    char* buf;

    int totalRetrievedPng = 0;
    int stillFetching = 0;
    int res_data = 0;

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

    RECV_BUF recv_buf[t];

    while(1) {
        //Exit conditions
        /*--------------------------------------*/
        if (totalRetrievedPng >= m) {
            //printf("found all png\n");
            break;
        }

        if (isEmpty(url_frontier) && running == 0) {
            //printf("no more frontier + no more running\n");
            break;
        }
        /*--------------------------------------*/

        if (running < t - 1 && !isEmpty(url_frontier)) {
            //Start another Curl Multi
            char* url = calloc(256, sizeof(char));
            pop_from_stack(&url_frontier, url);
            //printf("#%d performing: %s\n", running, url);
            easy_handle_multi_init(curlm, &recv_buf[running++], url);
            add_to_hashmap(&all_visited_urls, url);
        }

        int res = curl_multi_wait(curlm, NULL, 0, MAX_WAIT_MSECS, NULL);
        if(res != CURLM_OK) {
            fprintf(stderr, "error: curl_multi_wait() returned %d\n", res);
            return EXIT_FAILURE;
        }
        curl_multi_perform (curlm, &stillFetching);

        if ((curlmsg = curl_multi_info_read(curlm, &msg_left))) {
            if (curlmsg->msg == CURLMSG_DONE) {
                running--;
                CURL* curl = curlmsg->easy_handle;

                return_code = curlmsg->data.result;
                if(return_code != CURLE_OK) {
                    //fprintf(stderr, "CURL error code: %d\n", curlmsg->data.result);
                    continue;
                }

                // Get HTTP status code
                http_status_code=0;
                szUrl = NULL;
                RECV_BUF buf;
                recv_buf_init(&buf, BUF_SIZE);
                curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_status_code);
                curl_easy_getinfo(curl, CURLINFO_PRIVATE, &(buf.buf));
                curl_easy_getinfo(curl, CURLINFO_EFFECTIVE_URL, &szUrl);

                if(http_status_code==200) {
                    //printf("200 OK for %s\n", szUrl);
                } else {
                    //fprintf(stderr, "GET of %s returned http status code %d\n", szUrl, http_status_code);
                }
                buf.size = strlen(buf.buf);
                res_data = process_data(curl, &buf, &url_frontier, &pngTable, &all_visited_urls, szUrl, logUrls, logFile);
                if (res_data < 0) {
                    //printf("process data failed\n");
                }
                else if (res_data == 1) {
                    //unique png file
                    totalRetrievedPng++;
                }
                //print_stack(html_args->url_frontier, &html_args->frontier_lock);
                recv_buf_cleanup(&buf);
                curl_multi_remove_handle(curlm, curl);
                curl_easy_cleanup(curl);
            }
            else {
                fprintf(stderr, "error: after curl_multi_info_read(), CURLMsg=%d\n", curlmsg->msg);
            }
        }
    }

    //Thread cleanup

    //printf("png: %d\n", totalRetrievedPng);

    delete_hashmap(pngTable); // cleanup everything for hashmap
    delete_hashmap(all_visited_urls);
    cleanup_stack(url_frontier);

    curl_multi_cleanup(curlm);
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