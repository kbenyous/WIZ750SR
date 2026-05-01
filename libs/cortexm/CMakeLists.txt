cmake_minimum_required(VERSION 3.28.0)

message("Library cortexm")


set(INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/include)
set(SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src)

set(HEADER_FILES
    ${INCLUDE_DIR}/cortexm/ExceptionHandlers.h
)
set(SOURCES
    ${SRC_DIR}/_initialize_hardware.c
    ${SRC_DIR}/_reset_hardware.c
    ${SRC_DIR}/exception_handlers.c
)

add_library(cortexm OBJECT ${SOURCES} ${HEADER_FILES})

target_include_directories(cortexm PUBLIC 
 ${INCLUDE_DIR}
    ${INCLUDE_DIR}/cortexm
)

target_link_libraries(cortexm CMSIS)
target_link_libraries(cortexm diag)


