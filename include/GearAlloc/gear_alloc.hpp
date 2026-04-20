#pragma once

#include <memory>
#include <vector>

#include "priv.h"

namespace GearAlloc {
    template<class T>
    class PoolAllocer {
    private:
        size_t pool_size;
        size_t map_size;   
        GearAlloc_Table** map_list;
        size_t map_count = 0;
    
    public:
        PoolAllocer(size_t page_size = GEARALLOC_DEFAULT_PAGE)
        : pool_size(sizeof(T)), map_size(page_size*pool_size) { }

        ~PoolAllocer() {
            for(size_t i = 0; i < map_count; ++i) {
                ::free(map_list[i]);
            }

            ::free(map_list);
        }

        inline T* malloc() { return reinterpret_cast<T*>(GearAlloc_pool_malloc(pool_size, map_size, &map_list, &map_count)); }
        inline void free(T* ptr) { GearAlloc_pool_free(pool_size, map_size, &map_list, map_count, ptr); }
    };
}