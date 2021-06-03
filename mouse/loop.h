#pragma once

#include <assert.h>
#include <stdint.h>
#include <nrf.h>

#define LOOP_TIMER NRF_TIMER2
#define LOOP_IRQn TIMER2_IRQn

#ifdef DELAY_IRQn
static_assert(LOOP_IRQn != DELAY_IRQn);
#endif

const int loop_period = 16 * 125;  // 125us main loop period (nominal)
// tune/tune_denom == 1 corresponds to 1/period of clock inaccuracy
static int loop_tune = 0; // positive means clock too slow
const int loop_tune_denom = 1000000;
const int loop_max_backshift = 16 * 10; // 10us max amount of backwards shift
static int loop_count = 0; // calls to loop_wait after last loop_tune_phase

static inline void loop_set_period(const uint32_t p)
{
	// note: most of the time, loop_wait resets CC[0] to loop_period
	LOOP_TIMER->CC[0] = p;
}

static void loop_init(void)
{
	LOOP_TIMER->BITMODE = TIMER_BITMODE_BITMODE_32Bit << TIMER_BITMODE_BITMODE_Pos;
	// 1/16us timer period
	LOOP_TIMER->PRESCALER = 0 << TIMER_PRESCALER_PRESCALER_Pos;
	LOOP_TIMER->SHORTS = (TIMER_SHORTS_COMPARE0_CLEAR_Enabled << TIMER_SHORTS_COMPARE0_CLEAR_Pos);
	NVIC_EnableIRQ(LOOP_IRQn);
	loop_set_period(loop_period);
	LOOP_TIMER->TASKS_START = 1;
}

// returns on a periodic cycle, with
// average period = loop_period - (loop_tune/loop_tune_denom)
static inline void loop_wait(void)
{
	static int offset = 0; // accumulated clock error
	offset += loop_tune;

	int next_period = loop_period;
	if (offset >= loop_tune_denom) {
		offset -= loop_tune_denom;
		next_period--; // speed up by one tick
	} else if (offset <= -loop_tune_denom) {
		offset += loop_tune_denom;
		next_period++; // slow down by one tick
	}

	loop_count++;

	LOOP_TIMER->EVENTS_COMPARE[0] = 0;
	LOOP_TIMER->INTENSET = TIMER_INTENSET_COMPARE0_Enabled << TIMER_INTENSET_COMPARE0_Pos;
	do {
		__WFI();
	} while (LOOP_TIMER->EVENTS_COMPARE[0] == 0);
	//LOOP_TIMER->EVENTS_COMPARE[0] = 0;
	LOOP_TIMER->INTENCLR = TIMER_INTENSET_COMPARE0_Enabled << TIMER_INTENSET_COMPARE0_Pos;
	NVIC_ClearPendingIRQ(LOOP_IRQn);
	loop_set_period(next_period);
}

static inline void loop_tune_phase(int error)
{
	int new_per = loop_period - error;
	if (new_per < loop_period - loop_max_backshift)
		new_per += loop_period;
	loop_set_period(new_per);
}

static inline void loop_tune_period(int error)
{
	loop_tune += error*(loop_tune_denom/loop_count);
	loop_count = 0;
}
