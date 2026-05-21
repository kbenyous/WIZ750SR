/*
 * fw_version.c — Firmware version marker embedded in flash.
 *
 * Format: "WIZFWVER:<major>.<minor>.<maintenance>:<status>:END"
 *
 * Extracted by scripts/wiz750sr_flash.py from a raw .bin (no source or
 * device required). Linked into both Boot and App via W750SR_S2E_Common,
 * so the combined firmware image carries one marker per partition.
 *
 * The .fw_version section must be KEEP()'d in each sections.ld, otherwise
 * --gc-sections drops it (the symbol has no internal references).
 */

#include "common.h"

#define _FW_STR(x) #x
#define FW_STR(x) _FW_STR(x)

__attribute__((used, section(".fw_version")))
const char fw_version_marker[] =
    "WIZFWVER:"
    FW_STR(MAJOR_VER) "." FW_STR(MINOR_VER) "." FW_STR(MAINTENANCE_VER)
    ":" STR_VERSION_STATUS ":END";
