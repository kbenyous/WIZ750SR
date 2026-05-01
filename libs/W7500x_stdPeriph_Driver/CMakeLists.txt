cmake_minimum_required(VERSION 3.28.0)

message("Library W7500_stdPeriph_Driver")


set(INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/include)
set(SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src)

set(HEADER_FILES
    ${INCLUDE_DIR}/W7500x_wztoe.h
    ${INCLUDE_DIR}/W7500x_uart.h
    ${INCLUDE_DIR}/W7500x_rng.h
    ${INCLUDE_DIR}/W7500x_wdt.h
    ${INCLUDE_DIR}/W7500x_dualtimer.h
    ${INCLUDE_DIR}/W7500x_exti.h
    ${INCLUDE_DIR}/W7500x_adc.h
    ${INCLUDE_DIR}/W7500x_gpio.h
    ${INCLUDE_DIR}/W7500x_dma.h
    ${INCLUDE_DIR}/W7500x_ssp.h
    ${INCLUDE_DIR}/W7500x_i2c.h
    ${INCLUDE_DIR}/W7500x_pwm.h
    ${INCLUDE_DIR}/W7500x_crg.h
)

set(SOURCES
    ${SRC_DIR}/W7500x_exti.c
    ${SRC_DIR}/W7500x_wdt.c
    ${SRC_DIR}/W7500x_adc.c
    ${SRC_DIR}/W7500x_uart.c
    ${SRC_DIR}/W7500x_gpio.c
    ${SRC_DIR}/W7500x_dma.c
    ${SRC_DIR}/W7500x_i2c.c
    ${SRC_DIR}/W7500x_pwm.c
    ${SRC_DIR}/W7500x_crg.c
    ${SRC_DIR}/W7500x_ssp.c
    ${SRC_DIR}/W7500x_wztoe.c
    ${SRC_DIR}/W7500x_rng.c
    ${SRC_DIR}/W7500x_dualtimer.c
)

add_library(W7500x_stdPeriph_Driver OBJECT ${SOURCES} ${HEADER_FILES})

target_link_libraries(W7500x_stdPeriph_Driver CMSIS)
target_include_directories(W7500x_stdPeriph_Driver PUBLIC 
 ${INCLUDE_DIR}
)
