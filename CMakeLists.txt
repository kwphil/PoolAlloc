cmake_minimum_required(VERSION 3.20)

project(gear_alloc LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

if(DEBUG)
    message("-- Compiling in debug mode")
    set(CMAKE_BUILD_TYPE Debug)
    set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
    include(CheckCXXCompilerFlag)
    set(CUSTOM_FLAGS "-fsanitize=address,undefined,leak -O0 -g3")
else()
    message("-- Compiling in release mode")
    set(CMAKE_BUILD_TYPE Release)
    set(CUSTOM_FLAGS "-O3")
endif()

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC -Wall -Wextra -Wpedantic ${CUSTOM_FLAGS}")

set(GEAR_ALLOC
    src/core.c
)

add_library(gear_alloc SHARED ${GEAR_ALLOC})

target_include_directories(gear_alloc PUBLIC
    ${CMAKE_SOURCE_DIR}/include
)

if(TESTER)
    add_executable(alloc_test src/test.cpp)
    target_link_libraries(alloc_test gear_alloc) 

    target_include_directories(alloc_test PUBLIC
        ${CMAKE_SOURCE_DIR}/include
    )
endif()