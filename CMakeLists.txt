cmake_minimum_required(VERSION 3.22)
project(NeZnayu C)
set(CMAKE_C_STANDARD 17)

set(PROJECT_SOURCES
    main.c
    chunk.c
    memory.c
    debug.c
    value.c
    vm.c
    compiler.c
    scanner.c
    object.c
    table.c
    global_vars.c
)

set(PROJECT_HEADERS
    common.h
)

add_executable(${PROJECT_NAME} ${PROJECT_SOURCES} ${PROJECT_HEADERS})

# Add math.h library
target_link_libraries(${PROJECT_NAME} m)