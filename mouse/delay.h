#pragma once

#include <assert.h>
#include <stdint.h>
#include <nrf.h>

#define DELAY_TIMER NRF_TIMER1
#define DELAY_IRQn TIMER1_IRQn

#ifdef LOOP_IRQn
static_assert(LOOP_IRQn != DELAY_IRQn);
#endif

static void delay_init()
{
	// 32-bit timer
	DELAY_TIMER->BITMODE = TIMER_BITMODE_BITMODE_32Bit << TIMER_BITMODE_BITMODE_Pos;
	// 1/16 us timer period
	DELAY_TIMER->PRESCALER = 0 << TIMER_PRESCALER_PRESCALER_Pos;
	// clear and stop the timer when COMPARE0 event is triggered
	DELAY_TIMER->SHORTS =
		(TIMER_SHORTS_COMPARE0_CLEAR_Enabled << TIMER_SHORTS_COMPARE0_CLEAR_Pos) |
		(TIMER_SHORTS_COMPARE0_STOP_Enabled << TIMER_SHORTS_COMPARE0_STOP_Pos);
	// enable IRQ on EVENTS_COMPARE[0]
	DELAY_TIMER->INTENSET = TIMER_INTENSET_COMPARE0_Enabled << TIMER_INTENSET_COMPARE0_Pos;

	//NVIC_SetPriority(DELAY_IRQn, 253); // mask interrupt if 0 < basepri <= 255 (set in main)
	NVIC_EnableIRQ(DELAY_IRQn);
}

// actual delay will be about ticks/16 us + 0.8us
static inline void delay_ticks(const uint32_t ticks)
{
	DELAY_TIMER->CC[0] = ticks;
	DELAY_TIMER->TASKS_START = 1;
	do {
		__WFI();
	} while (DELAY_TIMER->EVENTS_COMPARE[0] == 0);
	DELAY_TIMER->EVENTS_COMPARE[0] = 0;
	NVIC_ClearPendingIRQ(DELAY_IRQn);
}

static inline void delay_us(const uint32_t us)
{
	delay_ticks(16*us - 10);
}
