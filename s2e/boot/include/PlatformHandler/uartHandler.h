#ifndef UARTHANDLER_H_
#define UARTHANDLER_H_

#include <stdint.h>
#include "common.h"
#include "W7500x_uart.h"
#include "ConfigData.h"

// UART interface selector, RS-232/TTL or RS-422/485
#define UART_IF_RS232_TTL			0
#define UART_IF_RS422_485			1
#define UART_IF_STR_RS232_TTL		"RS-232/TTL"
#define UART_IF_STR_RS422_485		"RS-422/485"

#define UART_IF_RS422				0
#define UART_IF_RS485				1
#define UART_IF_RS485_REVERSE		2				//Added by James in March 29
// If the define '__USE_UART_IF_SELECTOR__' disabled, default UART interface is selected to be 'UART_IF_DEFAULT'
#define UART_IF_DEFAULT				UART_IF_RS232_TTL
//#define UART_IF_DEFAULT				UART_IF_RS422_485

enum baud {
	baud_300 = 0,
	baud_600 = 1,
	baud_1200 = 2,
	baud_1800 = 3,
	baud_2400 = 4,
	baud_4800 = 5,
	baud_9600 = 6,
	baud_14400 = 7,
	baud_19200 = 8,
	baud_28800 = 9,
	baud_38400 = 10,
	baud_57600 = 11,
	baud_115200 = 12,
	baud_230400 = 13,
    baud_460800 = 14
};

enum word_len {
	word_len7 = 0,
	word_len8 = 1,
	word_len9 = 2
};

enum stop_bit {
	stop_bit1 = 0,
	stop_bit2 = 1
};

enum parity {
	parity_none = 0,
	parity_odd = 1,
	parity_even = 2
};

enum flow_ctrl {
	flow_none = 0,
	flow_xon_xoff = 1,
	flow_rts_cts = 2,
	flow_rtsonly = 3,  // RTS_ONLY
	flow_reverserts = 4 // Reverse RTS
};

extern uint8_t * uart_if_table[];

void UART1_Configuration(void);
void UART2_Configuration(void);

#endif /* UARTHANDLER_H_ */
