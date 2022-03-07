/*
 *                              ppmtrans
 *
 *   Purpose:
 *  
 *     A polymorphic program that can use either a uarray2 or
 *     uarray2b (blocked) to access mapping function that are used
 *     in a ppm image rotator.
 * 
 *   Authors: Henry Liu (hliu12) and Blake Watabe (bwatab01)
 *   Last Edited: 2-23-22
 *   
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "cputiming.h"

#include "assert.h"
#include "a2methods.h"
#include "a2plain.h"
#include "a2blocked.h"
#include "pnm.h"


typedef A2Methods_UArray2 A2;

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 
 *              Forward declaration of functions/
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

Pnm_ppm fileToPnm(char *fileName, A2Methods_T methods);
void transformImg(Pnm_ppm pixMap,
                int rotation,
                A2Methods_mapfun map,
                A2Methods_T methods,
                char *time_file_name);
A2 createResArr(Pnm_ppm pixMap,
                A2Methods_T methods,
                int rotation);
struct transformedArr *createClosure(A2Methods_T methods,
                                A2 finalArr,
                                int rotation);
void timeFileWrite(Pnm_ppm pixMap, A2Methods_T methods, A2Methods_mapfun map, 
                int rotation, float timeUsed, char *time_file_name);
void rotationApply(int col, int row, A2 array,
void *elem, void *cl);

/* Struct transformedArr                                
* Struct to hold an initially empty A2 array 
* that is used to fill pixel coordinates post rotation
*/
struct transformedArr{
    A2Methods_T methods; /* Methods of current 2D array representation */
    A2 resArr; /* Stores final rotated arr */
    int rotation; /* Rotation to be performed */
};

#define SET_METHODS(METHODS, MAP, WHAT) do {                    \
        methods = (METHODS);                                    \
        assert(methods != NULL);                                \
        map = methods->MAP;                                     \
        if (map == NULL) {                                      \
                fprintf(stderr, "%s does not support "          \
                                WHAT "mapping\n",               \
                                argv[0]);                       \
                exit(1);                                        \
        }                                                       \
} while (0)

static void
usage(const char *progname)
{
        fprintf(stderr, "Usage: %s [-rotate <angle>] "
                        "[-{row,col,block}-major] [filename]\n",
                        progname);
        exit(1);
}

int main(int argc, char *argv[]) 
{

    char *time_file_name = NULL;
    int   rotation       = 0;
    int   i;


    /* default to UArray2 methods */
    A2Methods_T methods = uarray2_methods_plain; 
    assert(methods);

    /* default to best map */
    A2Methods_mapfun *map = methods->map_default; 
    assert(map);
    char *fileName;
    for (i = 1; i < argc; i++) {

        if (strcmp(argv[i], "-row-major") == 0) {
                SET_METHODS(uarray2_methods_plain, map_row_major, 
                            "row-major");
        } else if (strcmp(argv[i], "-col-major") == 0) {
                SET_METHODS(uarray2_methods_plain, map_col_major, 
                            "column-major");
        } else if (strcmp(argv[i], "-block-major") == 0) {
                SET_METHODS(uarray2_methods_blocked,
                                map_block_major,
                                "block-major");
        } else if (strcmp(argv[i], "-rotate") == 0) {
                if (!(i + 1 < argc)) {      /* no rotate value */
                        usage(argv[0]);
                }
                char *endptr;
                rotation = strtol(argv[++i], &endptr, 10);
                if (!(rotation == 0 || rotation == 90 ||
                    rotation == 180 || rotation == 270)) {
                        fprintf(stderr, 
                                "Rotation must be 0, 90 180 or 270\n");
                        usage(argv[0]);
                }
                if (!(*endptr == '\0')) {    /* Not a number */
                        usage(argv[0]);
                }
        } else if (strcmp(argv[i], "-transpose") == 0) {
                fprintf(stderr, "Transpose functionality not implemented\n");
                usage(argv[0]);
        } else if (strcmp(argv[i], "-flip") == 0) {
                fprintf(stderr, "Transpose functionality not implemented\n");
                usage(argv[0]);

        } else if (strcmp(argv[i], "-time") == 0) {
                time_file_name = argv[++i];
        } else if (*argv[i] == '-') {
                fprintf(stderr, "%s: unknown option '%s'\n", argv[0],
                        argv[i]);
                usage(argv[0]);
        } else if (argc - i > 1) {
                fprintf(stderr, "Too many arguments\n");
                usage(argv[0]);
        } else {
                fileName = argv[i];
        }
    }

    Pnm_ppm pixMap = fileToPnm(fileName, methods);

    transformImg(pixMap, rotation, map, methods, time_file_name);

    exit(EXIT_SUCCESS);

}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 
 *            Functions implementing the ppmtrans program
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

/* Function: fileToPnm
 * Purpose: A function to open the specified file for reading and convert
 *           it into a Pnm_ppm instance to extract the info from the ppm file
 * Arguments: A char pointer to the name of the file, an A2 methods for
 *           access to the right functions
 * Returns: An instance of a Pnm_ppm
 */
Pnm_ppm fileToPnm(char *fileName, A2Methods_T methods)
{
    assert(methods != NULL);
    FILE *fp;
    Pnm_ppm pixMap;
    if (fileName == NULL) {
        pixMap = Pnm_ppmread(stdin, methods);;
    }
    else {
        fp = fopen(fileName, "r");
        if(fp == NULL) {
                fprintf(stderr, 
                        "%s: %s %s\n",
                        "Could not open file", fileName, "for reading");
                exit(EXIT_FAILURE);
                }
        assert(fp != NULL);
        pixMap = Pnm_ppmread(fp, methods);
        
    }
    assert(pixMap != NULL);
    return pixMap;
}


/* Function: transformImg
 * Purpose: The main function to execute the commands from user input.
            Implements the rotation mapping functions and the time
            function.
 * Arguments: A Pnm_ppm instance,
            the rotation amount,
            a A2Methods_mapfun instance,
            an A2 methods for access to the right functions,
            a char pointer to the name of the time file
 * Returns: none
 */
void transformImg(Pnm_ppm pixMap,
                int rotation,
                A2Methods_mapfun map,
                A2Methods_T methods,
                char *time_file_name)
{
    assert(pixMap != NULL);
    assert(map != NULL);
    assert(methods != NULL);
    A2 finalArr = createResArr(pixMap, methods, rotation);
    struct transformedArr *result = createClosure(methods,
                                                finalArr,
                                                rotation);
    assert(result);

    CPUTime_T timer = CPUTime_New();
    CPUTime_Start(timer);

    /* Pass new Pnm through map function as cl */
    map(pixMap->pixels, rotationApply, result);

    float timeUsed = CPUTime_Stop(timer);
    CPUTime_Free(&timer);

    methods->free(&pixMap->pixels);
    pixMap->pixels = result->resArr;
    Pnm_ppmwrite(stdout, pixMap);
    if (time_file_name != NULL) {
        timeFileWrite(pixMap, methods, map, rotation, timeUsed, time_file_name);
    }
    free(result);
    /* write the transfored image to output */
    Pnm_ppmfree(&pixMap);
        
}

/* Function: createResArr
 * Purpose: A helper function for image rotations. Creates
            an empty map that is populated with the new
            image post rotation.
 * Arguments: A Pnm_ppm instance,
            an A2 methods for access to the right functions,
            the rotation amount
 * Returns: An A2 object
*/

A2 createResArr(Pnm_ppm pixMap, A2Methods_T methods, int rotation)
{
    assert(pixMap != NULL);
    assert(methods != NULL);

    // Get width height from pixMap
    int width = pixMap->width;
    int height = pixMap->height;
    int size = sizeof(struct Pnm_rgb);

    // Create new empty A2 object
    A2 finalArr;
    if (rotation == 90) {
            finalArr = methods->new(height, width, size);
            pixMap->height = width;
            pixMap->width = height; 
    } else {
            finalArr = methods->new(width, height, size);
    }
    return finalArr;
}

/* Function: createClosure
 * Purpose: A helper function to create the struct that we send
            as a closure argument in the mapping function.
 * Arguments: The mapping functions,
                A2 object to store the final array,
                the correct mapping functions
 * Returns: a struct containing the arguments to be passed into the closure
 */
struct transformedArr *createClosure(A2Methods_T methods,
                                    A2 finalArr,
                                    int rotation)
{                                          
    assert(finalArr != NULL);
    assert(methods != NULL);
    struct transformedArr *result = malloc(sizeof(struct transformedArr));
    assert(result != NULL);
    result->methods = methods;
    result->resArr = finalArr; 
    result->rotation = rotation;
    return result;
}

/* Function: timeFileWrite
 * Purpose: A helper function to write the transformation time to a file
 * Arguments: A Pnm_ppm pixMap,
 *            an A2Methods_T object,
 *            the rotation,
 *            the time,
 *            the name of the time file
 * Returns: none
 */
void timeFileWrite(Pnm_ppm pixMap, A2Methods_T methods, A2Methods_mapfun map,
                int rotation, float timeUsed, char *time_file_name)
{
        assert(methods != NULL);
        assert(pixMap != NULL);

        FILE *timefile = fopen(time_file_name, "a");
        int totalPixels = pixMap->height * pixMap->width;
        fprintf(timefile,
                "Overall time: %fms\nTime per pixel: %fms\n",
                timeUsed,
                timeUsed / totalPixels);
        if (map == methods->map_block_major) {
                fprintf(timefile, "Method Used: Block Major\n");
        } else if (map == methods->map_row_major) {
                fprintf(timefile, "Method Used: Row Major\n");
        } else if (map == methods->map_col_major) {
                fprintf(timefile, "Method Used: Col Major\n");
        }
        fprintf(timefile, "Rotation: %d degrees\n", rotation);
        fprintf(timefile, "----------------------------------------\n");
        fclose(timefile);
}

/* Function: rotationApply
 * Purpose: An apply function that applies a specified rotation to 
 * the original image and saves the rotated pixel into the result
 * array
 * Arguments: The column,
 *            the row,
 *            A2 object,
 *            the element at (col,row)
 *            The closure, containing:
 *                The result arr to store rotated pixels                     
 *                The methods used for that array
 *                The rotation amount
 * Returns: none 
*/
void rotationApply(int col, int row, A2 array,
void *elem, void *cl)
{
    struct transformedArr *finalStruct = (struct transformedArr *) cl;
    A2Methods_T methods = finalStruct->methods;
    A2 resArr = finalStruct->resArr;
    int rotation = finalStruct->rotation;

    Pnm_rgb currPix = (Pnm_rgb) elem;
    Pnm_rgb newSpot;


    int height = methods->height(array);
    int width = methods->width(array);
    
    if (rotation == 0) {
        newSpot = methods->at(resArr, col, row);
            
    } else if (rotation == 90) {
        newSpot = methods->at(resArr, height - row - 1, col);
    
    } else if (rotation == 180) {
        newSpot = methods->at(resArr, width - col - 1, height - row - 1);
    }

    *newSpot = *currPix;

}

