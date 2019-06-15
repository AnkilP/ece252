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
#include <errno.h>
#include <arpa/inet.h>
#include "../starter/png_util/zutil.h"
#include "../starter/png_util/crc.h"

/**
 *GLOBAL VARIABLES
 */

void concatenatePNG(U8* png) {
    unsigned int data_chunk_length = 0;
    int totalHeight = 0;
    U64 deflated_length = 0;
    U64 inflated_length = 0;
    int totaldeflatedsize = 0;

    U8* concatData = malloc(100000000);
    if (concatData == NULL) {
        printf("concatData malloc err");
    }

    U8* compressedConcatData = malloc(100000000);
    if (compressedConcatData == NULL) {
        printf("concatData malloc err");
    }

    U8* memBuff;

    U8 header[8];

    IHDR_p ihdr_p = NULL;
    U8 IHDR_length[4];
    U8 IHDR_type[4];
    U8* IHDR_crc_buff = NULL;
    U32 IHDR_crc;

    U8 IDAT_type[4];
    U8* IDAT_crc_buff = NULL;
    U32 IDAT_crc;

    U8 IEND[12];
    U8* IDAT = NULL;

    U8* buff = NULL;

    int q;
    for( q = 0; q < 50; q++) {
        if(f == NULL) {
            printf("NULL file pointer: %d\n", errno);
            return;
        }

        fread(&header, 1, 8, f);
        if(header[1] != 0x50 || header[2] != 0x4e || header[3] != 0x47) { //  check if it's a png
            printf("One of the files is not a png: %s, aborting command\n", paths[q]);
            return;
        }

        fread(&IHDR_length, 4, 1, f);
        fread(&IHDR_type, 4, 1, f);
        
        buff = malloc(13);
        fread(buff, 13, 1, f);
        ihdr_p = (IHDR_p) buff;
        ihdr_p->height = ntohl(ihdr_p->height);
        totalHeight += (ihdr_p->height);

        fseek(f, 33, SEEK_SET); //Skip the rest of data from IHDR to IDAT

        //Read IDAT Length
        fread(&data_chunk_length, 4, 1, f);
        data_chunk_length = ntohl(data_chunk_length);

        //Read IDAT Type
        fread(&IDAT_type, 4, 1, f);

        //Read in this png's IDAT
        IDAT = malloc(data_chunk_length);
        fread(IDAT, sizeof(char), data_chunk_length, f);
        memBuff = malloc(100000);
        if (memBuff == NULL) {
            printf("memBuff malloc failed");
        }
        int res = mem_inf(memBuff, &deflated_length, IDAT, data_chunk_length);
        if (res != 0) {
            printf("mem_inf: %d\n", res);
        }

        memcpy(concatData + totaldeflatedsize, memBuff, deflated_length);
        totaldeflatedsize += deflated_length;
        free(IDAT);
        free(memBuff);
        fseek(f, 4, SEEK_CUR); //skip idat crc
        fread(&IEND, 1, 12, f);
        fclose(f);
    }

    int resDef = mem_def(compressedConcatData, &inflated_length, concatData, totaldeflatedsize, Z_DEFAULT_COMPRESSION);
    if (resDef != 0) {
        printf("mem_def failed: %d\n", resDef);
    }
    FILE * fw = fopen("./output.png", "wb");

    fwrite(&header, 1, 8, fw);

    /*Writing IHDR**********************/
    fwrite(&IHDR_length, 4, 1, fw); //length
    
    fwrite(&IHDR_type, 4, 1, fw); //type

    ihdr_p->height = totalHeight;
    ihdr_p->height = htonl(ihdr_p->height);

    fwrite(ihdr_p, 13, 1, fw); //data

    IHDR_crc_buff = malloc(17);
    memcpy(IHDR_crc_buff, &IHDR_type, 4);
    memcpy(IHDR_crc_buff + 4, ihdr_p, 13);
    IHDR_crc = crc(IHDR_crc_buff, 17);
    IHDR_crc = htonl(IHDR_crc);
    fwrite(&IHDR_crc, 4, 1, fw); //crc
    free(IHDR_crc_buff);
    /***********************************/

    /*Writing IDAT**********************/
    inflated_length = htonl(inflated_length);
    fwrite(&inflated_length, 4, 1, fw); //length

    fwrite(&IDAT_type, 4, 1, fw); //type

    inflated_length = ntohl(inflated_length);
    fwrite(compressedConcatData, inflated_length, 1, fw); //data

    IDAT_crc_buff = malloc(inflated_length + 4);
    printf("%ld\n", inflated_length);
    memcpy(IDAT_crc_buff, &IDAT_type, 4);
    memcpy(IDAT_crc_buff + 4, compressedConcatData, inflated_length);
    IDAT_crc = crc(IDAT_crc_buff, inflated_length + 4);
    IDAT_crc = htonl(IDAT_crc);
    fwrite(&IDAT_crc, 4, 1, fw); //crc
    free(IDAT_crc_buff);
    /***********************************/

    /*Writing IEND**********************/
    fwrite(IEND, 1, 12, fw); //IEND
    /***********************************/

    fclose(fw);
    free(concatData);
    free(compressedConcatData);
}

int main(int argc, char ** argv) {
    if(argc > 1) {
        FILE *fp = fopen(argv[1], "rb");
        fclose(fp);
        concatenatePNG(argv, argc);
    }
//    free(paths);
} 
