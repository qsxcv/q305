#pragma once

#include <stdint.h>
#include <nrf.h>
#include "delay.h"
#include "pins.h"

static inline int wheel_read_A(void)
{
	return (NRF_P0->IN >> WHL_A) & 1;
}

static inline int wheel_read_B(void)
{
	return (NRF_P0->IN >> WHL_B) & 1;
}

static inline void wheel_pullup_on(void)
{
	NRF_P0->OUTSET = 1 << WHL_PULL;
}

static inline void wheel_pullup_off(void)
{
	NRF_P0->OUTCLR = 1 << WHL_PULL;
}

static inline int wheel_read(void)
{
	static int wheel_prev_same = 0; // what A was last time A == B
	static int wheel_prev_diff = 0; // what A was last time A != B

	int A = wheel_read_A();
	int B = wheel_read_B();

	if (A != B)
		wheel_prev_diff = A;
	else if (A != wheel_prev_same) {
		wheel_prev_same = A;
		return (A == wheel_prev_diff) ? -1 : 1;
	}
	return 0;
}

static void wheel_init(void)
{
	NRF_P0->PIN_CNF[WHL_PULL] =
		(GPIO_PIN_CNF_DIR_Output << GPIO_PIN_CNF_DIR_Pos) |
		(GPIO_PIN_CNF_INPUT_Disconnect << GPIO_PIN_CNF_INPUT_Pos);
	wheel_pullup_on();

	const int in =
		(GPIO_PIN_CNF_DIR_Input << GPIO_PIN_CNF_DIR_Pos) |
		(GPIO_PIN_CNF_INPUT_Connect << GPIO_PIN_CNF_INPUT_Pos);
	NRF_P0->PIN_CNF[WHL_A] = in;
	NRF_P0->PIN_CNF[WHL_B] = in;

	delay_us(1000); // pullup takes a bit of time
	(void)wheel_read(); // initialize static variables in wheel_read()
}
