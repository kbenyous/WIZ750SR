#ifndef __ISR_DEF__
#define __ISR_DEF__

#include "W7500x_it.h"

//TODO KBEN Dedup file (isr_vector.c) between Boot and App, as they are identical
/*
 * Remap stubs — placed in the FLASH_REMAP region (0x7D00) by the linker.
 *
 * The boot vector table (startup_W7500.S) stores the actual addresses of these
 * functions. When the App is flashed, it overwrites this region with its own
 * stubs that redirect to App-level handlers.
 *
 * All functions share the single ".remap_handlers" section; the linker places
 * them contiguously inside FLASH_REMAP.
 */

#define REMAP __attribute__((section(".remap_handlers"), used, noinline))

void REMAP Remap_NMI_Handler(void)          { NMI_Handler();         }
void REMAP Remap_HardFault_Handler(void)    { HardFault_Handler();   }
void REMAP Remap_SVC_Handler(void)          { SVC_Handler();         }
void REMAP Remap_PendSV_Handler(void)       { PendSV_Handler();      }
void REMAP Remap_SysTick_Handler(void)      { SysTick_Handler();     }
void REMAP Remap_SSP0_Handler(void)         { SSP0_Handler();        }
void REMAP Remap_SSP1_Handler(void)         { SSP1_Handler();        }
void REMAP Remap_UART0_Handler(void)        { UART0_Handler();       }
void REMAP Remap_UART1_Handler(void)        { UART1_Handler();       }
void REMAP Remap_UART2_Handler(void)        { UART2_Handler();       }
void REMAP Remap_I2C0_Handler(void)         { I2C0_Handler();        }
void REMAP Remap_I2C1_Handler(void)         { I2C1_Handler();        }
void REMAP Remap_PORT0_Handler(void)        { PORT0_Handler();       }
void REMAP Remap_PORT1_Handler(void)        { PORT1_Handler();       }
void REMAP Remap_PORT2_Handler(void)        { PORT2_Handler();       }
void REMAP Remap_PORT3_Handler(void)        { PORT3_Handler();       }
void REMAP Remap_DMA_Handler(void)          { DMA_Handler();         }
void REMAP Remap_DUALTIMER0_Handler(void)   { DUALTIMER0_Handler();  }
void REMAP Remap_DUALTIMER1_Handler(void)   { DUALTIMER1_Handler();  }
void REMAP Remap_PWM0_Handler(void)         { PWM0_Handler();        }
void REMAP Remap_PWM1_Handler(void)         { PWM1_Handler();        }
void REMAP Remap_PWM2_Handler(void)         { PWM2_Handler();        }
void REMAP Remap_PWM3_Handler(void)         { PWM3_Handler();        }
void REMAP Remap_PWM4_Handler(void)         { PWM4_Handler();        }
void REMAP Remap_PWM5_Handler(void)         { PWM5_Handler();        }
void REMAP Remap_PWM6_Handler(void)         { PWM6_Handler();        }
void REMAP Remap_PWM7_Handler(void)         { PWM7_Handler();        }
void REMAP Remap_RTC_Handler(void)          { RTC_Handler();         }
void REMAP Remap_ADC_Handler(void)          { ADC_Handler();         }
void REMAP Remap_WZTOE_Handler(void)        { WZTOE_Handler();       }
void REMAP Remap_EXTI_Handler(void)         { EXTI_Handler();        }

#undef REMAP

#endif /* __ISR_DEF__ */
