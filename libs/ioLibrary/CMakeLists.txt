cmake_minimum_required(VERSION 3.28.0)

message("Library ioLibrary")


set(INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/include)
set(SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src)

set(HEADER_FILES
    ${INCLUDE_DIR}/Application/loopback/loopback.h
    ${INCLUDE_DIR}/Internet/DNS/dns.h
    ${INCLUDE_DIR}/Internet/DHCP/dhcp.h
    ${INCLUDE_DIR}/Internet/httpServer/webpage.h
    ${INCLUDE_DIR}/Internet/httpServer/httpParser.h
    ${INCLUDE_DIR}/Internet/httpServer/httpUtil.h
    ${INCLUDE_DIR}/Internet/httpServer/httpServer.h
    ${INCLUDE_DIR}/Ethernet/wizchip_conf.h
    ${INCLUDE_DIR}/Ethernet/socket.h
    ${INCLUDE_DIR}/MDIO/W7500x_miim.h
)

set(SOURCES
    ${SRC_DIR}/Application/loopback/loopback.c
    ${SRC_DIR}/Internet/DNS/dns.c
    ${SRC_DIR}/Internet/DHCP/dhcp.c
    ${SRC_DIR}/Internet/httpServer/httpParser.c
    ${SRC_DIR}/Internet/httpServer/httpServer.c
    ${SRC_DIR}/Internet/httpServer/httpUtil.c
    ${SRC_DIR}/Ethernet/socket.c
    ${SRC_DIR}/Ethernet/wizchip_conf.c
    ${SRC_DIR}/MDIO/W7500x_miim.c
)

add_library(ioLibrary OBJECT ${SOURCES} ${HEADER_FILES})

target_include_directories(ioLibrary PUBLIC 
    ${INCLUDE_DIR}/Application/loopback
    ${INCLUDE_DIR}/Internet/DNS
    ${INCLUDE_DIR}/Internet/DHCP
    ${INCLUDE_DIR}/Internet/httpServer
    ${INCLUDE_DIR}/Ethernet
    ${INCLUDE_DIR}/MDIO
)

target_link_libraries(ioLibrary CMSIS)
target_link_libraries(ioLibrary W7500x_stdPeriph_Driver)


