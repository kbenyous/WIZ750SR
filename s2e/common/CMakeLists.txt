cmake_minimum_required(VERSION 3.28.0)

message("Common configuration")

set(INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/include)
set(SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src)

set(HEADER_FILES
    ${INCLUDE_DIR}/common.h
    ${INCLUDE_DIR}/W7500x_board.h
    ${INCLUDE_DIR}/Configuration/ConfigData.h
    ${INCLUDE_DIR}/PlatformHandler/storageHandler.h
    ${INCLUDE_DIR}/types/uart_types.h
    ${INCLUDE_DIR}/types/device_types.h
    ${INCLUDE_DIR}/types/modbus_types.h  
)

set(SOURCES
    ${SRC_DIR}/ConfigData.c
    ${SRC_DIR}/W7500x_board.c
)


add_library(W750SR_S2E_Common OBJECT  ${HEADER_FILES} ${SOURCES})
target_link_libraries(W750SR_S2E_Common ioLibrary W7500x_stdPeriph_Driver)
target_include_directories(W750SR_S2E_Common PUBLIC
 ${INCLUDE_DIR}
  ${INCLUDE_DIR}/Configuration/
    ${INCLUDE_DIR}/PlatformHandler/
)