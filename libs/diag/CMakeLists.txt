cmake_minimum_required(VERSION 3.28.0)

message("Library diag")


set(INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/include)
set(SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src)

set(HEADER_FILES
    ${INCLUDE_DIR}/diag/Trace.h
)

set(SOURCES
    ${SRC_DIR}/Trace.c
    ${SRC_DIR}/trace_impl.c
)

add_library(diag OBJECT ${SOURCES} ${HEADER_FILES})

target_include_directories(diag PUBLIC 
 ${INCLUDE_DIR}
  ${INCLUDE_DIR}/diag
)

target_link_libraries(diag CMSIS)


