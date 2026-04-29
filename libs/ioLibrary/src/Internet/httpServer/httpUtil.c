/*******************************************************************************************************************************************************
 * Copyright ¡§I 2016 <WIZnet Co.,Ltd.> 
 * Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the ¢®¡ÆSoftware¢®¡¾), 
 * to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, 
 * and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

 * THE SOFTWARE IS PROVIDED ¢®¡ÆAS IS¢®¡¾, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, 
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. 
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, 
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*********************************************************************************************************************************************************/
/**
 * @file	httpUtil.c
 * @brief	HTTP Server Utilities	
 * @version 1.0
 * @date	2014/07/15
 * @par Revision
 *			2014/07/15 - 1.0 Release
 * @author	
 * \n\n @par Copyright (C) 1998 - 2014 WIZnet. All rights reserved.
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "W7500x_gpio.h"
#include "httpServer.h"
#include "httpParser.h"
#include "httpUtil.h"
#include "wizchip_conf.h"

uint8_t http_get_cgi_handler(uint8_t * uri_name, uint8_t * buf, uint32_t * file_len)
{
	uint8_t ret = HTTP_OK;
	uint16_t len = 0;
	
	if(predefined_get_cgi_processor(uri_name, buf, &len))
	{
		;
	}
	else
	{
		// CGI file not found
		ret = HTTP_FAILED;
	}

	if(ret)	*file_len = len;
	return ret;
}

uint8_t http_post_cgi_handler(uint8_t * uri_name, st_http_request * p_http_request, uint8_t * buf, uint32_t * file_len)
{
	uint8_t ret = HTTP_OK;
	uint16_t len = 0;
	uint8_t * device_ip;
	uint8_t val;

	if(predefined_set_cgi_processor(uri_name, p_http_request->URI, buf, &len))
	{
    ret = HTTP_RESET;
	}
	else
	{
		// CGI file not found
		ret = HTTP_FAILED;
	}

	if(ret)	*file_len = len;
	return ret;
}


uint8_t predefined_get_cgi_processor(uint8_t * uri_name, uint8_t * buf, uint16_t * len)
{
	uint8_t ret = 1;	// ret = 1 means 'uri_name' matched
	uint8_t cgibuf[14] = {0, };
	int8_t cgi_dio = -1;
	int8_t cgi_ain = -1;

	uint8_t i;
  
	if(strcmp((const char *)uri_name, "get_devinfo.cgi") == 0)
	{
    make_json_devinfo(buf, len);
	}
	else
	{
		;
	}

	return ret;

}


uint8_t predefined_set_cgi_processor(uint8_t * uri_name, uint8_t * uri, uint8_t * buf, uint16_t * len)
{
	uint8_t ret = 1;	// ret = 1 means 'uri_name' matched
	uint8_t val = 0;

	// Devinfo; devname
	if(strcmp((const char *)uri_name, "set_devinfo.cgi") == 0)
	{
		val = set_devinfo(uri);
    *len = sprintf((char *)buf, "%d", val);
	}
  else if(strcmp((const char *)uri_name, "set_devreset.cgi") == 0)
	{
		val = set_devreset(uri);
	  *len = sprintf((char *)buf, "%d", val);
	  //*len = sprintf((char *)buf,"<html><head><title>WIZ750SR - Configuration</title><body>Reset. Please wait a few seconds. <span style='color:red;'>DHCP server</span></body></html>\r\n\r\n");
	}
  else if(strcmp((const char *)uri_name, "set_devfacreset.cgi") == 0)
	{
		val = set_devfacreset(uri);
		*len = sprintf((char *)buf, "%d", val);
    //*len = sprintf((char *)buf,"<html><head><title>WIZ750SR - Configuration</title><body>Factory Reset Complete. Please wait a few seconds. <span style='color:red;'>DHCP server</span></body></html>\r\n\r\n");
	}
	else
	{
		ret = 0;
	}

  //*len = (*len)+ 1;
	return ret;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Pre-defined Set CGI functions
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

int8_t set_LED(uint8_t * uri)
{
	uint8_t buf[4];
	uint8_t * param;
	uint8_t pin = 0, val = 0;

		
	if((param = get_http_param_value((char *)uri, "pin", (char*)buf))) // GPIO; D0 ~ D15
	{
		pin = (uint8_t)ATOI(param, 10);
		
		if(pin > 15) return -1;

		if((param = get_http_param_value((char *)uri, "val", (char*)buf)))  // State; high(off)/low(on)
		{
			val = (uint8_t)ATOI(param, 10);
			
		}

		if(pin == RED)
		{
		    if(val == 0) 		GPIO_ResetBits(GPIOC, GPIO_Pin_0);	// Low(On)
		    else				GPIO_SetBits(GPIOC, GPIO_Pin_0);// High(off)
		}
		else if(pin == GREEN)
		{
		    if(val == 0) 		GPIO_ResetBits(GPIOC, GPIO_Pin_4);	// Low(On)
		    else				GPIO_SetBits(GPIOC, GPIO_Pin_4);// High(off)
		}
		else if(pin == BLUE)
		{
		    if(val == 0) 		GPIO_ResetBits(GPIOC, GPIO_Pin_5);	// Low(On)
		    else				GPIO_SetBits(GPIOC, GPIO_Pin_5);// High(off)
		}
	}

	return pin;
}

