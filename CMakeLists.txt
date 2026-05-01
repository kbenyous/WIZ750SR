cmake_minimum_required(VERSION 3.28.0)

include(${CMAKE_SOURCE_DIR}/tools/arm-none-eabi-gcc.cmake)
include(${CMAKE_SOURCE_DIR}/tools/functions.cmake)

project(WIZ750SR VERSION 0.1.0 LANGUAGES C ASM) 

add_compile_options(
    -mcpu=cortex-m0
    -mthumb
    #-mabi=aapcs
    -fdata-sections     
    -fdiagnostics-color=always
#    -Wall
#    -O2
    -Og
    -g3
    -fmessage-length=0
    -fsigned-char
    -ffunction-sections 
    --specs=nano.specs    
    -DUSE_STDPERIPH_DRIVER
    -D_MAIN_DEBUG_
#   -DTRACE
#   -DOS_USE_TRACE_SEMIHOSTING_DEBUG
#   -DDEBUG
#   --verbose
)

add_subdirectory(libs/CMSIS)
add_subdirectory(libs/W7500x_stdPeriph_Driver)
add_subdirectory(libs/ioLibrary)
add_subdirectory(libs/newlib)
add_subdirectory(libs/diag)
add_subdirectory(libs/cortexm)
add_subdirectory(s2e/common)
add_subdirectory(s2e/boot)
add_subdirectory(s2e/app)
add_subdirectory(s2e/firmware)

