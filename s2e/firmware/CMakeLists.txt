cmake_minimum_required(VERSION 3.28.0)

message("WIZ750SR S2E Firware (Boot+App)")

set(FIRMWARE_DIR ${CMAKE_BINARY_DIR}/s2e/firmware)
set(BOOT_HEX    ${CMAKE_BINARY_DIR}/s2e/boot/W750SR_S2E_Boot.hex)
set(APP_HEX     ${CMAKE_BINARY_DIR}/s2e/app/W750SR_S2E_App.hex)

add_custom_command(
    OUTPUT ${FIRMWARE_DIR}/W750SR_Firmware.hex
    COMMAND srec_cat 
            ${BOOT_HEX} -Intel -exclude 0x7D00 0x7E00
            ${APP_HEX} -Intel
            -o ${FIRMWARE_DIR}/W750SR_Firmware.hex -Intel
    DEPENDS W750SR_S2E_Boot W750SR_S2E_App
    COMMENT "Merging Boot + App into firmware hex"
)

add_custom_command(
    OUTPUT ${FIRMWARE_DIR}/W750SR_Firmware.bin
    COMMAND ${CMAKE_OBJCOPY} -Obinary -Iihex ${FIRMWARE_DIR}/W750SR_Firmware.hex  ${FIRMWARE_DIR}/W750SR_Firmware.bin
    DEPENDS ${FIRMWARE_DIR}/W750SR_Firmware.hex
    COMMENT "Converting firmware hex to bin"
)

add_custom_target(WIZ750SR_firmware ALL
    DEPENDS ${FIRMWARE_DIR}/W750SR_Firmware.bin
)
add_dependencies(WIZ750SR_firmware W750SR_S2E_Boot W750SR_S2E_App)

