cmake_minimum_required(VERSION 3.28.0)

message("Library newlib")


set(SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src)
set(SOURCES
    ${SRC_DIR}/_cxx.cpp
    ${SRC_DIR}/_exit.c
    ${SRC_DIR}/_sbrk.c
    ${SRC_DIR}/_startup.c
    ${SRC_DIR}/_syscalls.c
    ${SRC_DIR}/_write.c
    ${SRC_DIR}/assert.c

)

add_library(newlib OBJECT ${SOURCES})
target_link_libraries(newlib diag)
target_link_libraries(newlib cortexm)



