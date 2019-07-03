/**
 * @file: catpng.c
 * @brief: iteratively concatenates png files pointed to by user
  */

#include "catpng.h"

/**
 *GLOBAL VARIABLES
 */

void concatenatePNG(U8 ** paths, int num_of_splits) {
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
    IHDR_p ihdr_copy = NULL;
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
    
    for(int q = 0; q < num_of_splits; q++) {
        U8 * png = paths[q];
        memcpy(&header, png, 8);
        png += 8 * sizeof(*png);
        if(header[1] != 0x50 || header[2] != 0x4e || header[3] != 0x47) { //  check if it's a png
            printf("One of the files is not a png: %s, aborting command\n", png);
            continue;
        }

        memcpy(&IHDR_length, png, 4);
        png += 4 * sizeof(*(png));
        memcpy(&IHDR_type, png, 4);
        png += 4 * sizeof(*(png));

        buff = malloc(13);
        memcpy(buff, png, 13);
        png += 13 * sizeof(*(png));

        ihdr_p = (IHDR_p) buff;
        ihdr_p->height = ntohl(ihdr_p->height);
        totalHeight += (ihdr_p->height);

        png += 4 * sizeof(*(png));

        //Read IDAT Length
        data_chunk_length = 0;
        memcpy(&data_chunk_length, png, 4);
        png += 4 * sizeof(*(png));
        data_chunk_length = ntohl(data_chunk_length);

        //Read IDAT Type
        memcpy(&IDAT_type, png, 4);
        png += 4 * sizeof(*(png));

        //Read in this png's IDAT
        printf("q: %d, length: %d\n", q, data_chunk_length);
        IDAT = malloc(data_chunk_length * sizeof(*(png)));
        if (IDAT == NULL) {
            printf("IDAT malloc failed");
        }
        memcpy(IDAT, png, data_chunk_length * sizeof(*(png)));
        png += data_chunk_length * sizeof(*(png));

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
        png += 4 * sizeof(*(png));
        memcpy(&IEND, png, 12);
        png += 12 * sizeof(*(png));
        free(buff);
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

    //Grab the IHDR Data chunk from first image
    U8* png_temp = paths[0];
    buff = malloc(13);
    memcpy(buff, png_temp+16, 13);
    ihdr_copy = (IHDR_p) buff;
    ihdr_copy->height = totalHeight;
    ihdr_copy->height = htonl(ihdr_copy->height);

    fwrite(ihdr_copy, 13, 1, fw); //data
    
    IHDR_crc_buff = malloc(17);
    memcpy(IHDR_crc_buff, &IHDR_type, 4);
    memcpy(IHDR_crc_buff + 4, ihdr_copy, 13);
    IHDR_crc = crc(IHDR_crc_buff, 17);
    IHDR_crc = htonl(IHDR_crc);
    fwrite(&IHDR_crc, 4, 1, fw); //crc
    free(IHDR_crc_buff);
    free(buff);
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

// int main(int argc, char ** argv) {
//     if(argc > 1) {
//         FILE *fp = fopen(argv[1], "rb");
//         fclose(fp);
//         concatenatePNG(argv, argc);
//     }
// //    free(paths);
// } 
