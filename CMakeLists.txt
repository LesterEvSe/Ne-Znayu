cmake_minimum_required(VERSION 3.22)
project(NeZnayu C)
set(CMAKE_C_STANDARD 17)

set(PROJECT_SOURCES
    main.c
    chunk.c
    memory.c
    debug.c
    value.c
)
add_executable(${PROJECT_NAME} ${PROJECT_SOURCES})
