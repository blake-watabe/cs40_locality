/*
 *                              UArray2b
 *
 *   Purpose:
 *  
 *     Implementation for the UArray2, a 2-Dimensional array modification
 *     of Hanson's UArray interface
 * 
 *   Authors: Henry Liu (hliu12) and Blake Watabe (bwatab01)
 *   Last Edited: 2-23-22
 *   
*/

#include <uarray2.h>
#include <uarray.h>
#include <stdlib.h>
#include <stdio.h>
#include <mem.h>
#include <assert.h>
#include <math.h>
#include "uarray2b.h"

#define T UArray2b_T

const int MAX_BLOCKSIZE = 65536;

struct T {
    UArray2_T array;
    int blocksize;
    int width;
    int height;
};


/********************************************************************
 *               UArray2B Implementation Functions                  *
 ********************************************************************/

/* Function: UArray2b_new
 * Purpose: Creates a new instance of a blocked 2D array with a given
            block size
 * Representation: The blocked representation was executed using a
 *                 UArray2 as the 2D array. Each index of the Uarray2
 *                 stores a block. Each block was implemented using a
 *                 Uarray 1D array, so each index of the Uarray is
 *                 a cells of the block.
 * Arguments: The width, height, element size, and blocksize
 *
 * Returns: A new UArray2B
 */
extern T UArray2b_new (int width, int height, int size, int blocksize)
{
    assert(blocksize > 0 && size >0);
    assert(height > 0 && width > 0);

    T uarray2b;
    NEW(uarray2b);
    assert(uarray2b != NULL);

    uarray2b->width = width;
    uarray2b->height = height;
    uarray2b->blocksize = blocksize;
    
    int num_blocks_wide = (width / blocksize);
    num_blocks_wide += (num_blocks_wide > 0) ? 1 : 0;

    int num_blocks_high = (height / blocksize);
    num_blocks_high += (num_blocks_high > 0) ? 1 : 0;

    uarray2b->array = UArray2_new(num_blocks_wide,
                                    num_blocks_high,
                                    sizeof(UArray_T));

    UArray_T *blockArrSpot;
    for (int row = 0; row < num_blocks_high; row++) {
        for (int col = 0; col < num_blocks_wide; col++) {
            blockArrSpot = UArray2_at(uarray2b->array, col, row);
            *blockArrSpot = UArray_new(blocksize * blocksize, size);
        }
    }

    return uarray2b;
}

/* Function: UArray2b_new_64K_block
 * Purpose: Creates a new instance of a blocked 2D array, the block
            size allocated is the maximum based on element size and
            dimensions, with the max block size being 64KB so that
            the cache can store roughly 2 blocks.
 * Arguments: the width, height, and element size
 * Returns: A new UArray2B
 */
extern T UArray2b_new_64K_block(int width, int height, int size)
{
    assert(height > 0 && width > 0 && size > 0);
    int blocksize = sqrt(MAX_BLOCKSIZE / size);
    /* edge case: if size is greater than max block size */
    if (blocksize < 1) {
        blocksize = 1;
    } else if (blocksize > width || blocksize > height) {
        if (width > height) {
            blocksize = width;
        } else {
            blocksize = height;
        }
    }

    T array = UArray2b_new(width, height, size, blocksize);

    return array;
}

/* Function: UArray2b_free
 * Purpose: Frees memory allocated for the UArray2b, as well as at every
            block and at every cell inside of the block
 * Arguments: A pointer to the UArray2b to free
 * Returns: none
 */
extern void UArray2b_free (T *array2b)
{
    assert(*array2b != NULL && array2b != NULL);
    UArray2_T arr = (*array2b)->array;
    int width = UArray2_width(arr);
    int height = UArray2_height(arr);
    UArray_T *blockArrSpot;
    for (int row = 0; row < height; row++) {
        for (int col = 0; col < width; col++) {
            blockArrSpot = UArray2_at((*array2b)->array, col, row);
            UArray_free(blockArrSpot);
        }
    }
    UArray2_free(&(*array2b)->array);
    free(*array2b);
}

/* Function: UArray2b_width
 * Purpose: Gets the width of a 2b array
 * Arguments: The 2b array to get the width from
 * Returns: The int width of the array
*/
extern int UArray2b_width (T array2b)
{
    assert(array2b != NULL);
    return array2b->width;
}

/* Function: UArray2b_height
 * Purpose: Gets the height of a 2b array
 * Arguments: The 2b array to get the height from
 * Returns: The int height of the array
*/
extern int UArray2b_height (T array2b)
{
    assert(array2b != NULL);
    return array2b->height;
}

/* Function: UArray2b_size
 * Purpose: Gets the size of an elem in a 2b array
 * Arguments: The 2b array to get the size from
 * Returns: The int size of an elem
*/
extern int UArray2b_size (T array2b)
{
    assert(array2b != NULL);
    return UArray2_size(array2b->array);
}

/* Function: UArray2b_blocksize 
 * Purpose: Gets the blocksize of a 2b array
 * Arguments: The 2b array to get the blocksize from
 * Returns: The int blocksize of that array
*/
extern int UArray2b_blocksize(T array2b)
{
    assert(array2b != NULL);
    return array2b->blocksize;
}

/* Function: UArray2b_at
 * Purpose: Gets the element at a specified col and row
 * Arguments: The col, row, and 2b array to look in
 * Returns: A void pointer to the location of the elem in col, row
*/
extern void *UArray2b_at(T array2b, int col, int row)
{
    assert(array2b != NULL);
    assert(col < array2b->width && row < array2b->height);
    int blocksize = array2b->blocksize;
    UArray_T *currBlock = UArray2_at(array2b->array, col / blocksize,
                                          row / blocksize);
    return UArray_at(*currBlock, blocksize * (row % blocksize) + 
                                        (col % blocksize));
}

/* Function: UArray2b_map
 * Purpose: A mapping function for UArray2b that applies
 * its specified apply function block-major, traversing 
 * the entirety of one block before moving to the next
 * Arguments: The 2b array,
 *            The apply function to apply to each elem
 *            A closure
 * Returns: None
*/
extern void UArray2b_map(T array2b,
void apply(int col, int row, T array2b,
void *elem, void *cl),
void *cl)
{
    assert(array2b != NULL); 
    int blockWidth = UArray2_width(array2b->array);
    int blockHeight = UArray2_height(array2b->array);
    /* For every block high */
    for(int blockCol = 0; blockCol < blockWidth; blockCol++) {
        /* For every block wide */
        for (int blockRow = 0; blockRow < blockHeight; blockRow++) {
            UArray_T *currBlockArr = UArray2_at(array2b->array,
                                                blockCol,
                                                blockRow);
            /* for every element of the current block*/
            for(int blockIdx = 0;
                    blockIdx < UArray_length(*currBlockArr);
                    blockIdx++) {
                /* Check if out of bounds */
                int row = (blockRow * array2b->blocksize) +
                            (blockIdx / array2b->blocksize);
                int col = (blockCol * array2b->blocksize) +
                            (blockIdx % array2b->blocksize);
                
                if (row >= array2b->height ||
                    col >= array2b->width) {
                    continue;
                }

                apply(col, row, array2b, 
                    UArray_at(*currBlockArr, blockIdx), cl);

            }
        }
    }

}