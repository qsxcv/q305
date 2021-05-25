#pragma once

#include <stdint.h>
#include <nrf.h>
#include "pins.h"

// maybe should use inline functions... whatever
#define LED_ENABLE() NRF_P0->OUTSET = (1 << LED_POWER)
#define LED_DISABLE() NRF_P0->OUTCLR = (1 << LED_POWER)
#define LED_ON(L) NRF_P0->OUTSET = (1 << (L))
#define LED_OFF(L) NRF_P0->OUTCLR = (1 << (L))
#define LED_TOGGLE(L) NRF_P0->OUT ^= (1 << (L))

static void led_init(void)
{
	const uint32_t out =
		(GPIO_PIN_CNF_DIR_Output << GPIO_PIN_CNF_DIR_Pos) |
		(GPIO_PIN_CNF_INPUT_Disconnect << GPIO_PIN_CNF_INPUT_Pos);
	LED_OFF(LED_POWER);
	LED_OFF(LED_R);
	LED_OFF(LED_G);
	LED_OFF(LED_B);
	NRF_P0->PIN_CNF[LED_POWER] = out;
	NRF_P0->PIN_CNF[LED_R] = out;
	NRF_P0->PIN_CNF[LED_G] = out;
	NRF_P0->PIN_CNF[LED_B] = out;
}
