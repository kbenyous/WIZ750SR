cmake_minimum_required(VERSION 3.28.0)

message("Library CMSIS")


set(INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/include)
set(SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src)

set(HEADER_FILES
    ${INCLUDE_DIR}/RTOS/cmsis_os.h
    ${INCLUDE_DIR}/ARM/core_cmFunc.h
    ${INCLUDE_DIR}/ARM/arm_const_structs.h
    ${INCLUDE_DIR}/ARM/arm_math.h
    ${INCLUDE_DIR}/ARM/core_cm0.h
    ${INCLUDE_DIR}/ARM/core_cmInstr.h
    ${INCLUDE_DIR}/ARM/arm_common_tables.h
    ${INCLUDE_DIR}/WIZnet/W7500/W7500x.h
    ${INCLUDE_DIR}/WIZnet/W7500/system_W7500x.h
    ${INCLUDE_DIR}/gcc_compat.h
)

set(SOURCES
    ${SRC_DIR}/WIZnet/W7500/system_W7500x.c
    #${SRC_DIR}/WIZnet/W7500/startup_W7500.S
)

add_library(CMSIS OBJECT ${SOURCES} ${HEADER_FILES})

target_include_directories(CMSIS PUBLIC 
 ${INCLUDE_DIR}/RTOS
  ${INCLUDE_DIR}/ARM
   ${INCLUDE_DIR}/WIZnet/W7500
)


