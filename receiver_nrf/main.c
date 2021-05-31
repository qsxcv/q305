#include <stdint.h>
#include <nrf.h>
#include "led.h"
#include "radio.h"

#define SET_OUTPUT(p) NRF_P0->PIN_CNF[p] = (GPIO_PIN_CNF_DIR_Output << GPIO_PIN_CNF_DIR_Pos)
#define HIGH(p) NRF_P0->OUTSET = (1 << (p))
#define LOW(p) NRF_P0->OUTCLR = (1 << (p))
#define TOGGLE(p) NRF_P0->OUT ^= (1 << (p))

static void clock_init(void)
{
	NRF_CLOCK->EVENTS_HFCLKSTARTED = 0;
	NRF_CLOCK->TASKS_HFCLKSTART = 1;
	while (NRF_CLOCK->EVENTS_HFCLKSTARTED == 0);
}

static void power_init(void)
{
	NRF_POWER->TASKS_CONSTLAT = 1;
}

int main(void)
{
#ifdef VECT_TAB
	SCB->VTOR = VECT_TAB;
#endif

	__disable_irq();

	clock_init();
	power_init();
	led_init();
	SET_OUTPUT(3);
	SET_OUTPUT(4);

	// time in units of 1/16 us ticks
	const int period = 16 * 125; // us
	const int ideal_rx_time = 16 * 75;

	// simulate a 1khz usb/spi timing
	NRF_TIMER2->BITMODE = TIMER_BITMODE_BITMODE_32Bit << TIMER_BITMODE_BITMODE_Pos;
	NRF_TIMER2->PRESCALER = 0 << TIMER_PRESCALER_PRESCALER_Pos;
	NRF_TIMER2->SHORTS = (TIMER_SHORTS_COMPARE0_CLEAR_Enabled << TIMER_SHORTS_COMPARE0_CLEAR_Pos);
	NRF_TIMER2->CC[0] = period; // 125us period
	NRF_TIMER2->TASKS_START = 1;

	 // 10us pulse on pin 4
	NRF_TIMER2->CC[1] = 16*10;
	NRF_GPIOTE->CONFIG[0] =
		(GPIOTE_CONFIG_MODE_Task       << GPIOTE_CONFIG_MODE_Pos) |
		(GPIOTE_CONFIG_OUTINIT_Low     << GPIOTE_CONFIG_OUTINIT_Pos) |
		(GPIOTE_CONFIG_POLARITY_Toggle << GPIOTE_CONFIG_POLARITY_Pos) |
		(4                             << GPIOTE_CONFIG_PSEL_Pos);
	NRF_PPI->CH[1].EEP = (uint32_t)&NRF_TIMER2->EVENTS_COMPARE[0];
	NRF_PPI->CH[1].TEP = (uint32_t)&NRF_GPIOTE->TASKS_OUT[0];
	NRF_PPI->CHENSET = 1 << 1;
	NRF_PPI->CH[2].EEP = (uint32_t)&NRF_TIMER2->EVENTS_COMPARE[1];
	NRF_PPI->CH[2].TEP = (uint32_t)&NRF_GPIOTE->TASKS_OUT[0];
	NRF_PPI->CHENSET = 1 << 2;

	// radio timing
	NRF_TIMER0->BITMODE = TIMER_BITMODE_BITMODE_32Bit << TIMER_BITMODE_BITMODE_Pos;
	NRF_TIMER0->PRESCALER = 0 << TIMER_PRESCALER_PRESCALER_Pos;
	NRF_TIMER0->TASKS_START = 1;
	// reset radio timer to 0 on usb/spi poll
	NRF_PPI->CH[0].EEP = (uint32_t)&NRF_TIMER2->EVENTS_COMPARE[0]; // use spi here for actual receiver
	NRF_PPI->CH[0].TEP = (uint32_t)&NRF_TIMER0->TASKS_CLEAR;
	NRF_PPI->CHENSET = 1 << 0;
	// capture timing of received pkt, relative to usb/spi poll
	// pre-programmed ch27: RADIO->EVENTS_END => TIMER0->TASKS_CAPTURE[2]
	NRF_PPI->CHENSET = 1 << 27;

	radio_init();

	NRF_RADIO->TASKS_RXEN = 1;

	for (;;) {
		NRF_RADIO->TASKS_START = 1;
		NRF_RADIO->EVENTS_END = 0;
		while (NRF_RADIO->EVENTS_END == 0);

TOGGLE(3);
		if (NRF_RADIO->CRCSTATUS == RADIO_CRCSTATUS_CRCSTATUS_CRCError) {
			LED_TOGGLE(LED_4);
			continue;
		}

		// placeholder for copying pkt to spi
		if (radio_packet.mouse.x == 1232) {
			LED_ON(LED_1);
		} else {
			LED_OFF(LED_1);
			LED_TOGGLE(LED_2);
		}
TOGGLE(3);

		// if mouse requests sync
		if (radio_packet.mouse.btn & RADIO_MOUSE_SYNC) {
LED_TOGGLE(LED_3);
			radio_packet.time.delta = (int16_t)NRF_TIMER0->CC[2] - ideal_rx_time;

			// transmit time data
			NRF_RADIO->EVENTS_DISABLED = 0;
			NRF_RADIO->TASKS_DISABLE = 1;
			while (NRF_RADIO->EVENTS_DISABLED == 0);

			radio_mode_time();
			NRF_RADIO->TASKS_TXEN = 1;

			NRF_RADIO->EVENTS_END = 0;
			while (NRF_RADIO->EVENTS_END == 0);

			NRF_RADIO->EVENTS_DISABLED = 0;
			NRF_RADIO->TASKS_DISABLE = 1;
			while (NRF_RADIO->EVENTS_DISABLED == 0);

			// return to rx mode
			radio_mode_mouse();
			NRF_RADIO->TASKS_RXEN = 1;
		}
	}
}
