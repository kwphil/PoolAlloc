#include <GearAlloc/gear_alloc.hpp>

#include <chrono>
#include <vector>
#include <random>
#include <cassert>
#include <iostream>

#define ALLOCATION_COUNT 1'000

int main() {
    GearAlloc::PoolAllocer<int> manager;
    std::vector<int*> live;

    std::cout << "Running " << ALLOCATION_COUNT << " allocations" << std::endl;

    live.reserve(ALLOCATION_COUNT);

    std::chrono::nanoseconds alloc_max;
    std::chrono::nanoseconds count_alloc;
    for(int i = 0; i < ALLOCATION_COUNT; i++) {
        auto old_time = std::chrono::high_resolution_clock::now();
        int* p = manager.malloc();
        auto new_time = std::chrono::high_resolution_clock::now();

        auto time = std::chrono::duration_cast<std::chrono::nanoseconds>(new_time - old_time);

        if(time > alloc_max) alloc_max = time;

        count_alloc += time;

        live.push_back(p);
    }

    std::chrono::nanoseconds free_max;
    std::chrono::nanoseconds count_free;
    for(auto p : live) {
        auto old_time = std::chrono::high_resolution_clock::now();
        manager.free(p);
        auto new_time = std::chrono::high_resolution_clock::now();

        auto time = std::chrono::duration_cast<std::chrono::nanoseconds>(new_time - old_time);

        if(time > free_max) free_max = time;

        count_free += time;
    }

    count_alloc /= ALLOCATION_COUNT;
    count_free /= ALLOCATION_COUNT;

    std::cout << "Average allocation time: " << count_alloc.count() << "ns\n";
    std::cout << "Maximum allocation time: " << alloc_max.count() << "ns\n";
    std::cout << "Average free time: " << count_free.count() << "ns\n";
    std::cout << "Maximum free time: " << free_max.count() << "ns\n";

    return 0;
}