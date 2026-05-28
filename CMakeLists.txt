cmake_minimum_required(VERSION 3.28.0)

include(${CMAKE_SOURCE_DIR}/tools/arm-none-eabi-gcc.cmake)
include(${CMAKE_SOURCE_DIR}/tools/functions.cmake)
include(${CMAKE_SOURCE_DIR}/tools/embed_file.cmake)

project(WIZ750SR VERSION 0.1.0 LANGUAGES C ASM) 

# Default to Debug if no build type is set (e.g. plain `cmake ..` without preset)
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Debug CACHE STRING "Build type" FORCE)
    set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS Debug Release MinSizeRel RelWithDebInfo)
endif()

add_compile_options(
    # --- Architecture (Cortex-M0, Thumb only) ---
    -mcpu=cortex-m0
    -mthumb
    #-mabi=aapcs

    # --- Code layout for linker --gc-sections (see s2e/app + s2e/boot) ---
    -ffunction-sections
    -fdata-sections

    # --- Language / ABI ---
    -funsigned-char                       # char is signed by default on ARM EABI
    --specs=nano.specs                    # also at compile: defines _REENT_SMALL etc.

    # --- Diagnostics ---
    -Wall
    -Wextra
    -Wshadow
    -Wno-pointer-sign                     # safe: char ≡ uint8_t with -funsigned-char
    -fdiagnostics-color=always
    -fmessage-length=0

    # --- Debug info: kept in every config; lives in .elf only, not flashed ---
    -g3

    # --- Per-configuration optimization ---
    $<$<CONFIG:Debug>:-Og>
    $<$<CONFIG:Release>:-Os>              # size-optimized: W7500 has 128 KB Flash
    $<$<CONFIG:MinSizeRel>:-Os>
    $<$<CONFIG:RelWithDebInfo>:-Os>

    # --- Project defines ---
    -DUSE_STDPERIPH_DRIVER
    $<$<CONFIG:Debug>:-D_MAIN_DEBUG_>
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

