#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <getopt.h>
#include <curl/curl.h>
#include <sys/types.h>
#include "catpng.h" //for concatenatePNG()

#define ECE252_HEADER "X-Ece252-Fragment: "
#define BUF_INC 524288

typedef struct recv_buf2{
    U8 * buf;
    size_t size;
    size_t max_size;
    int seq;
} RECV_BUF;

typedef struct queue_struct {
    node* head;
    node* tail;
} queue;

typedef struct node_struct {
    U8 data[10000];
    node* next_node;
} node;

size_t header_cb_curl(char *p_recv, size_t size, size_t nmemb, void *userdata);
size_t write_cb_curl3(char *p_recv, size_t size, size_t nmemb, void *p_userdata);
int recv_buf_init(RECV_BUF *ptr, size_t max_size);
int recv_buf_cleanup(RECV_BUF *ptr);

int num_iter = 1;

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
    void *p = NULL;
    
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

int checkArray(int * images, int n){
    for(int i = 0; i < n; ++i){
        if(images[i] == -1){
            return 1;
        }
    }
    return 0;
}

int image_producer() {

}

int image_processor() {
     
}

int main(int argc, char ** argv){
    int buffer_size;
    int num_p;
    int num_c;
    int c_sleep;
    int image_id;
    queue buffer;

    int shmid;
    pid_t pid;
    
    if (argc < 6) {
        fprintf(stderr, "Not enough arguments to run paster2 command.\n");
        fprintf(stderr, "./paster2 <B> <P> <C> <X> <N>.\n");
        fprintf(stderr, "<B>: Buffer Size.\n<P>: Number of Producers.\n<C>: Number of Consumers.\n<X>: Number of milliseconds that a consumer sleeps.\n<N>: Image Number.\n");
        return -1;
    }

    buffer_size = atoi(argv[1]);
    num_p = atoi(argv[2]);
    num_c = atoi(argv[3]);
    c_sleep = atoi(argv[4]);
    image_id = argv[5];

    int shmsize = buffer_size * sizeof(node);
    shmid = shmget(IPC_PRIVATE, shmsize, IPC_CREAT | IPC_EXCL | S_IRUSR | S_IWUSR);
    if (shmid == -1 ) {
        fprintf(stderr, "shmget error");
        abort();
    }

    pid = fork();
    if (pid == 0) { //child process (producer, use this to download png data)

    }
    else { //parent process (use this to wait until downloads done, then we consume the data and write to our png total buffer to be concated later)

    }
    
    queue* buffer;
    buffer = shmat(shmid, NULL, 0);
    if ( buffer == (void *) -1 ) {
        fprintf(stderr, "shmat error");
        abort();
    }

    char* url = strcat("http://ece252-1.uwaterloo.ca:2530/image?img=", image_id);
    char* url_part = strcat(url, "&part=");

    CURL *curl_handle;
    RECV_BUF recv_buf;

    recv_buf_init(&recv_buf, BUF_SIZE);
    curl_global_init(CURL_GLOBAL_DEFAULT);

    /* init a curl session */
    curl_handle = curl_easy_init();

    if (curl_handle == NULL) {
        fprintf(stderr, "curl_easy_init: returned NULL\n");
        return 1;
    }

    int images[n];
    for(int i = 0; i < n; ++i){
        images[i] = -1;
    }

    U8 * img_cat[n];
    if(t == 1){
        while(checkArray(images, n)) {
            if(curl_main(curl_handle, url, &recv_buf, images)){ //new image found
                img_cat[recv_buf.seq] = malloc(recv_buf.size);
                memcpy(img_cat[recv_buf.seq], recv_buf.buf, recv_buf.size);
                recv_buf_init(&recv_buf, BUF_SIZE);
            }
            recv_buf_init(&recv_buf, BUF_SIZE);
        }
    }
    
    concatenatePNG(img_cat, n);

    for(int i = 0; i < n; ++i){
        free(img_cat[i]);
    }

    // image is in recv_buf.buf

    curl_easy_cleanup(curl_handle);
    curl_global_cleanup();
    recv_buf_cleanup(&recv_buf);

}
