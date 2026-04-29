/*******************************************************************************************************************************************************
 * Copyright ?? 2016 <WIZnet Co.,Ltd.> 
 * Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the ??Software??), 
 * to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, 
 * and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

 * THE SOFTWARE IS PROVIDED ??AS IS??, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, 
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. 
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, 
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*********************************************************************************************************************************************************/
#include <stdio.h>
#include "W7500x_miim.h"

#ifdef __W7500P__
	// PB_05, PB_12 pull down
	*(volatile uint32_t *)(0x41003070) = 0x61;
	*(volatile uint32_t *)(0x41003054) = 0x61;
#endif

#define MAX_PHYID_CHECK_CNT     (2)

#define __DEF_DBG_LEVEL1__

#if (DEVICE_BOARD_NAME == WIZ750SR_T1L)
extern void mdio_delay(__IO uint32_t count);
#else
extern void delay(__IO uint32_t nCount);
#endif

// Default MDC/MDIO Pin settings for W7500P
// It can be changed by W7500x_MDC / W7500x_MDIO defines
static uint16_t MDIO = GPIO_Pin_15;
static uint16_t MDC  = GPIO_Pin_14;

uint32_t PHY_ADDR_IP101G; //(phy_id())
uint32_t PHY_ADDR;// PHY_ADDR_IP101G

uint32_t link(void)
{
    uint32_t phy_status = mdio_read(GPIOB, PHYREG_STATUS);  
    uint32_t phy_status_link;
    
    phy_status_link = (phy_status>>2)&0x01;
    
    return phy_status_link; 
}

void set_link(SetLink_Type mode)
{
   uint32_t val=0;
   assert_param(IS_SETLINK_TYPE(mode));
   
   if( mode == CNTL_AUTONEGO)
   {    
      val = CNTL_AUTONEGO; 
   }
   else
   {
        val = (mode & (CNTL_SPEED|CNTL_DUPLEX)); 
   }

    mdio_write(GPIOB, PHYREG_CONTROL, val);
}


void mdio_init(GPIO_TypeDef* GPIOx, uint16_t GPIO_Pin_MDC, uint16_t GPIO_Pin_MDIO)
{
    /* Set GPIOs for MDIO and MDC */
    GPIO_InitTypeDef GPIO_InitDef;  
    GPIO_StructInit(&GPIO_InitDef); // init the structure
    GPIO_InitDef.GPIO_Pin = GPIO_Pin_MDC | GPIO_Pin_MDIO;
    GPIO_InitDef.GPIO_Mode = GPIO_Mode_OUT;
    GPIO_Init(GPIOx, &GPIO_InitDef);

    PAD_AFConfig(PAD_PB, GPIO_Pin_MDIO, PAD_AF1);  
    PAD_AFConfig(PAD_PB, GPIO_Pin_MDC, PAD_AF1);  

    MDC = GPIO_Pin_MDC;
    MDIO = GPIO_Pin_MDIO;

    PHY_ADDR = (phy_id());
}


void output_MDIO(GPIO_TypeDef* GPIOx, uint32_t val, uint32_t n)
{
    for(val <<= (32-n); n; val<<=1, n--)
    {
        if(val & 0x80000000)
            GPIO_SetBits(GPIOx, MDIO); 
        else
            GPIO_ResetBits(GPIOx, MDIO);

#if (DEVICE_BOARD_NAME == WIZ750SR_T1L)
				mdio_delay(1);
#else
        delay(1);
#endif
        GPIO_SetBits(GPIOx, MDC); 
#if (DEVICE_BOARD_NAME == WIZ750SR_T1L)
				mdio_delay(1);
#else
        delay(1);
#endif
        GPIO_ResetBits(GPIOx, MDC);
    }
}

uint32_t input_MDIO(GPIO_TypeDef* GPIOx)
{
    uint32_t i, val=0; 
    for(i=0; i<16; i++)
    {
        val <<=1;
        GPIO_SetBits(GPIOx, MDC); 
#if (DEVICE_BOARD_NAME == WIZ750SR_T1L)
				mdio_delay(1);
#else
        delay(1);
#endif
        GPIO_ResetBits(GPIOx, MDC);
#if (DEVICE_BOARD_NAME == WIZ750SR_T1L)
				mdio_delay(1);
#else
        delay(1);
#endif
        val |= GPIO_ReadInputDataBit(GPIOx, MDIO);
    }
    return (val);
}

void turnaround_MDIO(GPIO_TypeDef* GPIOx)
{

    GPIOx->OUTENCLR = MDIO ;

#if (DEVICE_BOARD_NAME == WIZ750SR_T1L)
		mdio_delay(1);
#else
		delay(1);
#endif
    GPIO_SetBits(GPIOx, MDC); 
#if (DEVICE_BOARD_NAME == WIZ750SR_T1L)
		mdio_delay(1);
#else
		delay(1);
#endif
    GPIO_ResetBits(GPIOx, MDC);
#if (DEVICE_BOARD_NAME == WIZ750SR_T1L)
		mdio_delay(1);
#else
		delay(1);
#endif
}

void idle_MDIO(GPIO_TypeDef* GPIOx)
{

    GPIOx->OUTENSET = MDIO ;

    GPIO_SetBits(GPIOx,MDC); 
#if (DEVICE_BOARD_NAME == WIZ750SR_T1L)
		mdio_delay(1);
#else
		delay(1);
#endif
    GPIO_ResetBits(GPIOx, MDC);
#if (DEVICE_BOARD_NAME == WIZ750SR_T1L)
		mdio_delay(1);
#else
		delay(1);
#endif
}

uint32_t mdio_read(GPIO_TypeDef* GPIOx, uint32_t PhyRegAddr)
{
    uint32_t val =0;

    /* 32 Consecutive ones on MDO to establish sync */
    //printf("mdio read - sync \r\n");
    output_MDIO(GPIOx, 0xFFFFFFFF, 32);

    /* start code 01, read command (10) */
    //printf("mdio read - start \r\n");
    output_MDIO(GPIOx, 0x06, 4);

    /* write PHY address */
    //printf("mdio write- PHY address \r\n");
    output_MDIO(GPIOx, PHY_ADDR, 5);

    //printf("mdio read - PHY REG address \r\n");
    output_MDIO(GPIOx, PhyRegAddr, 5);

    /* turnaround MDO is tristated */
    //printf("mdio read - turnaround \r\n");
    turnaround_MDIO(GPIOx);

    /* Read the data value */
    //printf("mdio read - read the data value \r\n");
    val = input_MDIO(GPIOx);
    //printf("mdio read - val : %X\r\n", val );

    /* turnaround MDO is tristated */
    //printf("mdio read - idle \r\n");
    idle_MDIO(GPIOx);

    return val;
}

void mdio_write(GPIO_TypeDef* GPIOx, uint32_t PhyRegAddr, uint32_t val)
{
    /* 32 Consecutive ones on MDO to establish sync */
    //printf("mdio write- sync \r\n");
    output_MDIO(GPIOx, 0xFFFFFFFF, 32);

    /* start code 01, write command (01) */
    //printf("mdio write- start \r\n");
    output_MDIO(GPIOx, 0x05, 4);

    /* write PHY address */
    //printf("mdio write- PHY address \r\n");
    output_MDIO(GPIOx, PHY_ADDR, 5);
    
    //printf("mdio read - PHY REG address \r\n");
    output_MDIO(GPIOx, PhyRegAddr, 5);

    /* turnaround MDO */
    //printf("mdio write- turnaround (1,0)\r\n");
    output_MDIO(GPIOx, 0x02, 2);

    /* Write the data value */
    //printf("mdio writeread - read the data value \r\n");
    output_MDIO(GPIOx, val, 16);

    /* turnaround MDO is tristated */
    //printf("mdio write- idle \r\n");
    idle_MDIO(GPIOx);
}

int32_t phy_id(void)
{
    int32_t data;
    int i = 0;
    uint8_t cnt = 0;
    
    while(cnt < MAX_PHYID_CHECK_CNT)
    {
        for(i=0; i<=8; i+=1)
        {
            /* 32 Consecutive ones on MDO to establish sync */
            //printf("mdio read - sync \r\n");
            output_MDIO(GPIOB, 0xFFFFFFFF, 32);

            /* start code 01, read command (10) */
            //printf("mdio read - start \r\n");
            output_MDIO(GPIOB, 0x06, 4);

            /* write PHY address */
            //printf("mdio read - PHY address \r\n");
            output_MDIO(GPIOB, i, 5);

            //printf("mdio read - PHY REG address \r\n");
            output_MDIO(GPIOB, PHYREG_STATUS, 5);

            /* turnaround MDO is tristated */
            //printf("mdio read - turnaround \r\n");
            turnaround_MDIO(GPIOB);

            /* Read the data value */
            //printf("mdio read - read the data value \r\n");
            data = input_MDIO(GPIOB);
            //printf("mdio read - val : %X\r\n", val );

            /* turnaround MDO is tristated */
            //printf("mdio read - idle \r\n");
            idle_MDIO(GPIOB);
            
            //printf("\r\nPHY_ID = 0x%.4x , STATUS = %x, MDIO = 0x%.4x, MDC = 0x%.4x", i, data, MDIO, MDC);  //right : 0x786d // ## for debugging        
            if((data != 0x0) && (data != 0xFFFF)) {
							return i;
						}
            
            
        }
        cnt++;
        printf("\r\nphy id detect error!!\r\n");
    }
    return i;
}

uint32_t mdio_read_ext(GPIO_TypeDef* GPIOx, uint32_t PhyRegAddr)
{
	uint32_t val =0;
	
	mdio_write(GPIOx, 0x1E, PhyRegAddr);
  val = mdio_read(GPIOx, 0x1F);
	
	return val;
}

void mdio_write_ext(GPIO_TypeDef* GPIOx, uint32_t PhyRegAddr, uint32_t val)
{
	mdio_write(GPIOx, 0x1E, PhyRegAddr);
	mdio_write(GPIOx, 0x1F, val);
}

uint16_t mdio_read_mmd(GPIO_TypeDef* GPIOx, uint8_t devad, uint16_t reg_addr)
{
    // Step 1: Write DEVAD to 0x0D
    mdio_write(GPIOx, MMD_CTRL, devad);

    // Step 2: Write Register Address to 0x0E
    mdio_write(GPIOx, MMD_ADDR, reg_addr);

    // Step 3: Write (0x4000 | DEVAD) to 0x0D to enable data access
    mdio_write(GPIOx, MMD_CTRL, 0x4000 | devad);

    // Step 4: Read data from 0x0E
    return mdio_read(GPIOx, MMD_ADDR);
}

void mdio_write_mmd(GPIO_TypeDef* GPIOx, uint8_t devad, uint16_t reg_addr, uint16_t value)
{
    // Step 1: Write DEVAD to 0x0D
    mdio_write(GPIOx, MMD_CTRL, devad);

    // Step 2: Write Register Address to 0x0E
    mdio_write(GPIOx, MMD_ADDR, reg_addr);

    // Step 3: Write (0x4000 | DEVAD) to 0x0D to enable data access
    mdio_write(GPIOx, MMD_CTRL, 0x4000 | devad);

    // Step 4: Write data to 0x0E
    mdio_write(GPIOx, MMD_ADDR, value);
}

void YT8111_init()
{
    printf("[YT8111] Starting YT8111 PHY initialization...\n");
    
    mdio_write_ext(GPIOB, 0x0221, 0x2a2a);
    mdio_write_ext(GPIOB, 0x2506, 0x0255);
    
    mdio_write_ext(GPIOB, 0x0510, 0x64f0);
    mdio_write_ext(GPIOB, 0x0511, 0x70f0);
    mdio_write_ext(GPIOB, 0x0512, 0x78f0);
    mdio_write_ext(GPIOB, 0x0507, 0xff80);
    mdio_write_ext(GPIOB, 0xA401, 0xA04);
    mdio_write_ext(GPIOB, 0xA400, 0xA04);

    mdio_write_ext(GPIOB, 0xA108, 0x0300);
    mdio_write_ext(GPIOB, 0xA109, 0x0800);
    mdio_write_ext(GPIOB, 0xA304, 0x0004);
    mdio_write_ext(GPIOB, 0xA301, 0x0810);
    mdio_write_ext(GPIOB, 0x0500, 0x002f);

    mdio_write_ext(GPIOB, 0xA206, 0x1500);
    mdio_write_ext(GPIOB, 0xA203, 0x1414);
    mdio_write_ext(GPIOB, 0xA208, 0x1515);
    mdio_write_ext(GPIOB, 0xA209, 0x1314);
    mdio_write_ext(GPIOB, 0xA20B, 0x2D04);
    mdio_write_ext(GPIOB, 0xA20C, 0x1500);

    mdio_write_ext(GPIOB, 0x0522, 0x0FFF);
    mdio_write_ext(GPIOB, 0x0403, 0x00FF);

    mdio_write_ext(GPIOB, 0xA51F, 0x1070);
    mdio_write_ext(GPIOB, 0x051A, 0x6F00);
    mdio_write_ext(GPIOB, 0xA403, 0x1C00);
    mdio_write_ext(GPIOB, 0x0506, 0xFFE0);

    // Additional configuration
    mdio_write_ext(GPIOB, 0x2513, 0x3C1A);
    mdio_write_ext(GPIOB, 0x0528, 0x0020);
    mdio_write_ext(GPIOB, 0x0516, 0x0050);

    mdio_write_ext(GPIOB, 0x00, 0x9100); // PHY Reset

    printf("[YT8111] YT8111 PHY initialization completed\n");
}

