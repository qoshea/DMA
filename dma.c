
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>


#include "mm.h"
#include "memlib.h"

#define ALIGNMENT 8
#define ALIGN(size) (((size) + (ALIGNMENT-1)) & ~0x7)  /* Ensures the heap is  double word aligned  */

/* Macros defining heap structure, all int are in bytes */
#define WORD_SIZE              4
#define DOUBLE_WORD_SIZE       8
#define CHUNKSIZE           1024
#define MIN_HEAP              24

/* Returns the max of x and y */
#define MAX(x, y) ((x) > (y) ? (x) : (y))

/* Combine a size and bit for the header and footer of a block */
#define PACK(size, alloc)  ((size) | (alloc))

/* Read and write a word at address p */
#define GET(p)       (*(int *)(p))
#define PUT(p, val)  (*(int *)(p) = (val))

/* Read the size and allocated fields from address p */
#define GET_SIZE(p)  (GET(p) & ~0x7)
#define GET_ALLOC(p) (GET(p) & 0x1)

/* Given block pointer bp, compute address of its header and footer */
#define HDRP(bp)       ((void *)(bp) - WORD_SIZE)
#define FTRP(bp)       ((void *)(bp) + GET_SIZE(HDRP(bp)) - DOUBLE_WORD_SIZE)

/* Given block pointer bp, compute address of next and previous blocks */
#define NEXT_BLKP(bp)  ((void *)(bp) + GET_SIZE(HDRP(bp)))
#define PREV_BLKP(bp)  ((void *)(bp) - GET_SIZE(HDRP(bp) - WORD_SIZE))

/* Given block pointer bp, which corresponds to the free list,
 compute address of the next and previous free blocks */
#define NEXT_FREEP(bp)(*(void **)(bp + DOUBLE_WORD_SIZE))
#define PREV_FREEP(bp)(*(void **)(bp))

/* Global variables */
static char *heap_listp = 0;  /* Pointer to first block */
static char *free_listp = 0;  /* Pointer to first free block */


/* Function prototypes for helper routines */
static void *extend_heap(size_t words);

/* Functions for checking the heap and block*/
//static void printblock(void *bp);
extern int mm_check_heap(void);
extern int checkblock(void *bp);
static void printblock(void *bp);

/*Functions for the doubly linked free block list*/
static void mm_insert(void *bp);
static void mm_remove(void *bp);
static void place(void *bp, size_t asize);
static void *first_fit(size_t asize);
static void *coalesce(void *bp);

/* Function prototypes for the main 4*/
int mm_init(void);
void *mm_malloc(size_t size);
void mm_free(void *bp);
void *mm_realloc(void *ptr, size_t size);


/*
 *                             _       _ _    ____
 *     _ __ ___  _ __ ___     (_)_ __ (_) |_ / /\ \
 *    | '_ ` _ \| '_ ` _ \    | | '_ \| | __| |  | |
 *    | | | | | | | | | | |   | | | | | | |_| |  | |
 *    |_| |_| |_|_| |_| |_|___|_|_| |_|_|\__| |  | |
 *                       |_____|             \_\/_/
 *
 * initializes the dynamic allocator with dummy header and footer along with padding and an epilogue "block"
 *
 * Input: None
 * Output: 0, if successful
 *         -1, if an error occurs
 */
int mm_init(void) {
    
    if ((heap_listp = mem_sbrk(2*MIN_HEAP)) == NULL){
        printf("Error: Initialization failed due to inability to extend heap.  mem_sbrk of %d returned null\n", 2*MIN_HEAP);
        return -1;
    }
    
    /*initialize new heap*/
    PUT(heap_listp, 0); // padding
        /* dummy block header */
    PUT(heap_listp + WORD_SIZE, PACK(MIN_HEAP, 1)); // padding
    PUT(heap_listp + DOUBLE_WORD_SIZE, 0); //prev
    PUT(heap_listp + DOUBLE_WORD_SIZE + WORD_SIZE, 0); // next
        /* dummy block footer */
    PUT(heap_listp + MIN_HEAP, PACK(MIN_HEAP, 1));
        /* dummy tail block */
    PUT(heap_listp+WORD_SIZE + MIN_HEAP, PACK(0, 1));
    
    /* the list pointers */
    free_listp = heap_listp + DOUBLE_WORD_SIZE;
    heap_listp += DOUBLE_WORD_SIZE;
    
    /* return -1 if failed extension */
    if (extend_heap(CHUNKSIZE/WORD_SIZE) == NULL)
        return -1;
    
    return 0;
}


/*
 *                                             _ _
 *     _ __ ___  _ __ ___      _ __ ___   __ _| | | ___   ___
 *    | '_ ` _ \| '_ ` _ \    | '_ ` _ \ / _` | | |/ _ \ / __|
 *    | | | | | | | | | | |   | | | | | | (_| | | | (_) | (__
 *    |_| |_| |_|_| |_| |_|___|_| |_| |_|\__,_|_|_|\___/ \___|
 *                       |_____|
 * allocates a block of memory and returns a pointer to that block
 *
 * Input: size: the desired block size
 * Output: a pointer to the newly-allocated block (whose size
 *          is a multiple of ALIGNMENT),
 *          or NULL if an error
 */
void *mm_malloc(size_t size) {
    size_t asize;
    size_t extendsize;
    char *bp;
    
    if (size <= 0)
        return NULL;
    
    /* Adjust block size and adjust for overhead and alignment reqs */
    asize = MAX(ALIGN(size) + DOUBLE_WORD_SIZE, MIN_HEAP);
    
    /* Go through free list */
    if ((bp = first_fit(asize))) {
        place(bp, asize);
        return bp;
    }
    
    /* If there was no fit found, get more mem and place */
    extendsize = MAX(asize, CHUNKSIZE);
    //return NULL if unable to get heap space
    if ((bp = extend_heap(extendsize/WORD_SIZE)) == NULL)
        return NULL;
    
    place(bp, asize);
    return bp;
}

/*
 *                              __
 *     _ __ ___  _ __ ___      / _|_ __ ___  ___
 *    | '_ ` _ \| '_ ` _ \    | |_| '__/ _ \/ _ \
 *    | | | | | | | | | | |   |  _| | |  __/  __/
 *    |_| |_| |_|_| |_| |_|___|_| |_|  \___|\___|
 *                       |_____|
 * frees a block of memory, enabling it to be reused later and adds to the free list
 *
 * Input: ptr to the allocated block to be freed
 * Output: nothing
 */

void mm_free(void *bp) {
    
    //if null
    if(!bp) return;
    
    //else
    size_t size = GET_SIZE(HDRP(bp));
    PUT(HDRP(bp), PACK(size, 0));
    PUT(FTRP(bp), PACK(size, 0));
    
    coalesce(bp);
}


/*
 *                     _
 *      ___ ___   __ _| | ___  ___  ___ ___
 *     / __/ _ \ / _` | |/ _ \/ __|/ __/ _ \
 *    | (_| (_) | (_| | |  __/\__ \ (_|  __/
 *     \___\___/ \__,_|_|\___||___/\___\___|
 * Join adjacent free blocks and update headers and stuff
 *
 * Input: pointer bp to a linked list
 * Output: pointer to newly coalesced block, or old ptr if no coalescing needed
 */
static void *coalesce(void *bp) {
    size_t prev_alloc = GET_ALLOC(FTRP(PREV_BLKP(bp))) || PREV_BLKP(bp) == bp;
    size_t next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(bp)));
    size_t size = GET_SIZE(HDRP(bp));
    
    /* Only next is free */
    if (prev_alloc && !next_alloc) {
        size += GET_SIZE(HDRP(NEXT_BLKP(bp)));
        mm_remove(NEXT_BLKP(bp));
        PUT(HDRP(bp), PACK(size, 0));
        PUT(FTRP(bp), PACK(size, 0));
    }
    
    /* Only previous is free */
    else if (!prev_alloc && next_alloc) {
        size += GET_SIZE(HDRP(PREV_BLKP(bp)));
        bp = PREV_BLKP(bp);
        mm_remove(bp);
        PUT(HDRP(bp), PACK(size, 0));
        PUT(FTRP(bp), PACK(size, 0));
    }
    
    /* Surrounded by free blocks */
    else if (!prev_alloc && !next_alloc) {
        size += GET_SIZE(HDRP(PREV_BLKP(bp))) +
        GET_SIZE(HDRP(NEXT_BLKP(bp)));
        
        mm_remove(PREV_BLKP(bp));
        mm_remove(NEXT_BLKP(bp));
        bp = PREV_BLKP(bp);
        PUT(HDRP(bp), PACK(size, 0));
        PUT(FTRP(bp), PACK(size, 0));
    }
    
    mm_insert(bp);
    return bp;
}


/*
 *                             _                     _
 *     _ __ ___  _ __ ___     (_)_ __  ___  ___ _ __| |_
 *    | '_ ` _ \| '_ ` _ \    | | '_ \/ __|/ _ \ '__| __|
 *    | | | | | | | | | | |   | | | | \__ \  __/ |  | |_
 *    |_| |_| |_|_| |_| |_|___|_|_| |_|___/\___|_|   \__|
 *                       |_____|
 * Inserts a block at the front of the free list
 *
 * Input: pointer to list, bp, that is the freelist
 * Output: none
 */
static void mm_insert(void *bp){
    
    //Sets next to (start of) free list
    NEXT_FREEP(bp) = free_listp;
    //Sets current prev to new
    PREV_FREEP(free_listp) = bp;
    // Set prev  to NULL
    PREV_FREEP(bp) = NULL;
    // Sets head of free list to new block
    free_listp = bp;
}

/*
 *
 *     _ __ ___  _ __ ___      _ __ ___ _ __ ___   _____   _____
 *    | '_ ` _ \| '_ ` _ \    | '__/ _ \ '_ ` _ \ / _ \ \ / / _ \
 *    | | | | | | | | | | |   | | |  __/ | | | | | (_) \ V /  __/
 *    |_| |_| |_|_| |_| |_|___|_|  \___|_| |_| |_|\___/ \_/ \___|
 *                       |_____|
 * Removes a block from the free list
 *
 * Input: pointer to a block, bp, that is the block to be removed
 * Output: none
 */
static void mm_remove(void *bp) {
    if (PREV_FREEP(bp))
        NEXT_FREEP(PREV_FREEP(bp)) = NEXT_FREEP(bp);
    else
        free_listp = NEXT_FREEP(bp);
    
    //no matter what, headers surrounding bp now skip bp
    PREV_FREEP(NEXT_FREEP(bp)) = PREV_FREEP(bp);
}

/*
 *      __ _          _        __ _ _
 *     / _(_)_ __ ___| |_     / _(_) |_
 *    | |_| | '__/ __| __|   | |_| | __|
 *    |  _| | |  \__ \ |_    |  _| | |_
 *    |_| |_|_|  |___/\__|___|_| |_|\__|
 *                      |_____|
 * Find a fit for a block with asize bytes, first fit algorithm implemented
 *
 * Input : size_t asize indicating min size of free block needed
 * Output : pointer to the first free block
 */
static void *first_fit(size_t asize) {
    
    void *bp;
    
    for (bp = free_listp; GET_ALLOC(HDRP(bp)) == 0; bp = NEXT_FREEP(bp)) {
        if (asize <= (size_t)GET_SIZE(HDRP(bp)))
            return bp;
                //^^ the first fit in the LIFO free list
    }
    // no fit
    return NULL;
}

/*
 *               _                 _     _
 *      _____  _| |_ ___ _ __   __| |   | |__   ___  __ _ _ __
 *     / _ \ \/ / __/ _ \ '_ \ / _` |   | '_ \ / _ \/ _` | '_ \
 *    |  __/>  <| ||  __/ | | | (_| |   | | | |  __/ (_| | |_) |
 *     \___/_/\_\\__\___|_| |_|\__,_|___|_| |_|\___|\__,_| .__/
 *                                 |_____|               |_|
* Extend heap with free block and return its block pointer
 *
 * Input: size_t words that indicates how many words for which we have to make room
 * Output:none
 */
static void *extend_heap(size_t words) {
    char *bp;
    size_t size;
    
    /* Allocate an even number of words to maintain alignment */
    //      if an odd number, (words_++)ws    else w*ws == bytes needed
    size = (words % 2) ? (words+1) * WORD_SIZE : words * WORD_SIZE;
    // must be the min size
    if (size < MIN_HEAP)
        size = MIN_HEAP;
    // mem_sbrk failed.  Did not print error for I wasn't sure if mem_sbrk writes errors to errno
    if ((long)(bp = mem_sbrk(size)) == -1){
        printf("Error: could not extend heap. Call to mem_sbrk failed with size %d\n", size);
        return NULL;
    }
    
    /* Initialize free block header/footer and the epilogue header */
    PUT(HDRP(bp), PACK(size, 0));
    PUT(FTRP(bp), PACK(size, 0));
    PUT(HDRP(NEXT_BLKP(bp)), PACK(0, 1));
    
    return coalesce(bp);
}

/*
 *           _
 *     _ __ | | __ _  ___ ___
 *    | '_ \| |/ _` |/ __/ _ \
 *    | |_) | | (_| | (_|  __/
 *    | .__/|_|\__,_|\___\___|
 *    |_|
 * Place block of asize bytes at head of heap list and split if remainder would be at least minimum block size
 *
 * Input: pointer to heap, bp, and size_t, size that indicates amount of bytes
 * Output: none
 */
static void place(void *bp, size_t asize) {
    /* size of free block */
    size_t completeSize = GET_SIZE(HDRP(bp));
    
    //room for another block
    if ((completeSize - asize) >= MIN_HEAP) {
        PUT(HDRP(bp), PACK(asize, 1));
        PUT(FTRP(bp), PACK(asize, 1));
        mm_remove(bp);
        bp = NEXT_BLKP(bp);
        PUT(HDRP(bp), PACK(completeSize-asize, 0));
        PUT(FTRP(bp), PACK(completeSize-asize, 0));
        coalesce(bp);
        // no room, just put at front and the new size
    } else {
        PUT(HDRP(bp), PACK(completeSize, 1));
        PUT(FTRP(bp), PACK(completeSize, 1));
        mm_remove(bp);
    }
}

/*
 *                                               _ _
 *     _ __ ___  _ __ ___      _ __ ___     __ _| | | ___   ___
 *    | '_ ` _ \| '_ ` _ \    | '__/ _ \   / _` | | |/ _ \ / __|
 *    | | | | | | | | | | |   | | |  __/  | (_| | | | (_) | (__
 *    |_| |_| |_|_| |_| |_|___|_|  \___|___\__,_|_|_|\___/ \___|
 *                       |_____|      |_____|
 *      __       _                             _                                _           __
 *     / /__ _  | | ___  ___ ___   _ __   __ _(_)_   _____  __   _____ _ __ ___(_) ___  _ __\ \
 *    | |/ _` | | |/ _ \/ __/ __| | '_ \ / _` | \ \ / / _ \ \ \ / / _ \ '__/ __| |/ _ \| '_ \| |
 *    | | (_| | | |  __/\__ \__ \ | | | | (_| | |\ V /  __/  \ V /  __/ |  \__ \ | (_) | | | | |
 *    | |\__,_| |_|\___||___/___/ |_| |_|\__,_|_| \_/ \___|   \_/ \___|_|  |___/_|\___/|_| |_| |
 *     \_\                                                                                  /_/
 *
 * changes the size of a block according to the size paramter passed in
 *
 * Input: a block pointer, ptr, to the block that much become
 * parameter size_t size byes in size
 * Ouput: block pointer to the newly realloc-ed block
 */
void *mm_realloc(void *ptr, size_t size) {
    size_t oldsize;
    void *newptr;
    size_t asize = MAX(ALIGN(size) + DOUBLE_WORD_SIZE, MIN_HEAP);
    
    if(size <= 0){
        mm_free(ptr);
        return 0;
    }
    
    /* If oldptr is NULL, then this is just malloc. */
    if(ptr == NULL)
        return mm_malloc(size);
    
    /* Get the size of the original block */
    oldsize = GET_SIZE(HDRP(ptr));
    
    /* If the size doesn't need to be changed, return orig pointer */
    if (asize == oldsize)
        return ptr;
    
    /* If the size needs to be decreased, shrink the block and return same pointer */
    if(asize <= oldsize){
        size = asize;
        
        /* If remaining space is too small for a new block, return pointer */
        if(oldsize - size <= MIN_HEAP)
            return ptr;
        
        //otherwise split and free remainder
        PUT(HDRP(ptr), PACK(size, 1));
        PUT(FTRP(ptr), PACK(size, 1));
        PUT(HDRP(NEXT_BLKP(ptr)), PACK(oldsize-size, 1));
        mm_free(NEXT_BLKP(ptr));
        return ptr;
    }
    
    // if the very next block is free and the last in the heap
    void *potentialSol = NEXT_BLKP(ptr);
    if (!GET_ALLOC(HDRP(potentialSol))){
        // add the size of that block to your current block
        size_t withFREE = GET_SIZE(HDRP(potentialSol)) + oldsize;
        // merge the two blocks if you can just fit everything between the 2
        if (withFREE >= asize) {
            mm_remove(potentialSol);
            PUT(HDRP(ptr), PACK(withFREE, 1));
            PUT(FTRP(ptr), PACK(withFREE, 1));
            return ptr;
        }
    }
    
    // inevitable call to malloc due to not enough space in heap
    newptr = mm_malloc(size);
    
    /* If mm_malloc() fails the original block is left untouched  */
    if(!newptr)
        return 0;
    
    /* Copy old payload & free old block */
    if(size < oldsize)
        oldsize = size;
    // Copy data over
    memcpy(newptr, ptr, oldsize);
    mm_free(ptr);
    return newptr;
}

/*
 *          _               _    _                __                  _   _
 *      ___| |__   ___  ___| | _(_)_ __   __ _   / _|_   _ _ __   ___| |_(_) ___  _ __  ___
 *     / __| '_ \ / _ \/ __| |/ / | '_ \ / _` | | |_| | | | '_ \ / __| __| |/ _ \| '_ \/ __|
 *    | (__| | | |  __/ (__|   <| | | | | (_| | |  _| |_| | | | | (__| |_| | (_) | | | \__ \
 *     \___|_| |_|\___|\___|_|\_\_|_| |_|\__, | |_|  \__,_|_| |_|\___|\__|_|\___/|_| |_|___/
 *                                       |___/
 *
 * These are currently not being called anywhere in the program */

/*Function printblock
 * Prints out the information of a block
 *
 * Input: a block pointer
 * Output: none
 */
static void printblock(void *bp) {
    if (GET_SIZE(HDRP(bp)) == 0) {
        printf("%p: End of list\n", bp);
        return;
    }
    printf("%s block at %p, size %u", (GET_ALLOC(HDRP(bp)) ? "allocated" : "free"), bp, GET_SIZE(HDRP(bp)));
    //if allocated
    if (GET_ALLOC(HDRP(bp)))
        printf("\n");
    //if unallocted
    else
        printf(", next: %p\n", NEXT_FREEP(bp));
    
    return;
}


/*Function checkblock
 * Checks validity of a block
 *
 * Input: a block pointer
 * Output: int indicating an error in the block
 */
int checkblock(void *bp) {
    int toReturn;
    printblock(bp);
    toReturn = 1;
    
    // if not aligned
    if ((size_t)bp % 8){
        printf("Error: %p is not doubleword aligned\n", bp);
        toReturn = 0;
    }
    
    //if mismatching headers and footers
    if (GET(HDRP(bp)) != GET(FTRP(bp))) {
        printf("Error: header does not match footer\n");
        toReturn = 0;
    }
    
    // if sizes in header and footer are off
    if (GET_SIZE(HDRP(bp)) != GET_SIZE(FTRP(bp))) {
        printf("Error: The sizes are off!\n");
        toReturn = 0;
    }
    
    // if out of bounds
    if (bp < mem_heap_lo() || bp > mem_heap_hi()){
        printf("Error: block is out of heap bounds!\n");
        toReturn= 0;
    }
    
    // if next free or prev free are out of bounds
    if (NEXT_FREEP(bp) != NULL && PREV_FREEP(bp) != NULL ){
        // got mem_heap_lo and mem_heap_hi from the same library of mem_sbrk. Hope it's okay I used these
        if (NEXT_FREEP(bp)< mem_heap_lo() || NEXT_FREEP(bp) > mem_heap_hi()){
            printf("Error: next pointer %p is out of heap bounds \n", NEXT_FREEP(bp));
            toReturn = 0;
        }
        
        if (PREV_FREEP(bp)< mem_heap_lo() || PREV_FREEP(bp) > mem_heap_hi()){
            printf("Error: prev pointer %p is out heap bounds \n", PREV_FREEP(bp));
            toReturn = 0;
        }
    }
    // 0 if error, 1 o/w
    return toReturn;
}

/*
 * Function mm_check_heap
 * check of the heap for consistency
 *
 * Input: none
 * Output: 0 if there was an error within the heap
 *         1 if the heap was fine
 */
extern int mm_check_heap(void) {
    void *bp = heap_listp;
    int toReturn = 1;
    
    /* check prologue header */
    if ( GET_SIZE(HDRP(bp)) != MIN_HEAP || !GET_ALLOC(HDRP(bp))){
        printf("Error: Prologue Header equals %d instead of %d\n", GET_SIZE(HDRP(bp)), MIN_HEAP);
        toReturn = 0;
    }
    
    /* check prologue footer */
    if (GET_SIZE(FTRP(bp)) != MIN_HEAP || !GET_ALLOC(FTRP(heap_listp))){
        printf("Error: Prologue Footer equals %d instead of %d \n", GET_SIZE(FTRP(bp)), MIN_HEAP);
        toReturn = 0;
    }
    
    //check the rest of the heap
    for (bp = heap_listp; GET_SIZE(HDRP(bp)) > 0; bp = NEXT_BLKP(bp)) {
        if (checkblock(bp) == 0 )
            toReturn = 0;
    }
    
    //Check free list
    int i = 1;
    for(bp = free_listp; GET_ALLOC(HDRP(bp)) == 0; bp = NEXT_FREEP(bp)) {
        if(checkblock(bp) == 0)
            toReturn = 0;
        /* Check every free block in the free list */
        if (GET_ALLOC(FTRP(bp))){
            if (i == 1)
                printf("Error: The first block in the free list was not free \n");
            else if (i ==2)
                printf("Error: The second block in the free list was not free \n");
            else if (i ==3)
                printf("Error: The third block in the free list was not free \n");
            else
                printf(" The %dth block in the free list is not free\n", i);
            
            toReturn = 0;
        }
        i++;
    }
    return toReturn;
}
