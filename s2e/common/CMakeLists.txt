cmake_minimum_required(VERSION 3.28.0)

message("Common configuration")

set(INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/include)
set(SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src)

set(HEADER_FILES
    ${INCLUDE_DIR}/common.h
    ${INCLUDE_DIR}/W7500x_board.h
    ${INCLUDE_DIR}/Configuration/ConfigData.h
    ${INCLUDE_DIR}/PlatformHandler/storageHandler.h
    ${INCLUDE_DIR}/PlatformHandler/i2cHandler.h
    ${INCLUDE_DIR}/PlatformHandler/eepromHandler.h
    ${INCLUDE_DIR}/Callback/dhcp_cb.h
    ${INCLUDE_DIR}/types/uart_types.h
    ${INCLUDE_DIR}/types/device_types.h
    ${INCLUDE_DIR}/types/modbus_types.h
)

set(SOURCES
    ${SRC_DIR}/ConfigData.c
    ${SRC_DIR}/W7500x_board.c
    ${SRC_DIR}/dhcp_cb.c
    # i2cHandler.c : bitbang impl, only compile when __USE_EXT_EEPROM__ is set
    # (otherwise collides with W7500x_i2c.c from W7500x_stdPeriph_Driver)
    ${SRC_DIR}/eepromHandler.c
    ${SRC_DIR}/fw_version.c
)


add_library(WIZ750SR_S2E_Common OBJECT  ${HEADER_FILES} ${SOURCES})
target_link_libraries(WIZ750SR_S2E_Common ioLibrary W7500x_stdPeriph_Driver)
target_include_directories(WIZ750SR_S2E_Common PUBLIC
    ${INCLUDE_DIR}
    ${INCLUDE_DIR}/Configuration/
    ${INCLUDE_DIR}/PlatformHandler/
    ${INCLUDE_DIR}/Callback/
)