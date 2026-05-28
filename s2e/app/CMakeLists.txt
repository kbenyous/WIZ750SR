cmake_minimum_required(VERSION 3.28.0)

message("WIZ750SR S2E_App")



set(INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/include)
set(SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src)

set(HEADER_FILES
    ${INCLUDE_DIR}/Serial_to_Ethernet/seg.h
    ${INCLUDE_DIR}/W7500x_it.h
    ${INCLUDE_DIR}/PlatformHandler/timerHandler.h
    ${INCLUDE_DIR}/PlatformHandler/storageHandler.h
    ${INCLUDE_DIR}/PlatformHandler/eepromHandler.h
    ${INCLUDE_DIR}/PlatformHandler/uartHandler.h
    ${INCLUDE_DIR}/PlatformHandler/flashHandler.h
    ${INCLUDE_DIR}/PlatformHandler/i2cHandler.h
    ${INCLUDE_DIR}/PlatformHandler/gpioHandler.h
    ${INCLUDE_DIR}/PlatformHandler/httpHandler.h
    ${INCLUDE_DIR}/PlatformHandler/deviceHandler.h
    ${INCLUDE_DIR}/Modbus/mbtcp.h
    ${INCLUDE_DIR}/Modbus/mbtimer.h
    ${INCLUDE_DIR}/Modbus/mbcrc.h
    ${INCLUDE_DIR}/Modbus/mbserial.h
    ${INCLUDE_DIR}/Modbus/mbrtu.h
    ${INCLUDE_DIR}/Modbus/mbascii.h
    ${INCLUDE_DIR}/Modbus/mb.h
    ${INCLUDE_DIR}/Callback/dhcp_cb.h
    ${INCLUDE_DIR}/W7500x_board.h
    ${INCLUDE_DIR}/Configuration/util.h
    ${INCLUDE_DIR}/Configuration/segcp.h

)

set(SOURCES
    ${SRC_DIR}/main.c
    ${SRC_DIR}/Serial_to_Ethernet/seg.c
    ${SRC_DIR}/PlatformHandler/gpioHandler.c
    ${SRC_DIR}/PlatformHandler/storageHandler.c
    ${SRC_DIR}/PlatformHandler/timerHandler.c
    ${SRC_DIR}/PlatformHandler/httpHandler.c
    ${SRC_DIR}/PlatformHandler/isr_vector.c
    ${SRC_DIR}/PlatformHandler/eepromHandler.c
    ${SRC_DIR}/PlatformHandler/deviceHandler.c
    ${SRC_DIR}/PlatformHandler/uartHandler.c
    ${SRC_DIR}/PlatformHandler/flashHandler.c
    ${SRC_DIR}/Modbus/mbrtu.c
    ${SRC_DIR}/Modbus/mbascii.c
    ${SRC_DIR}/Modbus/mbserial.c
    ${SRC_DIR}/Modbus/mbtcp.c
    ${SRC_DIR}/Modbus/mb.c
    ${SRC_DIR}/Modbus/mbtimer.c
    ${SRC_DIR}/Modbus/mbcrc.c
    ${SRC_DIR}/Callback/dhcp_cb.c
    ${SRC_DIR}/retarget.c
    ${SRC_DIR}/Configuration/util.c
    ${SRC_DIR}/Configuration/segcp.c
    ${SRC_DIR}/W7500x_it.c
    ${SRC_DIR}/startup_W7500_app.S
)


add_executable(WIZ750SR_S2E_App ${SOURCES} ${HEADER_FILES})

# Generate gen-src/index.h from html/index.html (as `_acindex` byte array).
embed_file_to_header(WIZ750SR_S2E_App
    ${CMAKE_CURRENT_SOURCE_DIR}/html/index.html
    ${CMAKE_CURRENT_SOURCE_DIR}/gen-src/index.h
    _acindex
)

target_include_directories(WIZ750SR_S2E_App PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/gen-src
    ${INCLUDE_DIR}/
    ${INCLUDE_DIR}/Serial_to_Ethernet
    ${INCLUDE_DIR}/PlatformHandler
    ${INCLUDE_DIR}/Modbus
    ${INCLUDE_DIR}/Callback
    ${INCLUDE_DIR}/Configuration
)

target_link_libraries(WIZ750SR_S2E_App CMSIS W7500x_stdPeriph_Driver ioLibrary WIZ750SR_S2E_Common diag newlib cortexm)

target_link_options(WIZ750SR_S2E_App PRIVATE 
    --specs=nosys.specs
    --specs=nano.specs
    "SHELL:-T ${CMAKE_CURRENT_SOURCE_DIR}/linker/libs.ld"
    "SHELL:-T ${CMAKE_CURRENT_SOURCE_DIR}/linker/mem.ld"
    "SHELL:-T ${CMAKE_CURRENT_SOURCE_DIR}/linker/sections.ld"
    -mcpu=cortex-m0
    -mthumb    
    -lc
    -lm
    -Wl,--gc-sections,-Map=WIZ750SR_S2E_App.map,--cref

    -nostartfiles

    "SHELL:-Xlinker -print-memory-usage"

    #--verbose
)
set_target_properties(WIZ750SR_S2E_App PROPERTIES SUFFIX ".elf")

# Hex file : for combinasion with Boot, and generate complete BIN file
create_hex_file(WIZ750SR_S2E_App)

# Bin file : for direct use with OTA update
create_binary_file(WIZ750SR_S2E_App)