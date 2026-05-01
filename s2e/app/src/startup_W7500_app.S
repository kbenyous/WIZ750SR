/* File: startup_W7500_app.S
 * Purpose: Startup file for W7500x App project (GCC)
 *
 * This startup is placed at the App base address (0x7000).
 * The vector table here serves two purposes:
 *   1) application_jump() reads [0x7000]=SP and [0x7004]=Reset_Handler
 *   2) The interrupt entries point to Remap_* trampolines (same as Boot)
 *
 * Note: On Cortex-M0 (no VTOR), the CPU always fetches vectors from 0x0000.
 * This table at 0x7000 is NOT read by the NVIC for interrupt dispatch; it's
 * only used by the Boot's application_jump() to find SP and Reset_Handler.
 *
 * Based on CMSIS startup for Cortex-M0, adapted for WIZ750SR.
 */

    .syntax unified
    .arch armv6-m

/* ========== Stack ========== */
    .section .stack
    .align 3
#ifdef __STACK_SIZE
    .equ    Stack_Size, __STACK_SIZE
#else
    .equ    Stack_Size, 0x800
#endif
    .globl  __StackTop
    .globl  __StackLimit
__StackLimit:
    .space  Stack_Size
    .size   __StackLimit, . - __StackLimit
__StackTop:
    .size   __StackTop, . - __StackTop

/* ========== Heap ========== */
    .section .heap
    .align 3
#ifdef __HEAP_SIZE
    .equ    Heap_Size, __HEAP_SIZE
#else
    .equ    Heap_Size, 0x400
#endif
    .globl  __HeapBase
    .globl  __HeapLimit
__HeapBase:
    .if     Heap_Size
    .space  Heap_Size
    .endif
    .size   __HeapBase, . - __HeapBase
__HeapLimit:
    .size   __HeapLimit, . - __HeapLimit

/* ========== Vector Table (App: at 0x7000, read by application_jump) ========== */

    .section .isr_vector
    .align 2
    .globl __isr_vector
__isr_vector:
    .long   __stack                     /* Top of Stack — read by application_jump */
    .long   Reset_Handler               /* Reset Handler — jumped to by Boot      */
    .long   NMI_Handler                 /* NMI Handler                            */
    .long   HardFault_Handler           /* Hard Fault Handler                     */
    .long   0                           /* Reserved                               */
    .long   0                           /* Reserved                               */
    .long   0                           /* Reserved                               */
    .long   0                           /* Reserved                               */
    .long   0                           /* Reserved                               */
    .long   0                           /* Reserved                               */
    .long   0                           /* Reserved                               */
    .long   SVC_Handler                 /* SVCall Handler                         */
    .long   0                           /* Reserved                               */
    .long   0                           /* Reserved                               */
    .long   PendSV_Handler              /* PendSV Handler                         */
    .long   SysTick_Handler             /* SysTick Handler                        */

    /* External Interrupts */
    .long   SSP0_Handler                /* 16+ 0: SSP 0                           */
    .long   SSP1_Handler                /* 16+ 1: SSP 1                           */
    .long   UART0_Handler               /* 16+ 2: UART 0                          */
    .long   UART1_Handler               /* 16+ 3: UART 1                          */
    .long   UART2_Handler               /* 16+ 4: UART 2                          */
    .long   I2C0_Handler                /* 16+ 5: I2C 0                           */
    .long   I2C1_Handler                /* 16+ 6: I2C 1                           */
    .long   PORT0_Handler               /* 16+ 7: GPIO Port 0                     */
    .long   PORT1_Handler               /* 16+ 8: GPIO Port 1                     */
    .long   PORT2_Handler               /* 16+ 9: GPIO Port 2                     */
    .long   PORT3_Handler               /* 16+10: GPIO Port 3                     */
    .long   DMA_Handler                 /* 16+11: DMA                             */
    .long   DUALTIMER0_Handler          /* 16+12: Dual timer 0                    */
    .long   DUALTIMER1_Handler          /* 16+13: Dual timer 1                    */
    .long   PWM0_Handler                /* 16+14: PWM0                            */
    .long   PWM1_Handler                /* 16+15: PWM1                            */
    .long   PWM2_Handler                /* 16+16: PWM2                            */
    .long   PWM3_Handler                /* 16+17: PWM3                            */
    .long   PWM4_Handler                /* 16+18: PWM4                            */
    .long   PWM5_Handler                /* 16+19: PWM5                            */
    .long   PWM6_Handler                /* 16+20: PWM6                            */
    .long   PWM7_Handler                /* 16+21: PWM7                            */
    .long   RTC_Handler                 /* 16+22: RTC                             */
    .long   ADC_Handler                 /* 16+23: ADC                             */
    .long   WZTOE_Handler               /* 16+24: WZTOE                           */
    .long   EXTI_Handler                /* 16+25: EXTI                            */

    .size   __isr_vector, . - __isr_vector

/* Reset_Handler is defined in libs/cortexm/src/exception_handlers.c
 * and delegates to _start (libs/newlib/src/_startup.c), which performs
 * the full newlib initialisation (bss, data, hardware, init_array) before
 * calling main.
 */

/* ========== Default Handlers (weak, infinite loop) ========== */
    .macro  def_default_handler handler_name
    .align 1
    .thumb_func
    .weak   \handler_name
    .type   \handler_name, %function
\handler_name:
    b       .
    .size   \handler_name, . - \handler_name
    .endm

/* System Exception Handlers */
    def_default_handler NMI_Handler
    def_default_handler HardFault_Handler
    def_default_handler SVC_Handler
    def_default_handler PendSV_Handler
    def_default_handler SysTick_Handler

/* IRQ Handlers */
    def_default_handler SSP0_Handler
    def_default_handler SSP1_Handler
    def_default_handler UART0_Handler
    def_default_handler UART1_Handler
    def_default_handler UART2_Handler
    def_default_handler I2C0_Handler
    def_default_handler I2C1_Handler
    def_default_handler PORT0_Handler
    def_default_handler PORT1_Handler
    def_default_handler PORT2_Handler
    def_default_handler PORT3_Handler
    def_default_handler DMA_Handler
    def_default_handler DUALTIMER0_Handler
    def_default_handler DUALTIMER1_Handler
    def_default_handler PWM0_Handler
    def_default_handler PWM1_Handler
    def_default_handler PWM2_Handler
    def_default_handler PWM3_Handler
    def_default_handler PWM4_Handler
    def_default_handler PWM5_Handler
    def_default_handler PWM6_Handler
    def_default_handler PWM7_Handler
    def_default_handler RTC_Handler
    def_default_handler ADC_Handler
    def_default_handler WZTOE_Handler
    def_default_handler EXTI_Handler

    .end
