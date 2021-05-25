#pragma once

#include <stdint.h>
#include <nrf.h>

static void delay_init()
{
	// 32-bit timer
	NRF_TIMER0->BITMODE = TIMER_BITMODE_BITMODE_32Bit << TIMER_BITMODE_BITMODE_Pos;
	// 1us timer period
	NRF_TIMER0->PRESCALER = 4 << TIMER_PRESCALER_PRESCALER_Pos;
	// Enable IRQ on EVENTS_COMPARE[0]
	NRF_TIMER0->INTENSET = TIMER_INTENSET_COMPARE0_Enabled << TIMER_INTENSET_COMPARE0_Pos;
	// Clear the timer when COMPARE0 event is triggered
	NRF_TIMER0->SHORTS =
		(TIMER_SHORTS_COMPARE0_CLEAR_Enabled << TIMER_SHORTS_COMPARE0_CLEAR_Pos) |
		(TIMER_SHORTS_COMPARE0_STOP_Enabled << TIMER_SHORTS_COMPARE0_STOP_Pos);

	__disable_irq();
	NVIC_EnableIRQ(TIMER0_IRQn);
}

static inline void delay_us(const uint32_t us)
{
	NRF_TIMER0->CC[0] = us;
	NRF_TIMER0->TASKS_START = 1;
	__WFI();
	NRF_TIMER0->EVENTS_COMPARE[0] = 0;
	NVIC_ClearPendingIRQ(TIMER0_IRQn);
}
