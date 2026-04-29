/**
  ******************************************************************************
  * @file    W7500x Serial to Ethernet Project - WIZ750SR Boot
  * @author  Irina Kim, Team network
  * @version v1.3.2
  * @date    Dec-2019
  * @brief   Boot program body
  ******************************************************************************
  * @attention
  * @par Revision history
  *    <2019/12/03> v1.3.2 Bugfix and Improvements by irina
  *    <2019/11/22> v1.3.1 Bugfix and Improvements by irina
  *    <2019/10/11> v1.3.0 Bugfix and Improvements by irina
  *						    Change the Boot Code
  *    <2019/09/19> v1.2.5 Bugfix and Improvements by Becky
  *    <2018/06/22> v1.2.4 Bugfix by Eric Jung
  *    <2018/04/27> v1.2.3 Bugfix and Improvements by Eric Jung
  *    <2018/04/12> v1.2.2 Bugfix by Eric Jung
  *    <2018/04/11> v1.2.2 Bugfix and Improvements by Eric Jung(Pre-released Ver.)
  *    <2018/03/26> v1.2.1 Bugfix by Eric Jung
  *    <2018/03/16> v1.2.1 Bugfix by Eric Jung(Pre-released Ver.)
  *    <2018/03/12> v1.2.0 Bugfix and Improvements by Eric Jung
  *    <2018/02/08> v1.1.2 Bugfix by Eric Jung
  *    <2018/01/26> v1.1.2 Added WIZ750SR-1xx function by Edward Ahn
  *    <2017/12/13> v1.1.1 Develop by Eric Jung
  *    <2016/11/18> v1.1.0 Develop by Eric Jung
  *    <2016/03/29> v1.0.0 Develop by Eric Jung
  *    <2016/03/02> v0.8.0 Develop by Eric Jung
  *    <2015/11/24> v0.0.1 Develop by Eric Jung
  *
  * THE PRESENT FIRMWARE WHICH IS FOR GUIDANCE ONLY AIMS AT PROVIDING CUSTOMERS
  * WITH CODING INFORMATION REGARDING THEIR PRODUCTS IN ORDER FOR THEM TO SAVE
  * TIME. AS A RESULT, WIZnet SHALL NOT BE HELD LIABLE FOR ANY
  * DIRECT, INDIRECT OR CONSEQUENTIAL DAMAGES WITH RESPECT TO ANY CLAIMS ARISING
  * FROM THE CONTENT OF SUCH FIRMWARE AND/OR THE USE MADE BY CUSTOMERS OF THE
  * CODING INFORMATION CONTAINED HEREIN IN CONNECTION WITH THEIR PRODUCTS.
  *
  * <h2><center>&copy; COPYRIGHT 2016 WIZnet Co., Ltd.</center></h2>
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include <stdio.h>
#include <string.h>
#include "W7500x_gpio.h"
#include "W7500x_uart.h"
#include "W7500x_crg.h"
#include "W7500x_wztoe.h"
#include "W7500x_miim.h"
#include "W7500x_wdt.h"


#include "common.h"
#include "W7500x_board.h"

#ifdef __USE_APPBOOT_DHCP__
    #include "dhcp.h"
    #include "dhcp_cb.h"
#endif

#include "timerHandler.h"
#include "uartHandler.h"
#include "flashHandler.h"
#include "deviceHandler.h"
#include "eepromHandler.h"


#include "segcp.h"
#include "configData.h"


/* Private typedef -----------------------------------------------------------*/
typedef void (*pFunction)(void);

typedef struct
{
  __IO   uint32_t  REMAP;          /*!< Offset: 0x000 Remap Control Register (R/W) */
  __IO   uint32_t  PMUCTRL;        /*!< Offset: 0x004 PMU Control Register (R/W) */
  __IO   uint32_t  RESETOP;        /*!< Offset: 0x008 Reset Option Register  (R/W) */
  __IO   uint32_t  EMICTRL;        /*!< Offset: 0x00C EMI Control Register  (R/W) */
  __IO   uint32_t  RSTINFO;        /*!< Offset: 0x010 Reset Information Register (R/W) */
} W7500x_SYSCON_TypeDef;

/* Private define ------------------------------------------------------------*/
//#define _MAIN_DEBUG_	// debugging message enable

// Define for MAC address settings
#define MACSTR_SIZE		22

#define W7500x_SYSCON            ((W7500x_SYSCON_TypeDef *) W7500x_SYSCTRL_BASE)
#define W7500x_SYSCTRL_BASE      (0x4001F000)
#define SYSRESETREQ_Msk 0x1
#define WDTRESETREQ_Msk 0x2

/* Private function prototypes -----------------------------------------------*/
void application_jump(uint32_t AppAddress);
uint8_t check_mac_address(void);

static void W7500x_Init(void);
static void W7500x_WZTOE_Init(void);

// Debug messages
void display_Dev_Info_header(void);

#ifdef __USE_APPBOOT_DHCP__
    int8_t process_dhcp(void);
    void display_Dev_Info_dhcp(void);
#endif

// Functions for Interrupt Vector Table Remap
void Copy_Interrupt_VectorTable(uint32_t start_addr);

// Delay
void delay(__IO uint32_t milliseconds); //Notice: used ioLibray

/* Private variables ---------------------------------------------------------*/
static __IO uint32_t TimingDelay;
static WDT_InitTypeDef WDT_InitStructure;

/* Public variables ---------------------------------------------------------*/
// Shared buffer declaration
uint8_t g_send_buf[DATA_BUF_SIZE];
uint8_t g_recv_buf[DATA_BUF_SIZE];

uint8_t flag_s2e_application_running = OFF;
uint8_t flag_process_dhcp_success = OFF;
uint8_t flag_process_ip_success = OFF;

/**
  * @brief  Main program
  * @param  None
  * @retval None
  */
int main(void)
{
	DevConfig *dev_config = get_DevConfig_pointer();
	uint8_t appjump_enable = OFF;
	uint8_t ret = 0;
	int rst_info = 0;
	
	////////////////////////////////////////////////////////////////////////////////////////////////////
	// W7500x Hardware Initialize
	////////////////////////////////////////////////////////////////////////////////////////////////////
	
	/* W7500x MCU Initialization */
	W7500x_Init(); // includes UART2 init code for debugging

	/* W7500x WZTOE (Hardwired TCP/IP stack) Initialization */
	W7500x_WZTOE_Init();
	
	/* W7500x Board Initialization */
	W7500x_Board_Init();
	
	rst_info = W7500x_SYSCON->RSTINFO;
	if((rst_info & WDTRESETREQ_Msk) != 0) //Reset request is caused by WDT
	{
			WDT_IntClear();
	}
	
	////////////////////////////////////////////////////////////////////////////////////////////////////
	// W7500x Application: Initialize part1
	////////////////////////////////////////////////////////////////////////////////////////////////////
	
	/* Load the Configuration data */
	load_DevConfig_from_storage();
	
	////////////////////////////////////////////////////////////////////////////////////////////////////
	// W7500x Application: Check the MAC address and Firmware update
	////////////////////////////////////////////////////////////////////////////////////////////////////
	
	// 1. MAC address�� ���آ �ن؟ٌ				-> MAC ءض�ز �ش�آ �ط save
	
	if(check_mac_address())
	{
		// Copy_Interrupt_VectorTable(DEVICE_APP_MAIN_ADDR);
		// delay(SAVE_INTERVAL_MS/2);
		appjump_enable = ON;
	}
	
	// 2. Firmware update flag�� 1�خ �ن؟ٌ 		-> ��أ� ؟�؟� erase, Fwup_size��إ­ fw update ��اـ, app backup (flash) >> app main (flash)
	
	if(dev_config->firmware_update.fwup_flag == 1)
	{
		// Firmware download has already been done at application routine.
			// 1. 50kB app mode: Firmware copy: [App backup] -> [App main]
			// 2. 100kB app mode: Firmware download and write: [Network] -> [App main] (default)
		ret = device_firmware_update(STORAGE_APP_MAIN);
		
		dev_config->firmware_update.fwup_flag = SEGCP_DISABLE;
		dev_config->firmware_update.fwup_size = 0;
		
		dev_config->network_info[0].state = ST_OPEN;
		save_DevConfig_to_storage();
		
		// if(ret == DEVICE_FWUP_RET_SUCCESS)
		// {
		// 	Copy_Interrupt_VectorTable(DEVICE_APP_MAIN_ADDR);
		// 	delay(SAVE_INTERVAL_MS/2);
		// }
		appjump_enable = ON;
	}
	
	
	if ((*(uint32_t*)DEVICE_APP_MAIN_ADDR != 0xFFFFFFFF) && (*(uint32_t*)DEVICE_APP_BACKUP_ADDR != 0xFFFFFFFF))
	{
		appjump_enable = ON;
	}
	
	if (*(uint32_t*)DEVICE_APP_MAIN_ADDR == 0xFFFFFFFF) 
	{
#ifdef _MAIN_DEBUG_
		printf("\r\n >> Application Main: Empty [0x%.8x], Jump Failed\r\n", DEVICE_APP_MAIN_ADDR);
#endif
		appjump_enable = OFF;
	}
	else
	{
#ifdef _MAIN_DEBUG_
		printf("\r\n >> Application Main: Detected [0x%.8x], Jump Start\r\n", DEVICE_APP_MAIN_ADDR);
#endif
		appjump_enable = ON;
	}
	
	if(appjump_enable == ON)
	{
		if (dev_config->network_info[0].state == ST_BOOT) // for AppBoot cmd
		{
			appjump_enable = OFF;
			dev_config->network_info[0].state = ST_OPEN;
		
			save_DevConfig_to_storage();
		}
#ifdef __USE_BOOT_ENTRY__
		else if(get_boot_entry_pin() == 0) appjump_enable = OFF;
#endif
	}

	if(appjump_enable == ON)
	{
		application_jump(DEVICE_APP_MAIN_ADDR);
	}
	
	////////////////////////////////////////////////////////////////////////////////////////////////////
	// W7500x Application: Initialize part2
	////////////////////////////////////////////////////////////////////////////////////////////////////
	
	/* Set the device working mode: BOOT */
	dev_config->network_info[0].state = ST_BOOT;
	
	if(dev_config->serial_info[0].serial_debug_en)
	{
		// Debug UART: Device information print out
		display_Dev_Info_header();
	}
	
	////////////////////////////////////////////////////////////////////////////////////////////////////
	// W7500x Application: DHCP client handler
	////////////////////////////////////////////////////////////////////////////////////////////////////
#ifdef __USE_APPBOOT_DHCP__
	/* Network Configuration - DHCP client */
	// Initialize Network Information: DHCP or Static IP allocation
	if(dev_config->options.dhcp_use)
	{
		if(process_dhcp() == DHCP_IP_LEASED) // DHCP success
		{
			flag_process_dhcp_success = ON;
		}
		else // DHCP failed
		{
			Net_Conf(); // Set default static IP settings
		}
	}
	else
#endif
	{
		Net_Conf(); // Set default static IP settings
	}

	// Debug UART: Network information print out (includes DHCP IP allocation result)
	if(dev_config->serial_info[0].serial_debug_en)
	{
		display_Net_Info();

#ifdef __USE_APPBOOT_DHCP__
		display_Dev_Info_dhcp();
#endif
	}

	////////////////////////////////////////////////////////////////////////////////////////////////////
	// W7500x Boot: Main Routine
	////////////////////////////////////////////////////////////////////////////////////////////////////
	
	flag_s2e_application_running = ON;
	LED_On(LED1);

	while(1) // main loop
	{
		
		do_segcp();
		
#ifdef __USE_APPBOOT_DHCP__
		if(dev_config->options.dhcp_use) DHCP_run(); // DHCP client handler for IP renewal
#endif
		
		if(flag_check_main_routine)
		{
			// Device working indicator: Boot
			// LEDs blink rapidly (100ms)
			LED_Toggle(LED1);
			LED_Toggle(LED2);
			flag_check_main_routine = 0;
		}
		
		Time_Counter(); // Counter for replace the timer interrupt
	} // End of application main loop
} // End of main

/*****************************************************************************
 * Private functions
 ****************************************************************************/
void application_jump(uint32_t AppAddress)
{
	pFunction Jump_To_Application;
	uint32_t JumpAddress;
	
	JumpAddress = *( uint32_t*)(AppAddress + 0x04);
	Jump_To_Application = (pFunction)JumpAddress;
	
	// Initialize user application's stack pointer
	__set_MSP(*(__IO uint32_t *)AppAddress);
	
	// Jump to application
	Jump_To_Application();
}


static void W7500x_Init(void)
{
	////////////////////////////////////////////////////
	// W7500x MCU Initialize
	////////////////////////////////////////////////////
	
	/* Reset supervisory IC Init */
	Supervisory_IC_Init();
	
	/* Set System init */
	SystemInit_User(CLOCK_SOURCE_INTERNAL, PLL_SOURCE_8MHz, SYSTEM_CLOCK_8MHz);
	delay(10); // delay for system clock stabilization
	
	/* Counter Initialization */
	Time_Counter_Configuration(); // To replace the timer interrupt, inaccurate count value
	
	/* UART Initialization */
	UART2_Configuration(); // Simple UART (UART2) for Debugging
	
	/* SysTick_Config */
	//SysTick_Config((GetSystemClock()/1000));
	
#ifdef _MAIN_DEBUG_
	printf("\r\n >> W7500x MCU Clock Settings ===============\r\n"); 
	printf(" - GetPLLSource: %s, %lu (Hz)\r\n", GetPLLSource()?"External":"Internal", PLL_SOURCE_8MHz);
	printf(" - SetSystemClock: %lu (Hz) \r\n", SYSTEM_CLOCK_8MHz);
	printf(" - GetSystemClock: %d (Hz) \r\n", GetSystemClock());
#endif
}

static void W7500x_WZTOE_Init(void)
{
	////////////////////////////////////////////////////
	// W7500x WZTOE (Hardwired TCP/IP core) Initialize
	////////////////////////////////////////////////////
	
	/* Set Network Configuration: HW Socket Tx/Rx buffer size */
	//uint8_t tx_size[8] = { 2, 2, 2, 2, 2, 2, 2, 2 };
	//uint8_t rx_size[8] = { 2, 2, 2, 2, 2, 2, 2, 2 };
	
	/* Structure for timeout control: RTR, RCR */
	wiz_NetTimeout net_timeout;
	
	/* Software reset the WZTOE(Hardwired TCP/IP core) */
	wizchip_sw_reset();
	
	/* Set WZ_100US Register */
	setTIC100US((GetSystemClock()/10000));
#ifdef _MAIN_DEBUG_
	printf(" GetSystemClock: %X, getTIC100US: %X, (%X) \r\n", GetSystemClock(), getTIC100US(), *(uint32_t *)WZTOE_TIC100US); // for debugging
#endif
	/* Set TCP Timeout: retry count / timeout val */
	// Retry count default: [8], Timeout val default: [2000]
	//net_timeout.retry_cnt = 8;
	//net_timeout.time_100us = 2500;
	//wizchip_settimeout(&net_timeout);
	
#ifdef _MAIN_DEBUG_
	wizchip_gettimeout(&net_timeout); // ## for debugging - check the TCP timeout settings
	printf(" Network Timeout Settings - RCR: %d, RTR: %d\r\n", net_timeout.retry_cnt, net_timeout.time_100us);
#endif
	
	/* Set Network Configuration */
	//wizchip_init(tx_size, rx_size);
	
}

void display_Dev_Info_header(void)
{
	DevConfig *dev_config = get_DevConfig_pointer();

	printf("\r\n");
	printf("%s\r\n", STR_BAR);

#if (DEVICE_BOARD_NAME == WIZ750SR)
	printf(" WIZ750SR \r\n");
	printf(" >> WIZnet Serial to Ethernet Device\r\n");
#else
	#ifndef __W7500P__
		printf(" [W7500] Serial to Ethernet Device\r\n");
	#else
		printf(" [W7500P] Serial to Ethernet Device\r\n");
	#endif
#endif
	
	printf(" >> Firmware version: Boot %d.%d.%d %s\r\n", dev_config->fw_ver[0], dev_config->fw_ver[1], dev_config->fw_ver[2], STR_VERSION_STATUS);
	printf("%s\r\n", STR_BAR);
}

#ifdef __USE_APPBOOT_DHCP__
int8_t process_dhcp(void)
{
	uint8_t ret = 0;
	uint8_t dhcp_retry = 0;

#ifdef _MAIN_DEBUG_
	printf(" - DHCP Client running\r\n");
#endif
	DHCP_init(SOCK_DHCP, g_send_buf);
	reg_dhcp_cbfunc(w7500x_dhcp_assign, w7500x_dhcp_assign, w7500x_dhcp_conflict);
	
	while(1)
	{
		ret = DHCP_run();
		
		if(ret == DHCP_IP_LEASED)
		{
#ifdef _MAIN_DEBUG_
			printf(" - DHCP Success\r\n");
#endif
			break;
		}
		else if(ret == DHCP_FAILED)
		{
			dhcp_retry++;
#ifdef _MAIN_DEBUG_
			if(dhcp_retry <= 3) printf(" - DHCP Timeout occurred and retry [%d]\r\n", dhcp_retry);
#endif
		}

		if(dhcp_retry > 3)
		{
#ifdef _MAIN_DEBUG_
			printf(" - DHCP Failed\r\n\r\n");
#endif
			DHCP_stop();
			break;
		}

		do_segcp(); // Process the requests of configuration tool during the DHCP client run.
		Time_Counter(); // Counter for replace the timer interrupt
	}
		
	return ret;
}

void display_Dev_Info_dhcp(void)
{
	DevConfig *dev_config = get_DevConfig_pointer();
	
	if(dev_config->options.dhcp_use) 
	{
		if(flag_process_dhcp_success == ON) printf(" # DHCP IP Leased time : %u seconds\r\n", getDHCPLeasetime());
		else printf(" # DHCP Failed\r\n");
	}
}
#endif

//////////////////////////////////////////////////////////////////////////////////
// Functions for MAC address 
//////////////////////////////////////////////////////////////////////////////////

uint8_t check_mac_address(void)
{
	DevConfig *dev_config = get_DevConfig_pointer();
	
	uint8_t i;
	uint8_t mac_buf[6] = {0, };
	uint8_t mac_str[MACSTR_SIZE] = {0, };
	uint8_t trep[MACSTR_SIZE] = {0, };
	uint8_t ret = 0;
	
    // ## 20180208 Modified by Eric, WIZnet MAC address check procedure removed  
	if((dev_config->network_info_common.mac[0] == 0xff) && (dev_config->network_info_common.mac[1] == 0xff) && (dev_config->network_info_common.mac[2] == 0xff))
	{
		read_storage(STORAGE_MAC, 0, mac_buf, 0);

		// Initial stage: Input the MAC address
        // ## 20180208 Modified by Eric, WIZnet MAC address check procedure removed
		if((mac_buf[0] == 0xff) && (mac_buf[1] == 0xff) && (mac_buf[2] == 0xff))
		{
			gSEGCPPRIVILEGE = SEGCP_PRIVILEGE_CLR;
			gSEGCPPRIVILEGE = (SEGCP_PRIVILEGE_SET|SEGCP_PRIVILEGE_WRITE);
			
			while(1)
			{
				printf("INPUT FIRST MAC?\r\n");
				for(i = 0; i < MACSTR_SIZE-1; i++)
				{
					mac_str[i] = S_UartGetc();
				}
				
				if(!proc_SEGCP(mac_str, trep))
				{
					// Save the MAC address to PRIVATE SPACE for MAC address only
					erase_storage(STORAGE_MAC);
					write_storage(STORAGE_MAC, 0, dev_config->network_info_common.mac, 0);
					
					// Set factory default
					set_DevConfig_to_factory_value();
					save_DevConfig_to_storage();
					
					ret = 1;
					break;
				}
				else
				{
					if(dev_config->serial_info[0].serial_debug_en) printf("%s\r\n", trep);
					memset(mac_str, 0x00, MACSTR_SIZE);
				}
			}
		}
		else // Lost the MAC address, MAC address restore
		{
            // ## 20180208 Modified by Eric, MAC address check and re-save procedure removed
            dev_config->network_info_common.mac[0] = mac_buf[0];
            dev_config->network_info_common.mac[1] = mac_buf[1];
            dev_config->network_info_common.mac[2] = mac_buf[2];
            dev_config->network_info_common.mac[3] = mac_buf[3];
            dev_config->network_info_common.mac[4] = mac_buf[4];
            dev_config->network_info_common.mac[5] = mac_buf[5];
		}
		
	}
	else
	{
		ret = 1;
	}
	
	return ret;
}


//////////////////////////////////////////////////////////////////////////////////
// Functions for Interrupt Vector Table Remap
//////////////////////////////////////////////////////////////////////////////////
void Copy_Interrupt_VectorTable(uint32_t start_addr)
{
	uint32_t i;
	uint8_t flash_vector_area[SECT_SIZE];
	
	for (i = 0x00; i < 0x08; i++)			flash_vector_area[i] = *(volatile uint8_t *)(0x00000000+i);
	for (i = 0x08; i < 0xA8; i++) 			flash_vector_area[i] = *(volatile uint8_t *)(start_addr+i); // Actual address range; Interrupt vector table is located here
	for (i = 0xA8; i < SECT_SIZE; i++)	flash_vector_area[i] = *(volatile uint8_t *)(0x00000000+i);
	
	__disable_irq();
	
	DO_IAP(IAP_ERAS_SECT, 0x00000000, 0, 0); 						// Erase the interrupt vector table area : Sector 0
	DO_IAP(IAP_PROG, 0x00000000, flash_vector_area , SECT_SIZE);	// Write the applicaion vector table to 0x00000000
	
	__enable_irq();
}

/**
  * @brief  Inserts a delay time when the situation cannot use the timer interrupt.
  * @param  ms: specifies the delay time length, in milliseconds.
  * @retval None
  */
void delay(__IO uint32_t milliseconds)
{
	volatile uint32_t nCount;
	
	nCount=(GetSystemClock()/10000)*milliseconds;
	for (; nCount!=0; nCount--);
}

void mdio_delay(__IO uint32_t count) 
{ 
	volatile uint32_t i; 
	
	for (i = 0; i < count * 260; i++) 
	{ 
		__asm volatile ("nop"); 
	} 
}
