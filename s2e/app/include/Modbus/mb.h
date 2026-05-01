#ifndef _MB_H_
#define _MB_H_

#include "types/modbus_types.h"

void mbTCPtoRTU(uint8_t sock);
void mbRTUtoTCP(uint8_t sock);
void mbASCIItoTCP(uint8_t sock);
void mbTCPtoASCII(uint8_t sock);
#endif

