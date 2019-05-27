/**
 * @file: catpng.c
 * @brief: iteratively concatenates png files pointed to by user
  */

#include <sys/types.h>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdio.h>   /* for printf().  man 3 printf */
#include <stdlib.h>  /* for exit().    man 3 exit   */
#include <string.h>  /* for strcat().  man strcat   */
#include "zutil.h"

/**
 *GLOBAL VARIABLES
 */
char * paths;
//char * paths[];
void lastchar_cut(char *str)
{
    if(str[strlen(str) - 1] == '/'){
        str[strlen(str)-1] = 0;
    }
}

/**
 * @brief: deflate in memory data from source to dest 
 * @param: dest U8* output buffer, caller supplies, should be big enough
 *         to hold the deflated data
 * @param: dest_len, U64* output parameter, length of deflated data
 * @param: source U8* source buffer, contains data to be deflated
 * @param: source_len U64 length of source data
 * @param: level int compression levels (https://www.zlib.net/manual.html)
 *         Z_NO_COMPRESSION, Z_BEST_SPEED, Z_BEST_COMPRESSION, Z_DEFAULT_COMPRESSION
 * @return =0  on success 
 *         <>0 on error
 * NOTE: 1. the compressed data length may be longer than the input data length,
 *       especially when the input data size is very small.
 */
int mem_def(U8 *dest, U64 *dest_len, U8 *source,  U64 source_len, int level)
{
    z_stream strm;    /* pass info. to and from zlib routines   */
    U8 out[CHUNK];    /* output buffer for deflate()            */
    int ret = 0;      /* zlib return code                       */
    int have = 0;     /* amount of data returned from deflate() */
    int def_len = 0;  /* accumulated deflated data length       */
    U8 *p_dest = dest;/* first empty slot in dest buffer        */

    
    strm.zalloc = Z_NULL;
    strm.zfree  = Z_NULL;
    strm.opaque = Z_NULL;

    ret = deflateInit(&strm, level);
    if (ret != Z_OK) {
        return ret;
    }

    /* set input data stream */
    strm.avail_in = source_len;
    strm.next_in = source;

    /* call deflate repetitively since the out buffer size is fixed
       and the deflated output data length is not known ahead of time */

    do {
        strm.avail_out = CHUNK;
        strm.next_out = out;
        ret = deflate(&strm, Z_FINISH); /* source contains the whole data */
        assert(ret != Z_STREAM_ERROR);
        have = CHUNK - strm.avail_out; 
        memcpy(p_dest, out, have);
        p_dest += have;  /* advance to the next free byte to write */
        def_len += have; /* increment deflated data length         */
    } while (strm.avail_out == 0);

    assert(strm.avail_in == 0);   /* all input will be used  */
    assert(ret == Z_STREAM_END);  /* stream will be complete */

    /* clean up and return */
    (void) deflateEnd(&strm);
    *dest_len = def_len;
    return Z_OK;
}

/**
 * @brief: inflate in memory data from source to dest 
 * @param: dest U8* output buffer, caller supplies, should be big enough
 *         to hold the deflated data
 * @param: dest_len, U64* output parameter, length of inflated data
 * @param: source U8* source buffer, contains zlib data to be inflated
 * @param: source_len U64 length of source data
 * 
 * @return =0  on success
 *         <>0 error
 */
int mem_inf(U8 *dest, U64 *dest_len, U8 *source,  U64 source_len)
{
    z_stream strm;    /* pass info. to and from zlib routines   */
    U8 out[CHUNK];    /* output buffer for inflate()            */
    int ret = 0;      /* zlib return code                       */
    int have = 0;     /* amount of data returned from inflate() */
    int inf_len = 0;  /* accumulated inflated data length       */
    U8 *p_dest = dest;/* first empty slot in dest buffer        */

    /* allocate inflate state 8 */
    strm.zalloc = Z_NULL;
    strm.zfree = Z_NULL;
    strm.opaque = Z_NULL;
    strm.avail_in = 0;        /* no input data being provided   */
    strm.next_in = Z_NULL;    /* no input data being provided   */
    ret = inflateInit(&strm);
    if (ret != Z_OK) {
        return ret;
    }

    /* set input data stream */
    strm.avail_in = source_len;
    strm.next_in = source;

    /* run inflate() on input until output buffer not full */
    do {
        strm.avail_out = CHUNK;
        strm.next_out = out;

        /* zlib format is self-terminating, no need to flush */
        ret = inflate(&strm, Z_NO_FLUSH);
        assert(ret != Z_STREAM_ERROR);    /* state no t clobbered */
        switch(ret) {
        case Z_NEED_DICT:
            ret = Z_DATA_ERROR;  /* and fall through */
        case Z_DATA_ERROR:
        case Z_MEM_ERROR:
            (void) inflateEnd(&strm);
			return ret;
        }
        have = CHUNK - strm.avail_out;
        memcpy(p_dest, out, have);
        p_dest += have;  /* advance to the next free byte to write */
        inf_len += have; /* increment inflated data length         */
    } while (strm.avail_out == 0 );

    /* clean up and return */
    (void) inflateEnd(&strm);
    *dest_len = inf_len;
    
    return (ret == Z_STREAM_END) ? Z_OK : Z_DATA_ERROR;
}

/* report a zlib or i/o error */
void zerr(int ret)
{
    fputs("zutil: ", stderr);
    switch (ret) {
    case Z_STREAM_ERROR:
        fputs("invalid compression level\n", stderr);
        break;
    case Z_DATA_ERROR:
        fputs("invalid or incomplete deflate data\n", stderr);
        break;
    case Z_MEM_ERROR:
        fputs("out of memory\n", stderr);
        break;
    case Z_VERSION_ERROR:
        fputs("zlib version mismatch!\n", stderr);
    default:
	fprintf(stderr, "zlib returns err %d!\n", ret);
    }
}

void concatenatePNG(char * paths[], int numpaths){
    int width;
    int height = 0;
    char header[8];
    char IDHR[13];
    char * IDATA;
    char IEND[12];
    int i = 0;
    for(int q = 0; q < numpaths; ++q){
        printf("%s", paths[0]); //debug
        fflush(stdout);
        FILE * f = fopen(paths[i++], "r");
        if(f == NULL){
            printf("Whats up");
            return;
        }
        fread(header, 1, sizeof(header), f);
        if(header[1] == 0x50 && header[2] == 0x4e && header[3] == 0x47){ //  check if it's a png
                printf("Or is this the problem");
        }
        fread(IDHR, 1, sizeof(IDHR), f);
        printf("%i%", IDHR[0]);
        memcpy(width, IDHR, 4*sizeof(*IDHR));
        width = ntohl(width);
        printf("%i", width);
        fclose(f);         
    }
}

int main(int argc, char * argv[]){
    if(argc == 1){
        return 0;
    }
    else{
        concatenatePNG(argv, argc - 1);
    }
    free(paths);
} 
