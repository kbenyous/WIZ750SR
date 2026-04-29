#ifndef __ISR_DEF__
#define __ISR_DEF__


#include "W7500x_it.h"

void Remap_NMI_Handler(void)            __attribute__((section (".ARM.__at_0x00007D00")));		//__attribute__((section (".ARM.__at_0x0001FD00")));
void Remap_HardFault_Handler(void)   	__attribute__((section (".ARM.__at_0x00007D08")));		//__attribute__((section (".ARM.__at_0x0001FD08)")));
void Remap_SVC_Handler(void)  	 		__attribute__((section (".ARM.__at_0x00007D0C"))); 		//__attribute__((section (".ARM.__at_0x0001FD0C"))); 
void Remap_PendSV_Handler(void)   		__attribute__((section (".ARM.__at_0x00007D14"))); 		//__attribute__((section (".ARM.__at_0x0001FD14")));  
void Remap_SysTick_Handler(void)   		__attribute__((section (".ARM.__at_0x00007D1C"))); 		//__attribute__((section (".ARM.__at_0x0001FD1C"))); 	
void Remap_SSP0_Handler(void)   		__attribute__((section (".ARM.__at_0x00007D24"))); 		//__attribute__((section (".ARM.__at_0x0001FD24")));                      
void Remap_SSP1_Handler(void)   		__attribute__((section (".ARM.__at_0x00007D2C"))); 		//__attribute__((section (".ARM.__at_0x0001FD2C")));                      
void Remap_UART0_Handler(void)   		__attribute__((section (".ARM.__at_0x00007D34"))); 		//__attribute__((section (".ARM.__at_0x0001FD34"))); 
void Remap_UART1_Handler(void)   		__attribute__((section (".ARM.__at_0x00007D3C"))); 		//__attribute__((section (".ARM.__at_0x0001FD3C")));                     
void Remap_UART2_Handler(void)   		__attribute__((section (".ARM.__at_0x00007D44"))); 		//__attribute__((section (".ARM.__at_0x0001FD44")));                     
void Remap_I2C0_Handler(void)   		__attribute__((section (".ARM.__at_0x00007D4C"))); 		//__attribute__((section (".ARM.__at_0x0001FD4C")));                     
void Remap_I2C1_Handler(void)   		__attribute__((section (".ARM.__at_0x00007D54"))); 		//__attribute__((section (".ARM.__at_0x0001FD54")));                     
void Remap_PORT0_Handler(void)  		__attribute__((section (".ARM.__at_0x00007D5C"))); 		//__attribute__((section (".ARM.__at_0x0001FD5C")));                     
void Remap_PORT1_Handler(void)  		__attribute__((section (".ARM.__at_0x00007D64"))); 		//__attribute__((section (".ARM.__at_0x0001FD64")));                     
void Remap_PORT2_Handler(void)  		__attribute__((section (".ARM.__at_0x00007D6C"))); 		//__attribute__((section (".ARM.__at_0x0001FD6C")));                     
void Remap_PORT3_Handler(void)  		__attribute__((section (".ARM.__at_0x00007D74"))); 		//__attribute__((section (".ARM.__at_0x0001FD74")));                     
void Remap_DMA_Handler(void)   			__attribute__((section (".ARM.__at_0x00007D7C"))); 		//__attribute__((section (".ARM.__at_0x0001FD7C")));                     
void Remap_DUALTIMER0_Handler(void)   	__attribute__((section (".ARM.__at_0x00007D84"))); 		//__attribute__((section (".ARM.__at_0x0001FD84")));                     
void Remap_DUALTIMER1_Handler(void)   	__attribute__((section (".ARM.__at_0x00007D8C"))); 		//__attribute__((section (".ARM.__at_0x0001FD8C")));                     
void Remap_PWM0_Handler(void)   		__attribute__((section (".ARM.__at_0x00007D94"))); 		//__attribute__((section (".ARM.__at_0x0001FD94")));                     
void Remap_PWM1_Handler(void)   		__attribute__((section (".ARM.__at_0x00007D9C"))); 		//__attribute__((section (".ARM.__at_0x0001FD9C")));                     
void Remap_PWM2_Handler(void)   		__attribute__((section (".ARM.__at_0x00007DA4"))); 		//__attribute__((section (".ARM.__at_0x0001FDA4")));                     
void Remap_PWM3_Handler(void)   		__attribute__((section (".ARM.__at_0x00007DAC"))); 		//__attribute__((section (".ARM.__at_0x0001FDAC")));                     
void Remap_PWM4_Handler(void)   		__attribute__((section (".ARM.__at_0x00007DB4"))); 		//__attribute__((section (".ARM.__at_0x0001FDB4")));                     
void Remap_PWM5_Handler(void)   		__attribute__((section (".ARM.__at_0x00007DBC"))); 		//__attribute__((section (".ARM.__at_0x0001FDBC")));                     
void Remap_PWM6_Handler(void)   		__attribute__((section (".ARM.__at_0x00007DC4"))); 		//__attribute__((section (".ARM.__at_0x0001FDC4")));                     
void Remap_PWM7_Handler(void)   		__attribute__((section (".ARM.__at_0x00007DCC"))); 		//__attribute__((section (".ARM.__at_0x0001FDCC")));                     
void Remap_RTC_Handler(void)   			__attribute__((section (".ARM.__at_0x00007DD4"))); 		//__attribute__((section (".ARM.__at_0x0001FDD4")));                     
void Remap_ADC_Handler(void)   			__attribute__((section (".ARM.__at_0x00007DDC"))); 		//__attribute__((section (".ARM.__at_0x0001FDDC")));                     
void Remap_WZTOE_Handler(void)   		__attribute__((section (".ARM.__at_0x00007DE4"))); 		//__attribute__((section (".ARM.__at_0x0001FDE4")));                     
void Remap_EXTI_Handler(void)   		__attribute__((section (".ARM.__at_0x00007DEC")));		//__attribute__((section (".ARM.__at_0x0001FDEC")));

void Remap_NMI_Handler(void)
{
   NMI_Handler();   
}
void Remap_SVC_Handler(void)  
{
	SVC_Handler();  
}	 		   
void Remap_PendSV_Handler(void)   		 
{
	PendSV_Handler();   		 
}	 		   
void Remap_SysTick_Handler(void)   			
{
	SysTick_Handler();   			
}	 		   
void Remap_SSP0_Handler(void)   		                     
{
	SSP0_Handler();   		    
}	 		   
void Remap_SSP1_Handler(void)   		                     
{
	SSP1_Handler();   		                     
}	 		   
void Remap_UART0_Handler(void)   		
{
	UART0_Handler();   		
}	 		   
void Remap_UART1_Handler(void)   		                    
{
	UART1_Handler();   		
}	 		   
void Remap_UART2_Handler(void)   		                    
{
	UART2_Handler();   		
}	 		   
void Remap_I2C0_Handler(void)   		                    
{
	I2C0_Handler();   		
}	 		   
void Remap_I2C1_Handler(void)   		                    
{
	I2C1_Handler();   		
}	 		   
void Remap_PORT0_Handler(void)  		                    
{
	PORT0_Handler();  		
}	 		   
void Remap_PORT1_Handler(void)  		                    
{
	PORT1_Handler();  		
}	 		   
void Remap_PORT2_Handler(void)  		                    
{
	PORT2_Handler();  		
}	 		   
void Remap_PORT3_Handler(void)  		                    
{
	PORT3_Handler();  		
}	 		   
void Remap_DMA_Handler(void)   			                    
{
	DMA_Handler();   			
}	 		   
void Remap_DUALTIMER0_Handler(void)   	                    
{
	DUALTIMER0_Handler();   	
}	 		   
void Remap_DUALTIMER1_Handler(void)   	                    
{
	DUALTIMER1_Handler();   	
}	 		   
void Remap_PWM0_Handler(void)   		                    
{
	PWM0_Handler();   		
}	 		   
void Remap_PWM1_Handler(void)   		                    
{
	PWM1_Handler();  
}	 		   
void Remap_PWM2_Handler(void)   		                    
{
	PWM2_Handler();  
}	 		   
void Remap_PWM3_Handler(void)   		                    
{
	PWM3_Handler();  
}	 		   
void Remap_PWM4_Handler(void)   		                    
{
	PWM4_Handler();  
}	 		   
void Remap_PWM5_Handler(void)   		                    
{
	PWM5_Handler();  
}	 		   
void Remap_PWM6_Handler(void)   		                    
{
	PWM6_Handler();  
}	 		   
void Remap_PWM7_Handler(void)   		                    
{
	PWM7_Handler();  
}	 		   
void Remap_RTC_Handler(void)   			                    
{
	RTC_Handler();   
}	 		   
void Remap_ADC_Handler(void)   			                    
{
	ADC_Handler();  
}	 		   
void Remap_WZTOE_Handler(void)
{
	WZTOE_Handler();
}	 		      		                    
void Remap_EXTI_Handler(void)   		                    
{
	EXTI_Handler(); 
}	
#endif