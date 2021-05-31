#pragma once

#include <stdint.h>
#include <nrf.h>

#define LED_1 13
#define LED_2 14
#define LED_3 15
#define LED_4 16
#define LED_ON(L) NRF_P0->OUTCLR = (1 << (L))
#define LED_OFF(L) NRF_P0->OUTSET = (1 << (L))
#define LED_TOGGLE(L) NRF_P0->OUT ^= (1 << (L))

static void led_init(void)
{
	const uint32_t out =
		(GPIO_PIN_CNF_DIR_Output << GPIO_PIN_CNF_DIR_Pos);
	LED_OFF(LED_1);
	LED_OFF(LED_2);
	LED_OFF(LED_3);
	LED_OFF(LED_4);
	NRF_P0->PIN_CNF[LED_1] = out;
	NRF_P0->PIN_CNF[LED_2] = out;
	NRF_P0->PIN_CNF[LED_3] = out;
	NRF_P0->PIN_CNF[LED_4] = out;
}
