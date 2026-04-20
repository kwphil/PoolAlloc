#ifndef GEAR_PRIV_H
#define GEAR_PRIV_H

/* Note: This SHOULD NOT BE USED PUBLICLY. Just use the hpp file. */

#define GEARALLOC_DEFAULT_PAGE 4096    /* Bytes */
#define GEARALLOC_TABLE_HEADER_SIZE 64 /* Count */

#include <stdlib.h>

#ifdef __cplusplus
#  define C_FUNC extern "C"
#else
#  define C_FUNC
#endif

typedef struct {
    void* ptr;
    size_t size;
} GearAlloc_FreeData;

typedef struct {
    GearAlloc_FreeData free_list[GEARALLOC_TABLE_HEADER_SIZE];
    void* page;
} GearAlloc_Table;

/*
 * @brief Allocator backend
 * @param alloc Size of the memory to be allocated
 * @param map_size Size of each map
 * @param map_list A pointer to the list of maps to grab
 * @param map_count The number of maps allocated.
 * @return A pointer of size `alloc`
*/
C_FUNC void* GearAlloc_pool_malloc(
    size_t              alloc, 
    size_t              map_size, 
    GearAlloc_Table***  map_list, 
    size_t*             map_count
);

/*
 * @brief Allocator backend
 * @param alloc Size of the memory to be allocated
 * @param map_size Size of each map
 * @param map_list A pointer to the list of maps to grab
 * @param map_count The number of maps allocated.
 * @param ptr The pointer to the parameter to be freed
 */
C_FUNC void GearAlloc_pool_free(
    size_t  alloc, 
    size_t  map_size, 
    GearAlloc_Table*** map_list, 
    size_t  map_count,
    void*   ptr
);

#endif /* GEAR_PRIV_H */