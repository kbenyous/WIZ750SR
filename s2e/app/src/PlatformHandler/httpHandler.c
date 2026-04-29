
#include <stdio.h>
#include <string.h>
#include "common.h"
#include "W7500x_wztoe.h"
#include "W7500x_board.h"
#include "ConfigData.h"
#include "storageHandler.h"
#include "deviceHandler.h"
#include "segcp.h"
#include "uartHandler.h"
#include "httpHandler.h"
#include "httpParser.h"

void make_json_devinfo(uint8_t * buf, uint16_t * len)
{
  DevConfig *dev_config = get_DevConfig_pointer();
  uint8_t uart_sel = 0;
 	uint8_t baudrate_index[2] = {0, };
	uint8_t databit_index[2] = {0, };
	uint8_t stopbit_index[2] = {0, };

	// for UART0 / UART1, uart_sel = 0 or 1
	if(uart_sel > 1) uart_sel = 0;

  baudrate_index[uart_sel] = dev_config->serial_info[uart_sel].baud_rate;
	databit_index[uart_sel] = dev_config->serial_info[uart_sel].data_bits;
  stopbit_index[uart_sel] = dev_config->serial_info[uart_sel].stop_bits;

  
	*len = sprintf((char *)buf, "DevinfoCallback({\"fwver\":\"%d.%d.%d_%s\","\
                                               "\"devname\":\"%s\","\
                                               "\"pcode\":\"%d-%d-%d\","\
                                               "\"mac\":\"%02X:%02X:%02X:%02X:%02X:%02X\","\
                                               "\"ip\":\"%d.%d.%d.%d\","\
                                               "\"gw\":\"%d.%d.%d.%d\","\
                                               "\"sub\":\"%d.%d.%d.%d\","\
                                               "\"dns\":\"%d.%d.%d.%d\","\
                                               "\"dhcp\":\"%d\","\
                                               "\"opmode\":\"%d\","\
                                               "\"lport\":\"%d\","\
                                               "\"rip\":\"%d.%d.%d.%d\","\
                                               "\"rport\":\"%d\","\
                                               "\"uart\":\"%d\","\
                                               "\"baud\":\"%d\","\
                                               "\"databit\":\"%d\","\
                                               "\"parity\":\"%d\","\
                                               "\"stopbit\":\"%d\","\
                                               "\"flow\":\"%d\","\
                                               "\"modbus\":\"%d\""\
                                               "});",
		dev_config->fw_ver[0], dev_config->fw_ver[1], dev_config->fw_ver[2], STR_VERSION_STATUS,
		dev_config->module_name,
		dev_config->module_type[0],dev_config->module_type[1],dev_config->module_type[2],
		dev_config->network_info_common.mac[0],dev_config->network_info_common.mac[1],dev_config->network_info_common.mac[2],dev_config->network_info_common.mac[3],dev_config->network_info_common.mac[4],dev_config->network_info_common.mac[5],
    dev_config->network_info_common.local_ip[0],dev_config->network_info_common.local_ip[1],dev_config->network_info_common.local_ip[2],dev_config->network_info_common.local_ip[3],
		dev_config->network_info_common.gateway[0],dev_config->network_info_common.gateway[1],dev_config->network_info_common.gateway[2],dev_config->network_info_common.gateway[3],
		dev_config->network_info_common.subnet[0],dev_config->network_info_common.subnet[1],dev_config->network_info_common.subnet[2],dev_config->network_info_common.subnet[3],
		dev_config->options.dns_server_ip[0],dev_config->options.dns_server_ip[1],dev_config->options.dns_server_ip[2],dev_config->options.dns_server_ip[3],
		dev_config->options.dhcp_use,
		dev_config->network_info[0].working_mode,
		dev_config->network_info[0].local_port,
		dev_config->network_info[0].remote_ip[0],dev_config->network_info[0].remote_ip[1],dev_config->network_info[0].remote_ip[2],dev_config->network_info[0].remote_ip[3],
		dev_config->network_info[0].remote_port,
		uart_sel,
		baudrate_index[uart_sel],
		databit_index[uart_sel],
		dev_config->serial_info[uart_sel].parity,
		stopbit_index[uart_sel],
		dev_config->serial_info[uart_sel].flow_control,
		dev_config->modbus_enable
    );
}

uint8_t set_devinfo(uint8_t * uri)
{
	uint8_t buf[8];
	uint8_t ret = 0;
	uint8_t * param;
	uint8_t str_size;
	DevConfig *dev_config = get_DevConfig_pointer();
  uint8_t uart_sel = 0;
  
	if((param = get_http_param_value((char *)uri, "devname", (char*)buf)))
	{
		memset(dev_config->module_name, 0x00, 15);
		if((str_size = strlen((char*)param)) > 14) str_size = 14; // exception handling
		memcpy(dev_config->module_name, param, str_size);

		ret = 1;
	}

	if((param = get_http_param_value((char *)uri, "dhcp", (char*)buf)))
	{
		if(strstr((char const*)param, "1") != NULL) dev_config->options.dhcp_use = 1; // DHCP mode
		else dev_config->options.dhcp_use = 0; // Static mode
		ret = 1;
	}

	if(dev_config->options.dhcp_use == 0) // Static mode
	{
		if((param = get_http_param_value((char *)uri, "ip", (char*)buf)))
		{
			inet_addr_((unsigned char*)param, dev_config->network_info_common.local_ip);
			ret = 1;
		}
		if((param = get_http_param_value((char *)uri, "gw", (char*)buf)))
		{
			inet_addr_((unsigned char*)param, dev_config->network_info_common.gateway);
			ret = 1;
		}
		if((param = get_http_param_value((char *)uri, "sub", (char*)buf)))
		{
			inet_addr_((unsigned char*)param, dev_config->network_info_common.subnet);
			ret = 1;
		}
		if((param = get_http_param_value((char *)uri, "dns", (char*)buf)))
		{
			inet_addr_((unsigned char*)param, dev_config->options.dns_server_ip);
			ret = 1;
		}
	}

	if((param = get_http_param_value((char *)uri, "opmode", (char*)buf)))
	{
	  dev_config->network_info[0].working_mode = ATOI(param, 10);
	}

  if((param = get_http_param_value((char *)uri, "lport", (char*)buf)))
  {
    dev_config->network_info[0].local_port = ATOI(param, 10);
    ret = 1;
  }
  
  if(dev_config->network_info[0].working_mode != TCP_SERVER_MODE)
  {
    if((param = get_http_param_value((char *)uri, "rip", (char*)buf)))
		{
      inet_addr_((unsigned char*)param, dev_config->network_info[0].remote_ip);
			ret = 1;
		}

    if((param = get_http_param_value((char *)uri, "rport", (char*)buf)))
    {
		  dev_config->network_info[0].remote_port = ATOI(param, 10);
			ret = 1;
    }
  }

	if((param = get_http_param_value((char *)uri, "baud", (char*)buf)))
	{
		uint8_t baudrate_idx = ATOI(param, 10);
		if(baudrate_idx > baud_230400) baudrate_idx = baud_115200; // 115200
		dev_config->serial_info[uart_sel].baud_rate = baudrate_idx;
		ret = 1;
	}
	if((param = get_http_param_value((char *)uri, "databit", (char*)buf)))
	{
    dev_config->serial_info[uart_sel].data_bits = ATOI(param, 10);
		ret = 1;
	}
	if((param = get_http_param_value((char *)uri, "parity", (char*)buf)))
	{
    dev_config->serial_info[uart_sel].parity = ATOI(param, 10);
		ret = 1;
	}
	if((param = get_http_param_value((char *)uri, "stopbit", (char*)buf)))
	{
    dev_config->serial_info[uart_sel].stop_bits = ATOI(param, 10);
		ret = 1;
	}
	if((param = get_http_param_value((char *)uri, "flow", (char*)buf)))
	{
    dev_config->serial_info[uart_sel].flow_control = ATOI(param, 10);
		ret = 1;
	}
	if((param = get_http_param_value((char *)uri, "modbus", (char*)buf)))
	{
    dev_config->modbus_enable = ATOI(param, 10);
		ret = 1;
	}

	if(ret == 1){
    flash_update_start();
    save_DevConfig_to_storage();
    flash_update_end();
	}
	return ret;
}


uint8_t set_devreset(uint8_t * uri)
{
	uint8_t ret = 0;

  ret = 1;
	return ret;
}

uint8_t set_devfacreset(uint8_t * uri)
{
	uint8_t ret = 0;
	uint8_t * param;

  flash_update_start();
  device_set_factory_default();
  flash_update_end();

  ret = 1;
	return ret;
}

