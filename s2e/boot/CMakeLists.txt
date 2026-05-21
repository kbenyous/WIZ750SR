cmake_minimum_required(VERSION 3.28.0)

message("WIZ750SR S2E_Boot")



set(INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/include)
set(SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src)

set(HEADER_FILES
    ${INCLUDE_DIR}/W7500x_it.h
    ${INCLUDE_DIR}/PlatformHandler/timerHandler.h
    ${INCLUDE_DIR}/PlatformHandler/storageHandler.h
    ${INCLUDE_DIR}/PlatformHandler/eepromHandler.h
    ${INCLUDE_DIR}/PlatformHandler/uartHandler.h
    ${INCLUDE_DIR}/PlatformHandler/flashHandler.h
#    ${INCLUDE_DIR}/PlatformHandler/i2cHandler.h
    ${INCLUDE_DIR}/PlatformHandler/deviceHandler.h
    ${INCLUDE_DIR}/Callback/dhcp_cb.h
    ${INCLUDE_DIR}/Configuration/util.h
    ${INCLUDE_DIR}/Configuration/segcp.h
)

set(SOURCES
    ${SRC_DIR}/main.c
#    ${SRC_DIR}/PlatformHandler/i2cHandler.c
    ${SRC_DIR}/PlatformHandler/storageHandler.c
    ${SRC_DIR}/PlatformHandler/timerHandler.c
    ${SRC_DIR}/PlatformHandler/isr_vector.c
    ${SRC_DIR}/PlatformHandler/eepromHandler.c
    ${SRC_DIR}/PlatformHandler/deviceHandler.c
    ${SRC_DIR}/PlatformHandler/uartHandler.c
    ${SRC_DIR}/PlatformHandler/flashHandler.c
    ${SRC_DIR}/Callback/dhcp_cb.c
    ${SRC_DIR}/retarget.c
    ${SRC_DIR}/Configuration/util.c
    ${SRC_DIR}/Configuration/segcp.c
    ${SRC_DIR}/W7500x_it.c
    ${SRC_DIR}/startup_W7500_boot.S
)


add_executable(WIZ750SR_S2E_Boot ${SOURCES} ${HEADER_FILES})

target_include_directories(WIZ750SR_S2E_Boot PRIVATE
${INCLUDE_DIR}
${INCLUDE_DIR}/PlatformHandler
${INCLUDE_DIR}/Callback
${INCLUDE_DIR}/Configuration
)

target_link_libraries(WIZ750SR_S2E_Boot CMSIS W7500x_stdPeriph_Driver ioLibrary WIZ750SR_S2E_Common diag newlib cortexm)

target_link_options(WIZ750SR_S2E_Boot PRIVATE 
    --specs=nosys.specs
    --specs=nano.specs
    "SHELL:-T ${CMAKE_CURRENT_SOURCE_DIR}/linker/libs.ld"
    "SHELL:-T ${CMAKE_CURRENT_SOURCE_DIR}/linker/mem.ld"
    "SHELL:-T ${CMAKE_CURRENT_SOURCE_DIR}/linker/sections.ld"
    -mcpu=cortex-m0
    -mthumb    
    -lc
    -lm
    -Wl,--gc-sections,-Map=WIZ750SR_S2E_Boot.map,--cref

    -nostartfiles

    "SHELL:-Xlinker -print-memory-usage"

    #--verbose
)
set_target_properties(WIZ750SR_S2E_Boot PROPERTIES SUFFIX ".elf")

# Boot hex : to be combined with App hex, and then converted to full firmware bin
create_hex_file(WIZ750SR_S2E_Boot)