/**
 * @file: catpng.c
 * @brief: iteratively concatenates png files pointed to by user
  */
#pragma once

#include <sys/types.h>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdio.h>   /* for printf().  man 3 printf */
#include <stdlib.h>  /* for exit().    man 3 exit   */
#include <string.h>  /* for strcat().  man strcat   */
#include "../starter/png_util/zutil.h"
#include "../starter/png_util/crc.h"
#include <errno.h>
#include <arpa/inet.h>

/**
 *GLOBAL VARIABLES
 */

/**
 * Type Definitions 
 */
typedef unsigned char U8;
typedef unsigned int  U32;
typedef unsigned long int U64;

typedef struct IHDR {// IHDR chunk data 
    U32 width;        /* width in pixels, big endian   */
    U32 height;       /* height in pixels, big endian  */
    U8  bit_depth;    /* num of bits per sample or per palette index.
			 valid values are: 1, 2, 4, 8, 16 */
    U8  color_type;   /* =0: Grayscale; =2: Truecolor; =3 Indexed-color
                         =4: Greyscale with alpha; =6: Truecolor with alpha */
    U8  compression;  /* only method 0 is defined for now */
    U8  filter;       /* only method 0 is defined for now */
    U8  interlace;    /* =0: no interlace; =1: Adam7 interlace */
}* IHDR_p;

void concatenatePNG(char ** paths, int numpaths);
} 
