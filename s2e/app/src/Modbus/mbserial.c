#include <stdlib.h>
#include "W7500x_uart.h"

#include "seg.h"
#include "uartHandler.h"
#include "common.h"
#include "mbserial.h"
#include "mbrtu.h"
#include "mbascii.h"

BUFFER_DECLARATION(data_rx);

/*****************************************************************************
    Private functions
 ****************************************************************************/

/**
    @brief	UART interrupt handler using ring buffers
    @return	Nothing
*/

int UART_read(void *data, int bytes) {
    uint32_t i;
    uint8_t *data_ptr = data;
    if (IS_BUFFER_EMPTY(data_rx)) {
        return RET_NOK;
    }

    for (i = 0; i < bytes; i++) {
        data_ptr[i] = (uint8_t)BUFFER_OUT(data_rx);
    }
    BUFFER_OUT_MOVE(data_rx, i);
    return i;
}

uint32_t UART_write(void *data, int bytes) {
    int i;
		uart_rs485_enable(SEG_DATA_UART);
    for(i = 0; i < bytes; i++) {
				
        uart_putc(SEG_DATA_UART, ((uint8_t *)data)[i]);
    }
		uart_rs485_disable(SEG_DATA_UART);
    return bytes;
}
