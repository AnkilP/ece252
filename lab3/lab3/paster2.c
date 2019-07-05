#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <getopt.h>
#include <curl/curl.h>
#include <sys/types.h>
#include "catpng.h" //for concatenatePNG()
#include <sys/shm.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <semaphore.h>
// #include "shm_stack.h"

#define ECE252_HEADER "X-Ece252-Fragment: "
#define BUF_INC 524288

typedef struct recv_buf2{
    size_t size;
    size_t max_size;
    int seq;
    U8 * buf;
} RECV_BUF;

typedef struct image_stack
{
    int size;               /* the max capacity of the stack */
    int pos;                /* position of last item pushed onto the stack */
    RECV_BUF **items;             /* stack of stored integers */
} RSTACK;


size_t header_cb_curl(char *p_recv, size_t size, size_t nmemb, void *userdata);
size_t write_cb_curl3(char *p_recv, size_t size, size_t nmemb, void *p_userdata);
int recv_buf_init(RECV_BUF *ptr, size_t max_size);
int recv_buf_cleanup(RECV_BUF *ptr);

int init_shm_stack(RSTACK *p, int stack_size);
int sizeof_shm_stack(int size);
RSTACK *create_image_stack(int size);
void destroy_stack(RSTACK *p);
int is_full(RSTACK *p, sem_t * sems);
int is_empty(RSTACK *p);
int push(RSTACK *p, RECV_BUF * item, sem_t * sems);
int pop(RSTACK *p, RECV_BUF *p_item, sem_t * sems);

int num_iter = 0;

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
    RECV_BUF *p = (RECV_BUF *) userdata;
    printf("This is the future of the universe %li\n", p->max_size);
    
    if (realsize > strlen(ECE252_HEADER) &&
	strncmp(p_recv, ECE252_HEADER, strlen(ECE252_HEADER)) == 0) {

        /* extract img sequence number */
        p->seq = atoi(p_recv + strlen(ECE252_HEADER));

    }
    return realsize;
}

int init_shm_stack(RSTACK *p, int stack_size)
{
    if ( p == NULL || stack_size == 0 ) {
        return 1;
    }

    p->size = stack_size;
    p->pos  = -1;
    p->items = (RECV_BUF **) (p + sizeof(RSTACK));
    return 0;
}

int sizeof_shm_stack(int size)
{
    return (sizeof(RSTACK) + sizeof(int) * size);
}

RSTACK *create_image_stack(int size)
{
    int mem_size = 0;
    RSTACK *pstack = NULL;
    
    if ( size == 0 ) {
        return NULL;
    }

    mem_size = sizeof_shm_stack(size);
    pstack = (RSTACK *) malloc(mem_size);

    if ( pstack == NULL ) {
        perror("malloc");
    } else {
        char *p = (char *)pstack;
        pstack->items = (RECV_BUF **) (p + sizeof(RSTACK));
        pstack->size = size;
        pstack->pos  = -1;
    }

    return pstack;
}

void destroy_stack(RSTACK *p)
{
    if ( p != NULL ) {
        free(p);
    }
}

int is_full(RSTACK *p, sem_t * sems)
{
    if ( p == NULL ) {
        return 0;
    }
    sem_wait(&sems[0]);
    return ( p->pos == (p->size -1) );
    sem_post(&sems[0]);
}

int is_empty(RSTACK *p)
{
    if ( p == NULL ) {
        return 0;
    }

    return ( p->pos == -1 );
}

int push(RSTACK *p, RECV_BUF * item, sem_t * sems)
{
    if ( p == NULL ) {
        printf("stack null\n");
        return -1;
    }

    if ( !is_full(p, sems) ) {
        sem_wait(&sems[0]);
        ++(p->pos);
        memcpy(&(p->items[p->pos]), item, sizeof(*item));
        sem_post(&sems[0]);
        return 0;
    } else {
        printf("stack full\n");
        return -1;
    }
}

int pop(RSTACK *p, RECV_BUF *p_item, sem_t * sems)
{
    if ( p == NULL ) {
        return -1;
    }

    if ( !is_empty(p) ) {
        sem_wait(&sems[0]);
        p_item = p->items[p->pos];
        (p->pos)--;
        sem_post(&sems[0]);
        return 0;
    } else {
        return 1;
    }
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
        U8 *q = (U8 *) realloc(p->buf, new_size);
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
    
    ptr->buf = (U8 *) p;
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

int curl_main(CURL * curl_handle, char * url, RECV_BUF * recv_buf){
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
        return -1;
    } 

    return 0;
}

int image_producer(char* url, CURL * curl_handle, RSTACK * p, int c_sleep, sem_t * sems) {
    RECV_BUF recv_buf;
    recv_buf_init(&recv_buf, BUF_INC);

    printf("grabbing from: %s\n", url);
    curl_main(curl_handle, url, &recv_buf);

    while(!push(p, &recv_buf, sems)) {
        sleep(c_sleep > 0 ? c_sleep : 500);
    }
    printf("pushed data: %ld, stack size: %d\n", recv_buf.size, p->pos);
    recv_buf_cleanup(&recv_buf);
    return 0;
}

int image_processor(RSTACK * p, int c_sleep, unsigned long * part_num, U8 ** img_cat, sem_t * sems) {
    RECV_BUF image;
    recv_buf_init(&image, BUF_INC);
    while(!pop(p, &image, sems)){
        sleep(c_sleep > 0 ? c_sleep: 0);
    }

    sem_wait(&sems[1]);
    if(!(*part_num & (1 << (image.seq)))){
        //img_cat[image.seq] = (U8 *) malloc(sizeof(image.size));
        memcpy(img_cat[image.seq], image.buf, image.size);
        *part_num |= 1 << (image.seq);
    }
    sem_post(&sems[1]);

    recv_buf_cleanup(&image);
    printf("processed data: %ld, stack size: %d, img_cat done: %d\n",image.size, p->pos, image.seq);
    return 0;
}

int main(int argc, char ** argv) {
    // printf("Starting the program\n");
    int buffer_size;
    int num_p;
    int num_c;
    int c_sleep;
    U8 ** img_cat;
    //U8 ** img_cat_cpy;
    double times[2];
    struct timeval tv;
    unsigned long part_num;
    pid_t pid;
    int pstate;

    CURL * curl_handle;

    if (argc < 6) {
        fprintf(stderr, "Not enough arguments to run paster2 command.\n");
        fprintf(stderr, "./paster2 <B> <P> <C> <X> <N>.\n");
        fprintf(stderr, "<B>: Buffer Size.\n<P>: Number of Producers.\n<C>: Number of Consumers.\n<X>: Number of milliseconds that a consumer sleeps.\n<N>: Image Number.\n");
        return -1;
    }

    if (gettimeofday(&tv, NULL) != 0) {
        perror("gettimeofday");
        abort();
    }
    times[0] = (tv.tv_sec) + tv.tv_usec/1000000.;

    buffer_size = atoi(argv[1]);
    num_p = atoi(argv[2]);
    num_c = atoi(argv[3]);
    c_sleep = atoi(argv[4]);

    if(num_p < 1 || num_c < 1){
        fprintf(stderr, "Producer and consumer values must be greater than 0");
        abort();
    }

    pid_t cpids[num_p + num_c];
    char url[255];
    strcpy(url, "http://ece252-1.uwaterloo.ca:2530/image?img=");
    strcat(url, argv[5]);
    strcat(url, "&part=");
    
    time_t t;
    srand((unsigned) time(&t));

    int shmsize = sizeof_shm_stack(buffer_size);
    int shmid = shmget(IPC_PRIVATE, shmsize, IPC_CREAT | IPC_EXCL | S_IRUSR | S_IWUSR);
    int shmid_sems = shmget(IPC_PRIVATE, sizeof(sem_t) * 2, IPC_CREAT | IPC_EXCL | S_IRUSR | S_IWUSR);
    int schmidt = shmget(IPC_PRIVATE, sizeof(unsigned long), IPC_CREAT | IPC_EXCL | S_IRUSR | S_IWUSR);
    int slhmantha = shmget(IPC_PRIVATE, shmsize, IPC_CREAT | IPC_EXCL | S_IRUSR | S_IWUSR);
    if (shmid == -1 || shmid_sems == -1 || schmidt == -1 || slhmantha == -1) {
        fprintf(stderr, "shmget error");
        abort();
    }

    curl_global_init(CURL_GLOBAL_DEFAULT);

    // forking producer processes
    for (int i = 0; i < num_p; i++) {
        pid = fork();

        if (pid == 0) { //child process (producer, use this to download png data)
            printf("creating a new producer process\n");
            RSTACK* image_buffer = shmat(shmid, NULL, 0);
            if ( image_buffer == (void *) -1 ) {
                fprintf(stderr, "shmat error");
                abort();
            }
            init_shm_stack(image_buffer, buffer_size);

            sem_t* sems = shmat(shmid_sems, NULL, 0);

            if ( sem_init(&sems[0], 1, 1) != 0 ) {
                perror("sem_init(sem[0])");
                abort();
            }
            if ( sem_init(&sems[1], 1, 1) != 0 ) {
                perror("sem_init(sem[1])");
                abort();
            }

            char stringz[20];
            char* url_part = (char* )malloc(256);
            //while(some condition related to getting all the image parts)
            if(num_p >= 50){
                sprintf(stringz,"%d",i % 50);
                strcpy(url_part, url);
                strcat(url_part, stringz);
                /* init a curl session */
                curl_handle = curl_easy_init();
                
                if (curl_handle == NULL) {
                    fprintf(stderr, "curl_easy_init: returned NULL\n");
                    return 1;
                }
                image_producer(url_part, curl_handle, image_buffer, c_sleep, sems);
            }
            else{
                for(int j = 0; j < 50/num_p + (50 % num_p != 0); ++j){
                    // printf("Downloading images\n");
                    sprintf(stringz, "%d", i * (50 / num_p + (50 % num_p != 0)) + j);
                    strcpy(url_part, url);
                    strcat(url_part, stringz);
                    /* init a curl session */
                    curl_handle = curl_easy_init();
                    
                    if (curl_handle == NULL) {
                        fprintf(stderr, "curl_easy_init: returned NULL\n");
                        return 1;
                    }
                    image_producer(url_part, curl_handle, image_buffer, c_sleep, sems);
                }
            }

            curl_easy_cleanup(curl_handle);

            shmdt(image_buffer);
            shmdt(sems);

            free(url_part);
            return 0;
            
        }
        else if (pid > 1) { //parent process (save all the child pids so we can wait for them)
            cpids[i] = pid;
        }
        else {
            fprintf(stderr, "fork error");
            abort();
        }
    }

    // forking consumer processes
    for (int i = num_p; i < num_p + num_c; ++i) {
        pid = fork();

        if (pid == 0) { //child process (consumer, use this to take the downloaded png data from our fixed buffer and put it into our global structure)
            printf("creating a new consumer process\n");
            RSTACK* image_buffer_c = (RSTACK *) shmat(shmid, NULL, 0);
            init_shm_stack(image_buffer_c, buffer_size);

            unsigned long part_num_c = (unsigned long) shmat(schmidt, NULL, 0);
            memset(&part_num_c, 0, sizeof(part_num_c));

            U8** img_cat_c = (U8 ** ) shmat(slhmantha, NULL, 0);

            sem_t* sems_c = (sem_t *) shmat(shmid_sems, NULL, 0);

            if ( sem_init(&sems_c[0], 1, 1) != 0 ) {
                perror("sem_init(sem[0])");
                abort();
            }
            if ( sem_init(&sems_c[1], 1, 1) != 0 ) {
                perror("sem_init(sem[1])");
                abort();
            }

            if ( image_buffer_c == (void *) -1) {
                fprintf(stderr, "shmat error");
                abort();
            }

            while(!is_empty(image_buffer_c) || part_num == 0) {
                // printf("processing images\n");
                image_processor(image_buffer_c, c_sleep, &part_num, img_cat_c, sems_c);
                num_iter++;
            }

            shmdt(image_buffer_c);
            shmdt(&part_num);
            shmdt(sems_c);

            return 0;

        }
        else if (pid > 1) { //parent process (save all the pids so we can wait)
            cpids[i] = pid;
        }
        else {
            fprintf(stderr, "fork error");
            abort();
        }
    }

    //After forking everything
    if (pid > 0) { //this is the parent process
        for (int i = 0; i < num_p + num_c; i++) {
            waitpid(cpids[i], &pstate, 0);
            if (WIFEXITED(pstate)) {
                printf("Child cpid[%d]=%d terminated with state: %d.\n", i, cpids[i], pstate);
            }
        }
        
        img_cat = (U8**) shmat(slhmantha, NULL, 0);

        concatenatePNG(img_cat, 50);
        shmdt(img_cat);

        shmctl( shmid, IPC_RMID, NULL );
        shmctl( shmid_sems, IPC_RMID, NULL );
        shmctl( schmidt, IPC_RMID, NULL );
        shmctl( slhmantha, IPC_RMID, NULL );

        curl_global_cleanup();

        if (gettimeofday(&tv, NULL) != 0) {
            perror("gettimeofday");
            abort();
        }
        times[1] = (tv.tv_sec) + tv.tv_usec/1000000.;
        printf("Parent pid = %d: total execution time is %.6lf seconds\n", getpid(),  times[1] - times[0]);
    }

    return 0;
}
