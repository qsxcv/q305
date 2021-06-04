#include <stdint.h>
#include <nrf.h>
#include "led.h"
#include "radio.h"
#include "spi.h"

#define SET_OUTPUT(p) NRF_P0->PIN_CNF[p] = (GPIO_PIN_CNF_DIR_Output << GPIO_PIN_CNF_DIR_Pos)
#define HIGH(p) NRF_P0->OUTSET = (1 << (p))
#define LOW(p) NRF_P0->OUTCLR = (1 << (p))
#define TOGGLE(p) NRF_P0->OUT ^= (1 << (p))

static void init(void)
{
	NRF_CLOCK->EVENTS_HFCLKSTARTED = 0;
	NRF_CLOCK->TASKS_HFCLKSTART = 1;
	while (NRF_CLOCK->EVENTS_HFCLKSTARTED == 0);

	NRF_POWER->TASKS_CONSTLAT = 1;
	led_init();
	spi_init();
	radio_init();
}

// from system_nrf52.c
void nvmc_wait(void);
void nvmc_config(uint32_t mode);

int main(void)
{
#ifdef VECT_TAB
	SCB->VTOR = VECT_TAB;
#endif

	// 3.3V for SPI with disco board
	if ((NRF_UICR->REGOUT0 & UICR_REGOUT0_VOUT_Msk) !=
		(UICR_REGOUT0_VOUT_3V3 << UICR_REGOUT0_VOUT_Pos)) {
		nvmc_config(NVMC_CONFIG_WEN_Wen);
		NRF_UICR->REGOUT0 = (UICR_REGOUT0_VOUT_3V3 << UICR_REGOUT0_VOUT_Pos);
		nvmc_wait();
		nvmc_config(NVMC_CONFIG_WEN_Ren);
		NVIC_SystemReset();
	}

	__disable_irq();

	init();

	SET_OUTPUT(3);
	SET_OUTPUT(4);

	// time in units of 1/16 us ticks
	const int nominal_period = 16 * 125;
	const int ideal_rx_time = nominal_period - (16*5);

	// event when CS goes low
	NRF_GPIOTE->CONFIG[0] =
		(GPIOTE_CONFIG_MODE_Event      << GPIOTE_CONFIG_MODE_Pos) |
		(GPIOTE_CONFIG_POLARITY_HiToLo << GPIOTE_CONFIG_POLARITY_Pos) |
		(0                             << GPIOTE_CONFIG_PORT_Pos) |
		(SPI_CS                        << GPIOTE_CONFIG_PSEL_Pos);

	// radio timing
	NRF_TIMER0->BITMODE = TIMER_BITMODE_BITMODE_32Bit << TIMER_BITMODE_BITMODE_Pos;
	NRF_TIMER0->PRESCALER = 0 << TIMER_PRESCALER_PRESCALER_Pos;
	NRF_TIMER0->TASKS_START = 1;

	// reset radio timer to 0 on usb/spi poll
	NRF_PPI->CH[0].EEP = (uint32_t)&NRF_GPIOTE->EVENTS_IN[0];
	NRF_PPI->CH[0].TEP = (uint32_t)&NRF_TIMER0->TASKS_CLEAR;
	NRF_PPI->CHENSET = 1 << 0;

	// capture timing of received pkt, relative to usb/spi poll
	// pre-programmed ch27: RADIO->EVENTS_END => TIMER0->TASKS_CAPTURE[2]
	NRF_PPI->CHENSET = 1 << 27;

	NRF_SPIS0->TASKS_RELEASE = 1;

	NRF_RADIO->TASKS_RXEN = 1;
	for (;;) {
LOW(4);
		NRF_RADIO->TASKS_START = 1;
		NRF_RADIO->EVENTS_END = 0;
		while (NRF_RADIO->EVENTS_END == 0);
HIGH(4);
		if (NRF_RADIO->CRCSTATUS == RADIO_CRCSTATUS_CRCSTATUS_CRCError) {
			radio_mouse_data.btn |= RADIO_MOUSE_IGNORE;
LED_TOGGLE(LED_4);
			continue;
		}

		// spi easydma reads from radio_mouse_data

		// if mouse requests sync
		if (radio_mouse_data.btn & RADIO_MOUSE_SYNC) {
LED_TOGGLE(LED_3);
			int diff = NRF_TIMER0->CC[2] - ideal_rx_time;
			if (diff > nominal_period/2) // probably spi isn't active
				diff = 0;
			else if (diff <= -nominal_period/2) // probably slightly too late rather than early
				diff += nominal_period;

			radio_time_delta = (int16_t)diff;

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
