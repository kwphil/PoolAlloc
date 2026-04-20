#include <GearAlloc/priv.h>

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>

/* Catch to stop infinite recursing */
bool GearAlloc_free_ptr_fail_defrag = false;

void GearAlloc_free_ptr(
    GearAlloc_Table* table,
    size_t size,
    void* ptr
) {
    for(size_t i = 0; i < GEARALLOC_TABLE_HEADER_SIZE; ++i) {
        GearAlloc_FreeData* data = &table->free_list[i];

        /*
         * +-----------------+------------+
         * |     Freed       |  To free   |
         * +-----------------+------------+
         *
         * Merges into
         *
         * +------------------------------+
         * |             Freed            |
         * +------------------------------+
         * 
         * Calculates the last index of freed memory
         * If the pointer is the NEXT index, then merge them into one
         */
        if(data->ptr+data->size == ptr-size) {
            data->size += size;
            return;
        }

        /* Do the same thing from the other side */
        if(ptr+size == data->ptr) {
            data->ptr -= size;
            data->size += size;
            return;
        }

        /* If there's a zero'd out index, replace it */
        if(data->size == 0) {
            data->ptr = ptr;
            data->size = size;
            return;
        }
    }

    if(GearAlloc_free_ptr_fail_defrag) {
        fprintf(stderr,
            "Failed to squash page entries\n"
            "Dumping page entries of failed page"
        );
        
        fflush(stderr);
    
        printf("[\n");
        for(size_t i = 0; i < GEARALLOC_TABLE_HEADER_SIZE; ++i) {
            printf("\t[ %p, %ld ],\n", table->free_list[i].ptr, table->free_list[i].size);
        }
        printf("]\n");

        exit(-1);
    }

    /* Page became too fragmented, attempt to merge */
    /* Start by sorting entries by size */
    for (size_t i = 0; i < GEARALLOC_TABLE_HEADER_SIZE; ++i) {
        size_t min = (size_t)-1;
        size_t min_idx = i;

        for (size_t j = i; j < GEARALLOC_TABLE_HEADER_SIZE; ++j) {
            size_t curr = (size_t)table->free_list[j].ptr;

            if (curr < min) {
                min = curr;
                min_idx = j;
            }
        }

        GearAlloc_FreeData tmp = table->free_list[i];
        table->free_list[i] = table->free_list[min_idx];
        table->free_list[min_idx] = tmp;
    }

    /* Then merge neighboring frees */
    for(size_t i = 0; i < GEARALLOC_TABLE_HEADER_SIZE; ++i) {
        GearAlloc_FreeData* curr = &table->free_list[i];

        /* Indexes that are already squashed can be skipped */
        if(curr->size == 0) continue;

        for(size_t j = i+1; j < GEARALLOC_TABLE_HEADER_SIZE; ++j) {
            GearAlloc_FreeData* next = &table->free_list[j];

            if(next->size == 0) continue;

            if(curr->ptr+curr->size == next->ptr-size) {
                curr->size += next->size; /* Squash entries */
                next->size = 0; /* Mark next as empty */
            }
        } 
    }

    /* Failsafe for recursion*/
    GearAlloc_free_ptr_fail_defrag = true;
    GearAlloc_free_ptr(table, size, ptr);
    GearAlloc_free_ptr_fail_defrag = false;
}

void* GearAlloc_pool_malloc(
    size_t              alloc, 
    size_t              map_size, 
    GearAlloc_Table***  map_list, 
    size_t*             map_count
) {
    for(size_t i = 0; i < *map_count; ++i) {
        GearAlloc_Table* curr = (*map_list)[i];
        for(size_t j = 0; j < GEARALLOC_TABLE_HEADER_SIZE; ++j) {
            /* No free indexes available at this index */
            if(curr->free_list[j].size == 0) continue;

            /* Return the first index allowed at the index */
            void* result = curr->free_list[j].ptr;
            curr->free_list[j].ptr = (char*)curr->free_list[j].ptr + alloc;
            curr->free_list[j].size -= alloc;
            return result;
        }
    }

    /* no available memory, create new table */
    GearAlloc_Table new;
    void* page = mmap(
        NULL, 
        map_size+sizeof(GearAlloc_Table),
        PROT_READ | PROT_WRITE, 
        MAP_SHARED | MAP_ANON, 
        -1, 
        0
    );

    if(page == MAP_FAILED) {
        fprintf(stderr, "GearAlloc failed to allocate new page of %ld bytes.\n", map_size+sizeof(GearAlloc_Table));
        exit(-1);
    }

    new.page = page;

    /* gonna overwrite the first index anyway, so just skip it for now */
    for(size_t i = 1; i < GEARALLOC_TABLE_HEADER_SIZE; ++i) { 
        new.free_list[i].ptr = NULL;
        new.free_list[i].size = 0;
    }

    new.free_list[0].ptr = (void*)(((size_t)new.page)+alloc*2);
    new.free_list[0].size = map_size-alloc;

    *map_list = realloc(*map_list, ++*map_count * sizeof(GearAlloc_Table));
    (*map_list)[*map_count-1] = malloc(sizeof(GearAlloc_Table));

    memcpy((*map_list)[*map_count-1], &new, sizeof(GearAlloc_Table));
    return new.page;
};

void GearAlloc_pool_free(
    size_t  alloc, 
    size_t  map_size, 
    GearAlloc_Table*** map_list, 
    size_t  map_count,
    void*   ptr
) {
    for(size_t i = 0; i < map_count; ++i) {
        GearAlloc_Table* curr = (*map_list)[i];
        size_t offset = ptr - curr->page;

        /* +--------------------+
         * |      Page          |
         * +--------------------+
         * ^ offset             ^ offset + map_size
        */
        if(offset >= 0 && offset <= map_size) {
            return GearAlloc_free_ptr(curr, alloc, ptr);
        }
    }

    fprintf(stderr, "Pointer %p not found in allocator table.\n", ptr);
    exit(-1);
}