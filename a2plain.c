/*
 *                              A2plain
 *
 *   Purpose:
 *  
 *     Defines a struct of function pointers for a UArray2 to be
 *     used by an A2Methods_T object
 * 
 *   Authors: Henry Liu (hliu12) and Blake Watabe (bwatab01)
 *   Last Edited: 2-23-22
 *   
 */

#include <string.h>
#include <a2plain.h>
#include "uarray2.h"


/* Function: A2Methods_UArray2
 * Purpose: A function pointer to allow A2methods to call the
            new function for UArray2
 * Arguments: The width, height, element size, and blocksize
 * Returns: A2methods_UArray2 object
 */
static A2Methods_UArray2 new(int width, int height, int size)
{
  return UArray2_new(width, height, size);
}

/* Function: new_with_blocksize
 * Purpose: A function pointer to allow A2methods to call the
            new function for UArray2
 * Arguments: the width, height, element size, and the blocksize
 * Returns: A new UArray2B
 */
static A2Methods_UArray2 new_with_blocksize(int width,
                                            int height,
                                            int size,
                                            int blocksize)
{
  (void) blocksize;
  return UArray2_new(width, height, size);
}

/* Function: a2free
 * Purpose: A function pointer to allow A2methods to call the
            free function for UArray2
 * Arguments: A2methods_UArray2 object
 * Returns: none
 */
static void a2free(A2Methods_UArray2 *uarray2) {
  UArray2_free((UArray2_T *)uarray2);
}

/* Function: width
 * Purpose: A function pointer to allow A2methods to call the
            width function for UArray2
 * Arguments: A2methods_UArray2 object
 * Returns: the width
 */
static int width(A2Methods_UArray2 uarray2) {
  return UArray2_width(uarray2);
}

/* Function: height
 * Purpose: A function pointer to allow A2methods to call the
            height function for UArray2
 * Arguments: A2methods_UArray2 object
 * Returns: the height
 */
static int height(A2Methods_UArray2 uarray2) {
  return UArray2_height(uarray2);
}

/* Function: size
 * Purpose: A function pointer to allow A2methods to call the
            size function for UArray2
 * Arguments: A2methods_UArray2 object
 * Returns: the element size
 */
static int size(A2Methods_UArray2 uarray2) {
  return UArray2_size(uarray2);
}



static int blocksize(A2Methods_UArray2 uarray2) {
  (void)uarray2;
  return 1;
}

/* Function: at
 * Purpose: Returns the pointer of the element at the specified row 
 *          and column index in UArray2
 * Arguments: A non-null UArray2 object pointer, row and col indices 
 *         that are within the UArray2 height and width respectively
 * Returns: A non-null pointer to the element at row,col
 */
static A2Methods_Object *at(A2Methods_UArray2 uarray2, int i, int j) {
  return UArray2_at(uarray2, i, j);
}


/* Function: map_row_major
 * Purpose: Iterates through the a Uarray2 array and applies a function to 
 *          it by column: row indices vary more rapidly than columns
 * Arguments: A non-null UArray2 object pointer, an existing applied 
 *         function which has an integer representing the column and 
 *         row, the non-null UArray2 object pointer, a void pointer 
 *         that holds an entry for the 2D array, and a non-null void  
 *         pointer that is a closure argument.
 * Returns: None
 */
static void map_row_major(A2Methods_UArray2 uarray2,
                          A2Methods_applyfun apply,
                          void *cl)
{
  UArray2_map_row_major(uarray2, (UArray2_applyfun*)apply, cl);
}

/* Function: map_col_major
 * Purpose: Iterates through the 2D array and applies a function to 
 *          it by row: column indices vary more rapidly than rows
 * Arguments: A non-null UArray2 object pointer, an existing applied 
 *         function which has an integer representing the column and 
 *         row, the non-null UArray2 object pointer, a void pointer 
 *         that holds an entry for the 2D array, and a non-null void  
 *         pointer that is a closure argument.
 * Returns: None
 */
static void map_col_major(A2Methods_UArray2 uarray2,
                          A2Methods_applyfun apply,
                          void *cl)
{
  UArray2_map_col_major(uarray2, (UArray2_applyfun*)apply, cl);
}

/* struct small_closure
* A struct to hold an additional apply 
* function along with a closure
*/
struct small_closure {
  A2Methods_smallapplyfun *apply; 
  void                    *cl;
};

static void apply_small(int i, int j, UArray2_T uarray2,
                        void *elem, void *vcl)
{
  struct small_closure *cl = vcl;
  (void)i;
  (void)j;
  (void)uarray2;
  cl->apply(elem, cl->cl);
}

/* Function: small_map_row_major
 * Purpose: Iterates through the 2D array and applies a function to 
 *          it by column: row indices vary more rapidly than columns
 * Arguments: A non-null UArray2 object pointer, the non-null UArray2 object 
 * pointer, a void pointer that holds an entry for the 2D array, and a 
 * non-null void pointer that is a closure argument.
 * Returns: None
 */
static void small_map_row_major(A2Methods_UArray2        a2,
                                A2Methods_smallapplyfun  apply,
                                void *cl)
{
  struct small_closure mycl = { apply, cl };
  UArray2_map_row_major(a2, apply_small, &mycl);
}

/* Function: small_map_col_major
 * Purpose: Iterates through the 2D array and applies a function to 
 *          it by row: column indices vary more rapidly than rows
 * Arguments: A non-null UArray2 object pointer, the non-null UArray2 
 * object pointer, a void pointer that holds an entry for the 2D array, 
 * and a non-null void pointer that is a closure argument.
 * Returns: None
 */
static void small_map_col_major(A2Methods_UArray2        a2,
                                A2Methods_smallapplyfun  apply,
                                void *cl)
{
  struct small_closure mycl = { apply, cl };
  UArray2_map_col_major(a2, apply_small, &mycl);
}

/* struct A2Methods_T uarray2_methods_plain_struct
*  Purpose: Holds the function pointers associated with a
*  UArray2 A2 Object
*/
static struct A2Methods_T uarray2_methods_plain_struct = {
    new,
    new_with_blocksize,
    a2free,
    width,
    height,
    size,
    blocksize,
    at,
    map_row_major,
    map_col_major,
    NULL,
    map_row_major,      /* map_default */
    small_map_row_major,
    small_map_col_major,
    NULL,
    small_map_row_major,        /* small_map_default */
};

/* the exported pointer to the struct */

A2Methods_T uarray2_methods_plain = &uarray2_methods_plain_struct;
