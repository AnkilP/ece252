#include <stdlib.h>                                                                                              
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <getopt.h>
#include <curl/curl.h>
#include <sys/types.h>
#include "catpng.h" //for concatenatePNG()
#include <pthread.h>

#define ECE252_HEADER "X-Ece252-Fragment: "
#define BUF_SIZE 1048576
#define BUF_INC 524288
#define IMG_URL "http://ece252-1.uwaterloo.ca:2520/image?img=1"

typedef struct recv_buf2{
    U8 * buf;
    size_t size;
    size_t max_size;
    int seq;
} RECV_BUF;

typedef struct thread_arguments{
    int server;
    int n;
    int * images;
    U8 ** img_container;
} th_args;

size_t header_cb_curl(char *p_recv, size_t size, size_t nmemb, void *userdata);
size_t write_cb_curl3(char *p_recv, size_t size, size_t nmemb, void *p_userdata);
int recv_buf_init(RECV_BUF *ptr, size_t max_size);
int recv_buf_cleanup(RECV_BUF *ptr);

int num_iter = 1;
pthread_mutex_t lock;

#define max(a, b) \
   ({ __typeof__ (a) _a = (a); \
       __typeof__ (b) _b = (b); \
     _a > _b ? _a : _b; })

/**
 * @brief  cURL header call back function to extract image sequence number from 
 *         http header data. An example header for image part n (assume n = 2) is:
 *         X-Ece252-Fragment: 2
 * @param  char *p_recv: header data delivered by cURL
 * @param  size_t size size of each memb
 * @param  size_t nmemb number of memb
 * @param  void *userdata user defined data structurea
 * @return size of header data received.
 * @details this routine will be invoked multiple times by the libcurl until the full
 * header data are received.  we are only interested in the ECE252_HEADER line 
 * received so that we can extract the image sequence number from it. This
 * explains the if block in the code.
 */
size_t header_cb_curl(char *p_recv, size_t size, size_t nmemb, void *userdata)
{
    int realsize = size * nmemb;
    RECV_BUF *p = userdata;
    
    if (realsize > strlen(ECE252_HEADER) &&
	strncmp(p_recv, ECE252_HEADER, strlen(ECE252_HEADER)) == 0) {

        /* extract img sequence number */
	p->seq = atoi(p_recv + strlen(ECE252_HEADER));

    }
    return realsize;
}


/**
 * @brief write callback function to save a copy of received data in RAM.
 *        The received libcurl data are pointed by p_recv, 
 *        which is provided by libcurl and is not user allocated memory.
 *        The user allocated memory is at p_userdata. One needs to
 *        cast it to the proper struct to make good use of it.
 *        This function maybe invoked more than once by one invokation of
 *        curl_easy_perform().
 */

size_t write_cb_curl3(char *p_recv, size_t size, size_t nmemb, void *p_userdata)
{
    size_t realsize = size * nmemb;
    RECV_BUF *p = (RECV_BUF *)p_userdata;
 
    if (p->size + realsize + 1 > p->max_size) {/* hope this rarely happens */ 
        /* received data is not 0 terminated, add one byte for terminating 0 */
        size_t new_size = p->max_size + max(BUF_INC, realsize + 1);   
        U8 *q = realloc(p->buf, new_size);
        if (q == NULL) {
            perror("realloc"); /* out of memory */
            return -1;
        }
        p->buf = q;
        p->max_size = new_size;
    }

    memcpy(p->buf + p->size, p_recv, realsize); /*copy data from libcurl*/
    p->size += realsize;
    p->buf[p->size] = 0;

    return realsize;
}


int recv_buf_init(RECV_BUF *ptr, size_t max_size)
{
    void * p = NULL;
    
    if (ptr == NULL) {
        return 1;
    }

    p = malloc(max_size);
    if (p == NULL) {
	return 2;
    }
    
    ptr->buf = p;
    ptr->size = 0;
    ptr->max_size = max_size;
    ptr->seq = -1;              /* valid seq should be positive */
    return 0;
}

void recv_buf_rest(RECV_BUF *ptr){
    ptr->buf = memset(ptr->buf, 0, ptr->size);
    ptr->size = 0;
    ptr->seq = -1;
}

int recv_buf_cleanup(RECV_BUF *ptr)
{
    if (ptr == NULL) {
	return 1;
    }
    
    free(ptr->buf);
    ptr->size = 0;
    ptr->max_size = 0;
    return 0;
}

int curl_main(CURL * curl_handle, char * url, RECV_BUF * recv_buf, int * img_ptr){
    CURLcode res;
    /* specify URL to get */
    curl_easy_setopt(curl_handle, CURLOPT_URL, url);

    /* register write call back function to process received data */
    curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, write_cb_curl3); 
    /* user defined data structure passed to the call back function */
    curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, (void *)recv_buf);

    /* some servers requires a user-agent field */
    curl_easy_setopt(curl_handle, CURLOPT_USERAGENT, "libcurl-agent/1.0");
    
    curl_easy_setopt(curl_handle, CURLOPT_HEADERFUNCTION, header_cb_curl); 
    curl_easy_setopt(curl_handle, CURLOPT_HEADERDATA, (void *)recv_buf);

        /* get it! */
    res = curl_easy_perform(curl_handle);

    if( res != CURLE_OK) {
        fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
    } 
    // else {
    // printf("%lu bytes received in memory %p\n", recv_buf->size, recv_buf->buf);
    // }

    if(img_ptr[recv_buf->seq] == -1){
        img_ptr[recv_buf->seq] = 1;  
        printf("Image Number: %i\n", num_iter++);
        return 1; // new image found
    }
    else{
        return 0; // duplicate found
    }
}

int curlthreadz(CURL * curl_handle, char * url, RECV_BUF * recv_buf){
    CURLcode res;
    /* specify URL to get */
    curl_easy_setopt(curl_handle, CURLOPT_URL, url);

    /* register write call back function to process received data */
    curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, write_cb_curl3); 
    /* user defined data structure passed to the call back function */
    curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, (void *)recv_buf);

    /* some servers requires a user-agent field */
    curl_easy_setopt(curl_handle, CURLOPT_USERAGENT, "libcurl-agent/1.0");
    
    curl_easy_setopt(curl_handle, CURLOPT_HEADERFUNCTION, header_cb_curl); 
    curl_easy_setopt(curl_handle, CURLOPT_HEADERDATA, (void *)recv_buf);

        /* get it! */
    res = curl_easy_perform(curl_handle);

    if( res != CURLE_OK) {
        fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
        return 0;
    } 
    else {
        printf("%lu bytes received in memory %p\n", recv_buf->size, recv_buf->buf);
        return 1;
    }
}

int checkArray(int * images, int n){
    printf("checkArray: %X\n", images);
    for(int i = 0; i < n; ++i){
        if(images[i] == -1){
            return 1;
        }
    }
    return 0;
}

static void * curl_thread(void * args){
    CURL *curl_handle;
    th_args * attr = (th_args *) args;
    char url[256];
    sprintf(url,"http://ece252-%i.uwaterloo.ca:2520/image?img=%i",attr->server,attr->n);
    RECV_BUF recv_buf;
    recv_buf_init(&recv_buf, BUF_SIZE);

    /* init a curl session */
    curl_handle = curl_easy_init();

    if (curl_handle == NULL) {
        fprintf(stderr, "curl_easy_init: returned NULL\n");
    }
    printf("%i", attr->images);
    int condition_var = checkArray(attr->images, attr->n);
    while(condition_var){
            int error = curlthreadz(curl_handle, url, &recv_buf);
            printf("%i", error);
            pthread_mutex_lock(&lock);
            if(attr->images[recv_buf.seq] == -1){
                attr->images[recv_buf.seq] = 1;  
                printf("Image Number: %i\n", num_iter++);
                attr->img_container[recv_buf.seq] = malloc(recv_buf.size);
                memcpy(attr->img_container[recv_buf.seq], recv_buf.buf, recv_buf.size);
            }            
            condition_var = checkArray(attr->images, attr->n);
            pthread_mutex_unlock(&lock);

            recv_buf_rest(&recv_buf);
        }
    
    curl_easy_cleanup(curl_handle);
    recv_buf_cleanup(&recv_buf);

    return NULL;

}

int main(int argc, char ** argv){
    int c;
    int t = 3;
    int n = 50;

    char * str = "option requires an argument";  
    curl_global_init(CURL_GLOBAL_NOTHING);

    while (argc>1&&(c = getopt (argc, argv, "t:n:")) != -1) {
        switch (c) {
            case 't':
                t = strtoul(optarg, NULL, 10);
                if (t <= 0) {
                    fprintf(stderr, "%s: %s > 0 -- 't'\n", argv[0], str);
                    return -1;
                }
                break;
            case 'n':
                n = strtoul(optarg, NULL, 10);
                if (n <= 0) {
                    fprintf(stderr, "%s: %s 1, 2, or 3 -- 'n'\n", argv[0], str);
                    return -1;
                }
                break;
            default:
                break;
        }
    }

    printf("t,n: %i, %i\n", t, n);
    int images[n];
    for(int i = 0; i < n; ++i){
        images[i] = -1;
    }
    U8 ** img_cat = malloc(n * sizeof(U8 *));


    if(t == 1){
        CURL *curl_handle;
        RECV_BUF recv_buf;
        char url[256];
        strcpy(url, IMG_URL);
        recv_buf_init(&recv_buf, BUF_SIZE);

        /* init a curl session */
        curl_handle = curl_easy_init();

        if (curl_handle == NULL) {
            fprintf(stderr, "curl_easy_init: returned NULL\n");
            return 1;
        }

        while(checkArray(images, n)){
            if(curl_main(curl_handle, url, &recv_buf, images)){ //new image found
                img_cat[recv_buf.seq] = malloc(recv_buf.size);
                memcpy(img_cat[recv_buf.seq], recv_buf.buf, recv_buf.size);
            }
            recv_buf_rest(&recv_buf);
        }
        
        curl_easy_cleanup(curl_handle);
        recv_buf_cleanup(&recv_buf);
    }
    else{
        printf("Entering thread mode\n");
        pthread_t * pids = malloc(t * sizeof(pthread_t));
        th_args  * parameters = (th_args * ) malloc(t * sizeof(th_args));

        if (pthread_mutex_init(&lock, NULL) != 0) 
        { 
            fprintf(stderr, "mutex initialization failed");  
        }

        for(int i = 0; i < t; ++i){
            parameters[i].n = n;
            parameters[i].server = i % 3 + 1;
            parameters[i].images = images;
            parameters[i].img_container = img_cat;
            int error = pthread_create(&pids[i], NULL, curl_thread, (void *) parameters + i);
            if(error != 0){
                fprintf(stderr, "Thread init went wrong");
            }
        }

        for(int i = 0; i < t; ++i){
            pthread_join(pids[i], NULL);
        }
        
        free(parameters);
    }
    
    concatenatePNG(img_cat, n);

    for(int i = 0; i < n; ++i){
        free(img_cat[i]);
    }
    free(images);
    free(img_cat);
    
    pthread_mutex_destroy(&lock); 
    curl_global_cleanup();
}
