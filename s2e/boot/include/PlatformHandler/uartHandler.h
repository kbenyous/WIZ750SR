#ifndef UARTHANDLER_H_
#define UARTHANDLER_H_

#include <stdint.h>
#include "common.h"
#include "W7500x_uart.h"
//#include "ConfigData.h"
#include "types/uart_types.h"

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



extern uint8_t * uart_if_table[];

void UART1_Configuration(void);
void UART2_Configuration(void);

#endif /* UARTHANDLER_H_ */
