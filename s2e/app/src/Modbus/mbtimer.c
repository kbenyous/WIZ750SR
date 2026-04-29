#include <stdint.h>
#include <stdbool.h>

#include "mbtimer.h"
#include "common.h"
#include "W7500x_board.h"
#include "W7500x_dualtimer.h"

volatile eMBRcvState eRcvState;

volatile uint8_t mb_state_rtu_finish;

static volatile uint32_t mb_timeout_us = 0;  

static uint32_t us_to_timer_load(uint32_t usec)
{
    uint64_t ticks = ((uint64_t)GetSystemClock() * (uint64_t)usec) / 1000000ULL;
    if (ticks == 0) ticks = 1; // ÃÖ¼Ò 1Æ½
    if (ticks > 0xFFFFFFFFULL) ticks = 0xFFFFFFFFULL;
    return (uint32_t)ticks;
}


void xMBPortTimersInit(uint32_t usTim1Timerout50us) {
    /* Calculate mb_timeout in ��s: T3.5 + 50ms response timeout */
    uint32_t mb_timeout;
    uint32_t t35_time_us = usTim1Timerout50us * 50;
		uint32_t sum = 0;
    DUALTIMER_InitTypDef Dualtimer_InitStructure;

		if (usTim1Timerout50us > (0xFFFFFFFFUL / 50UL)) t35_time_us = 0xFFFFFFFFUL;
    else t35_time_us = usTim1Timerout50us * 50UL;
	
		sum = t35_time_us + 50000UL;
    if (sum < t35_time_us) sum = 0xFFFFFFFFUL; 
	
		mb_timeout_us = sum;
		printf("mb_timeout = %u\n", mb_timeout_us);
	
		NVIC_EnableIRQ(DUALTIMER1_IRQn);
	
		DUALTIMER_ClockEnable(DUALTIMER1_0);
	
		printf("Timer Loader val : %u\n", us_to_timer_load(mb_timeout_us));
		Dualtimer_InitStructure.TimerLoad = us_to_timer_load(mb_timeout_us);
    Dualtimer_InitStructure.TimerControl_Mode = DUALTIMER_TimerControl_OneShot;
    Dualtimer_InitStructure.TimerControl_OneShot = DUALTIMER_TimerControl_OneShot;
    Dualtimer_InitStructure.TimerControl_Pre = DUALTIMER_TimerControl_Pre_1;
    Dualtimer_InitStructure.TimerControl_Size = DUALTIMER_TimerControl_Size_32;
		
		DUALTIMER_Init(DUALTIMER1_0, &Dualtimer_InitStructure);

    /* Dualtimer 1_0 Interrupt enable */
    DUALTIMER_IntConfig(DUALTIMER1_0, ENABLE);

    /* Dualtimer 1_0 start */
    //DUALTIMER_Start(DUALTIMER1_0);
}

void vMBPortTimersEnable(void) {
    // Instead of starting a timer, we'll use manual delay-based timing
    // The actual delay will happen in xMBRTUTimerT35Expired when called manually
    
    // Start the timer for compatibility
		DUALTIMER_Stop(DUALTIMER1_0);
		DUALTIMER_SetTimerLoad(DUALTIMER1_0, us_to_timer_load(mb_timeout_us));
    DUALTIMER_Start(DUALTIMER1_0);
}

void vMBPortTimersDisable(void) {
    DUALTIMER_Stop(DUALTIMER1_0);
}

void xMBRTUTimerT35Expired(void) {
    if(DUALTIMER_GetIntStatus(DUALTIMER1_0))
	{
		DUALTIMER_IntClear(DUALTIMER1_0);
        //printf("tim1 expired eRcvState = %d\r\n", eRcvState);
        switch (eRcvState) {
        /* Timer t35 expired. Startup phase is finished. */
        case STATE_RX_INIT:
            //printf("STATE_RX_INIT: Startup phase finished\r\n");
            break;

        /*  A frame was received and t35 expired. Notify the listener that
            a new frame was received. */
        case STATE_RX_RCV:
            //printf("STATE_RX_RCV: Frame received, setting mb_state_rtu_finish=true\r\n");
            mb_state_rtu_finish = true;
            break;

        /* An error occured while receiving the frame. */
        case STATE_RX_ERROR:
            //printf("STATE_RX_ERROR: Frame reception error\r\n");
            break;

        /* Function called in an illegal state. */
        default:
            //printf("WARNING: Timer expired in unexpected state: %d\r\n", eRcvState);
            break;
        }
        vMBPortTimersDisable();
        eRcvState = STATE_RX_IDLE;
    }
	if(DUALTIMER_GetIntStatus(DUALTIMER1_1))
	{
		DUALTIMER_IntClear(DUALTIMER1_1);
	}
}
