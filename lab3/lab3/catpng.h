/**
 * @file: catpng.c
 * @brief: iteratively concatenates png files pointed to by user
  */
#pragma once

/**
 *GLOBAL VARIABLES
 */

/**
 * Type Definitions 
 */

typedef unsigned char U8;
typedef unsigned int  U32;
typedef unsigned long int U64;

void concatenatePNG(U8 ** paths, int numpaths);

