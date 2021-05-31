#pragma once

#include <assert.h>
#include <stdint.h>
#include <nrf.h>

#define LOOP_TIMER NRF_TIMER2
#define LOOP_IRQn TIMER2_IRQn

#ifdef DELAY_IRQn
static_assert(LOOP_IRQn != DELAY_IRQn);
#endif

static inline void loop_set_period(const uint32_t p)
{
	LOOP_TIMER->CC[0] = p;
}

static void loop_init(const uint32_t p)
{
	// 32-bit timer
	LOOP_TIMER->BITMODE = TIMER_BITMODE_BITMODE_32Bit << TIMER_BITMODE_BITMODE_Pos;
	// 1/16us timer period
	LOOP_TIMER->PRESCALER = 0 << TIMER_PRESCALER_PRESCALER_Pos;
	// clear the timer when COMPARE0 event is triggered
	LOOP_TIMER->SHORTS = (TIMER_SHORTS_COMPARE0_CLEAR_Enabled << TIMER_SHORTS_COMPARE0_CLEAR_Pos);
	// enable IRQ on EVENTS_COMPARE[0]
	LOOP_TIMER->INTENSET = TIMER_INTENSET_COMPARE0_Enabled << TIMER_INTENSET_COMPARE0_Pos;

	NVIC_EnableIRQ(LOOP_IRQn);

	loop_set_period(p);
	LOOP_TIMER->TASKS_START = 1;
}

static inline void loop_wait(void)
{
	LOOP_TIMER->EVENTS_COMPARE[0] = 0;
	do {
		__WFI();
	} while (LOOP_TIMER->EVENTS_COMPARE[0] == 0);
	LOOP_TIMER->EVENTS_COMPARE[0] = 0;
	NVIC_ClearPendingIRQ(LOOP_IRQn);
}
