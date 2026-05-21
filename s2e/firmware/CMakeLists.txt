cmake_minimum_required(VERSION 3.28.0)

message("WIZ750SR S2E Firmware (Boot+App)")

set(FIRMWARE_DIR ${CMAKE_BINARY_DIR}/s2e/firmware)
set(BOOT_HEX    ${CMAKE_BINARY_DIR}/s2e/boot/WIZ750SR_S2E_Boot.hex)
set(APP_HEX     ${CMAKE_BINARY_DIR}/s2e/app/WIZ750SR_S2E_App.hex)

add_custom_command(
    OUTPUT ${FIRMWARE_DIR}/WIZ750SR_S2E_Firmware.hex
    COMMAND srec_cat
            ${BOOT_HEX} -Intel -exclude 0x7D00 0x7E00
            ${APP_HEX} -Intel
            -o ${FIRMWARE_DIR}/WIZ750SR_S2E_Firmware.hex -Intel
    DEPENDS WIZ750SR_S2E_Boot WIZ750SR_S2E_App
    COMMENT "Merging Boot + App into firmware hex"
)

add_custom_command(
    OUTPUT ${FIRMWARE_DIR}/WIZ750SR_S2E_Firmware.bin
    COMMAND ${CMAKE_OBJCOPY} -Obinary -Iihex ${FIRMWARE_DIR}/WIZ750SR_S2E_Firmware.hex  ${FIRMWARE_DIR}/WIZ750SR_S2E_Firmware.bin
    DEPENDS ${FIRMWARE_DIR}/WIZ750SR_S2E_Firmware.hex
    COMMENT "Converting firmware hex to bin"
)

add_custom_target(WIZ750SR_S2E_Firmware ALL
    DEPENDS ${FIRMWARE_DIR}/WIZ750SR_S2E_Firmware.bin
)
add_dependencies(WIZ750SR_S2E_Firmware WIZ750SR_S2E_Boot WIZ750SR_S2E_App)
