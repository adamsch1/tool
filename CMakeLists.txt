project(tool)
set(CMAKE_C_FLAGS "-fpic -mavx -std=c11 -pedantic -g -Wall -Wextra -lm")
set(CMAKE_C_FLAGS_RELEASE "-g")
set(CMAKE_CXX_FLAGS "-g -std=c++11")

set(source tool.c chunk.c sorter.c streamvbytedelta.c streamvbyte.c )

add_executable(tool ${source})
target_link_libraries(tool m)
