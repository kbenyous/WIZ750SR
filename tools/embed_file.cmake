# Portable embed-a-binary-as-a-C-array helper.
#
# Dual-purpose file:
#   * Included as a CMake module to expose the embed_file_to_header() function.
#   * Invoked via `cmake -P tools/embed_file.cmake -DSRC=... -DDST=... -DVAR=...`
#     to actually generate the header (this is what add_custom_command runs).
#
# Portable to Linux / Windows / macOS: relies only on CMake itself
# (no xxd, no sed, no python).

# --- Script mode: generate the header and exit ---------------------------------
if(CMAKE_SCRIPT_MODE_FILE)
    if(NOT SRC OR NOT DST OR NOT VAR)
        message(FATAL_ERROR "embed_file: SRC, DST and VAR must be defined")
    endif()
    file(READ "${SRC}" HEX_CONTENT HEX)
    string(LENGTH "${HEX_CONTENT}" HEX_LEN)
    math(EXPR ARR_LEN "${HEX_LEN} / 2")
    string(REGEX MATCHALL ".." BYTES "${HEX_CONTENT}")
    set(BODY "")
    set(COL 0)
    foreach(B IN LISTS BYTES)
        string(APPEND BODY "0x${B}, ")
        math(EXPR COL "${COL} + 1")
        if(COL EQUAL 16)
            string(APPEND BODY "\n  ")
            set(COL 0)
        endif()
    endforeach()
    get_filename_component(DST_DIR "${DST}" DIRECTORY)
    file(MAKE_DIRECTORY "${DST_DIR}")
    get_filename_component(SRC_NAME "${SRC}" NAME)
    file(WRITE "${DST}"
"/* Auto-generated from ${SRC_NAME} by embed_file_to_header. Do not edit. */\n"
"static const unsigned char ${VAR}[${ARR_LEN}] = {\n  ${BODY}\n};\n")
    return()
endif()


# --- Module mode: define the function ------------------------------------------
# Generates a header that exposes SRC as `static const unsigned char VAR[]`,
# regenerated whenever SRC changes, and attached as a source of TARGET.
function(embed_file_to_header TARGET SRC DST VAR)
    add_custom_command(
        OUTPUT ${DST}
        COMMAND ${CMAKE_COMMAND}
            -DSRC=${SRC} -DDST=${DST} -DVAR=${VAR}
            -P ${CMAKE_CURRENT_FUNCTION_LIST_FILE}
        DEPENDS ${SRC} ${CMAKE_CURRENT_FUNCTION_LIST_FILE}
        COMMENT "Embedding ${SRC} -> ${DST}"
        VERBATIM
    )
    target_sources(${TARGET} PRIVATE ${DST})
endfunction()
