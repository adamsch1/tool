project(fetch)
cmake_minimum_required(VERSION 3.2)

set(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR})
find_package(ICU REQUIRED)
find_package(CURL REQUIRED)

set(CMAKE_C_FLAGS "-I/usr/local/include -fpic -mavx -std=gnu11 -pedantic -g -Wall -Wextra -lm -I/usr/include/x86_64-linux-gnu  -licuio -licui18n -licuuc -licudata -lcurl -I/usr/local/lib")
set(CMAKE_C_FLAGS_RELEASE "-g")
set(CMAKE_CXX_FLAGS "-g -std=c++11")
set(fetch fetch.cc util.c)
set(con con.cc util.c)
set(inv inv.c util.c)
set(fcat fcat.c forward.c)
set(merger merger.c util.c)
add_executable(fetch ${fetch})
add_executable(con ${con})
add_executable(inv ${inv})
add_executable(merger ${merger})
add_executable(fcat ${fcat})
target_link_libraries(fetch m ICU::ICU curl gumbo)
