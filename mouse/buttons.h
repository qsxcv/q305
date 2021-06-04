#pragma once

#include <stdint.h>
#include <nrf.h>
#include "delay.h"
#include "pins.h"

#define BUTTON_RELEASE_DEBOUNCE 16

static_assert(
	BTN_R == BTN_L + 1 &&
	BTN_M == BTN_L + 2 &&
	BTN_F == BTN_L + 3 &&
	BTN_B == BTN_L + 4 &&
	BTN_DPI == BTN_L + 5,
	"button pins not contiguous. change buttons_read_raw()"
);

static inline uint8_t buttons_read_raw(void)
{
	uint8_t ret = (NRF_P0->LATCH >> BTN_L) & 0b00111111;
	NRF_P0->LATCH = 0b00111111 << BTN_L;
	return ret;
}

static inline uint8_t buttons_read_debounced(void)
{
	const uint32_t release_lag = 8*BUTTON_RELEASE_DEBOUNCE;

	static uint32_t prev = 0; // previous debounced state
	static uint32_t time[6] = {0}; // cycles (125us) for which button has been unpressed

	uint32_t debounced = buttons_read_raw();
	const uint32_t released = prev & ~debounced;
	for (int i = 0; i < 6; i++) {
		if (released & (1 << i)) {
			time[i]++;
			if (time[i] < release_lag)
				debounced |= (1 << i);
			else
				time[i] = 0;
		}
	}
	prev = debounced;
	return (uint8_t)debounced;
}

static void buttons_init(void)
{
	const int pullup_in =
		(GPIO_PIN_CNF_DIR_Input << GPIO_PIN_CNF_DIR_Pos) |
		(GPIO_PIN_CNF_INPUT_Connect << GPIO_PIN_CNF_INPUT_Pos) |
		(GPIO_PIN_CNF_PULL_Pullup << GPIO_PIN_CNF_PULL_Pos) |
		(GPIO_PIN_CNF_SENSE_Low << GPIO_PIN_CNF_SENSE_Pos);
	NRF_P0->PIN_CNF[BTN_L] = pullup_in;
	NRF_P0->PIN_CNF[BTN_R] = pullup_in;
	NRF_P0->PIN_CNF[BTN_M] = pullup_in;
	NRF_P0->PIN_CNF[BTN_F] = pullup_in;
	NRF_P0->PIN_CNF[BTN_B] = pullup_in;
	NRF_P0->PIN_CNF[BTN_DPI] = pullup_in;

	delay_us(1000);
	NRF_P0->LATCH = 0b00111111 << BTN_L;
}
