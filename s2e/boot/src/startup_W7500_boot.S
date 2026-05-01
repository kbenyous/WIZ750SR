/* File: startup_W7500_boot.S
 * Purpose: Startup file for W7500x Boot project (GCC)
 *
 * This is the Boot variant: the vector table at 0x0000 references
 * Remap_* trampolines (placed at fixed addresses in isr_remap.c).
 * The Remap_* functions then call the actual handlers, enabling
 * the Boot->App jump mechanism on Cortex-M0 (no VTOR).
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

/* ========== Vector Table (Boot: uses Remap_* symbols) ========== */

    .section .isr_vector
    .align 2
    .globl __isr_vector
__isr_vector:
    .long   __stack                     /* Top of Stack (from linker)    */
    .long   Reset_Handler               /* Reset Handler                 */
    .long   Remap_NMI_Handler           /* NMI Handler         -> remap  */
    .long   Remap_HardFault_Handler     /* Hard Fault Handler  -> remap  */
    .long   0                           /* Reserved                      */
    .long   0                           /* Reserved                      */
    .long   0                           /* Reserved                      */
    .long   0                           /* Reserved                      */
    .long   0                           /* Reserved                      */
    .long   0                           /* Reserved                      */
    .long   0                           /* Reserved                      */
    .long   Remap_SVC_Handler           /* SVCall Handler      -> remap  */
    .long   0                           /* Reserved                      */
    .long   0                           /* Reserved                      */
    .long   Remap_PendSV_Handler        /* PendSV Handler      -> remap  */
    .long   Remap_SysTick_Handler       /* SysTick Handler     -> remap  */

    /* External Interrupts — all go through Remap trampolines */
    .long   Remap_SSP0_Handler          /* 16+ 0: SSP 0                  */
    .long   Remap_SSP1_Handler          /* 16+ 1: SSP 1                  */
    .long   Remap_UART0_Handler         /* 16+ 2: UART 0                 */
    .long   Remap_UART1_Handler         /* 16+ 3: UART 1                 */
    .long   Remap_UART2_Handler         /* 16+ 4: UART 2                 */
    .long   Remap_I2C0_Handler          /* 16+ 5: I2C 0                  */
    .long   Remap_I2C1_Handler          /* 16+ 6: I2C 1                  */
    .long   Remap_PORT0_Handler         /* 16+ 7: GPIO Port 0            */
    .long   Remap_PORT1_Handler         /* 16+ 8: GPIO Port 1            */
    .long   Remap_PORT2_Handler         /* 16+ 9: GPIO Port 2            */
    .long   Remap_PORT3_Handler         /* 16+10: GPIO Port 3            */
    .long   Remap_DMA_Handler           /* 16+11: DMA                    */
    .long   Remap_DUALTIMER0_Handler    /* 16+12: Dual timer 0           */
    .long   Remap_DUALTIMER1_Handler    /* 16+13: Dual timer 1           */
    .long   Remap_PWM0_Handler          /* 16+14: PWM0                   */
    .long   Remap_PWM1_Handler          /* 16+15: PWM1                   */
    .long   Remap_PWM2_Handler          /* 16+16: PWM2                   */
    .long   Remap_PWM3_Handler          /* 16+17: PWM3                   */
    .long   Remap_PWM4_Handler          /* 16+18: PWM4                   */
    .long   Remap_PWM5_Handler          /* 16+19: PWM5                   */
    .long   Remap_PWM6_Handler          /* 16+20: PWM6                   */
    .long   Remap_PWM7_Handler          /* 16+21: PWM7                   */
    .long   Remap_RTC_Handler           /* 16+22: RTC                    */
    .long   Remap_ADC_Handler           /* 16+23: ADC                    */
    .long   Remap_WZTOE_Handler         /* 16+24: WZTOE                  */
    .long   Remap_EXTI_Handler          /* 16+25: EXTI                   */

    .size   __isr_vector, . - __isr_vector

/* Reset_Handler is defined in libs/cortexm/src/exception_handlers.c
 * and delegates to _start (libs/newlib/src/_startup.c), which performs
 * the full newlib initialisation (bss, data, hardware, init_array) before
 * calling main.
 */

/* ========== Default Handlers (weak, infinite loop) ========== */
/*
 * These are fallback definitions for Remap_* symbols.
 * They are overridden by the real implementations in isr_remap.c.
 * Having them here as [weak] ensures the linker never has unresolved symbols.
 */
    .macro  def_remap_handler handler_name
    .align 1
    .thumb_func
    .weak   \handler_name
    .type   \handler_name, %function
\handler_name:
    b       .
    .size   \handler_name, . - \handler_name
    .endm

    def_remap_handler   Remap_NMI_Handler
    def_remap_handler   Remap_HardFault_Handler
    def_remap_handler   Remap_SVC_Handler
    def_remap_handler   Remap_PendSV_Handler
    def_remap_handler   Remap_SysTick_Handler
    def_remap_handler   Remap_SSP0_Handler
    def_remap_handler   Remap_SSP1_Handler
    def_remap_handler   Remap_UART0_Handler
    def_remap_handler   Remap_UART1_Handler
    def_remap_handler   Remap_UART2_Handler
    def_remap_handler   Remap_I2C0_Handler
    def_remap_handler   Remap_I2C1_Handler
    def_remap_handler   Remap_PORT0_Handler
    def_remap_handler   Remap_PORT1_Handler
    def_remap_handler   Remap_PORT2_Handler
    def_remap_handler   Remap_PORT3_Handler
    def_remap_handler   Remap_DMA_Handler
    def_remap_handler   Remap_DUALTIMER0_Handler
    def_remap_handler   Remap_DUALTIMER1_Handler
    def_remap_handler   Remap_PWM0_Handler
    def_remap_handler   Remap_PWM1_Handler
    def_remap_handler   Remap_PWM2_Handler
    def_remap_handler   Remap_PWM3_Handler
    def_remap_handler   Remap_PWM4_Handler
    def_remap_handler   Remap_PWM5_Handler
    def_remap_handler   Remap_PWM6_Handler
    def_remap_handler   Remap_PWM7_Handler
    def_remap_handler   Remap_RTC_Handler
    def_remap_handler   Remap_ADC_Handler
    def_remap_handler   Remap_WZTOE_Handler
    def_remap_handler   Remap_EXTI_Handler

    .end
