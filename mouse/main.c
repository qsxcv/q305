#include <assert.h>
#include <stdint.h>
#include <nrf.h>

#define SET_OUTPUT(p) NRF_P0->PIN_CNF[p] = (GPIO_PIN_CNF_DIR_Output << GPIO_PIN_CNF_DIR_Pos)
#define HIGH(p) NRF_P0->OUTSET = (1 << (p))
#define LOW(p) NRF_P0->OUTCLR = (1 << (p))

#include "pins.h"
#include "delay.h"
#include "loop.h"
#include "led.h"
#include "buttons.h"
#include "wheel.h"
#include "spi.h"
#include "hero.h"
#include "radio.h"

static void init(void)
{
	NRF_CLOCK->EVENTS_HFCLKSTARTED = 0;
	NRF_CLOCK->TASKS_HFCLKSTART = 1;
	while (NRF_CLOCK->EVENTS_HFCLKSTARTED == 0);

	NRF_POWER->TASKS_LOWPWR = 1; // can't tell difference from CONSTLAT
	NRF_POWER->DCDCEN = 1;

	delay_init();
	led_init();
	buttons_init();
	wheel_init();
	spi_init();
	if (hero_init() != 0) { // blob upload failed
		LED_ENABLE();
		LED_ON(LED_B);
		while (1) __WFI();
	}
	radio_init();
	loop_init();
}

int main(void)
{
#ifdef VECT_TAB
	SCB->VTOR = VECT_TAB;
#endif
	__disable_irq();

	init();
SET_OUTPUT(TP6);

	// tell receiver about initial coordinates and set loop phase right
	for (;;) {
		loop_wait();
HIGH(TP6);
		(void)hero_motion_burst(1);
		(void)wheel_read();
		(void)buttons_read_debounced();
		radio_mouse_data.x = 0;
		radio_mouse_data.y = 0;
		radio_mouse_data.whl = 0;
		radio_mouse_data.btn = RADIO_MOUSE_FIRST | RADIO_MOUSE_SYNC;

		NRF_RADIO->TASKS_TXEN = 1;
		radio_wait_disabled();
		radio_mouse_data_prev = radio_mouse_data;

		radio_mode_time();
		NRF_RADIO->TASKS_RXEN = 1;
		NRF_RADIO->EVENTS_ADDRESS = 0;
		delay_us(80);
		if (NRF_RADIO->EVENTS_ADDRESS == 0) { // no address match, try again
			NRF_RADIO->TASKS_STOP = 1;
			NRF_RADIO->TASKS_DISABLE = 1;
			radio_mode_mouse();
		} else { // address match
			radio_wait_disabled();
			radio_mode_mouse();
			if (NRF_RADIO->CRCSTATUS == RADIO_CRCSTATUS_CRCSTATUS_CRCOk) {
				loop_tune_phase(radio_time_delta);
				break; // else try again
			}
		}
	}

	uint32_t sync_timeout = 0; // synchronize after 1s inactivity
	uint32_t idle_timeout = 0; // TODO implement power saving
	for (;;) {
		loop_wait();
		union motion_data motion = hero_motion_burst(0);
		radio_mouse_data.x += motion.dx;
		radio_mouse_data.y += motion.dy;
		radio_mouse_data.whl += wheel_read();
HIGH(TP6);
		radio_mouse_data.btn = buttons_read_debounced() & 0b00011111;
LOW (TP6);
		if (sync_timeout == 8000) { // sync
			radio_mouse_data.btn |= RADIO_MOUSE_SYNC;
		}

		if (radio_mouse_data.x_y != radio_mouse_data_prev.x_y ||
			radio_mouse_data.btn_whl != radio_mouse_data_prev.btn_whl) {
			NRF_RADIO->TASKS_TXEN = 1;
			radio_wait_disabled();
			radio_mouse_data_prev = radio_mouse_data;
			sync_timeout = 0;
			idle_timeout = 0;
		} else {
			sync_timeout++;
		}

		if ((radio_mouse_data.btn & RADIO_MOUSE_SYNC) != 0) {
			sync_timeout = 0;
			idle_timeout++;

			radio_mode_time();
			NRF_RADIO->TASKS_RXEN = 1;
			NRF_RADIO->EVENTS_ADDRESS = 0;
			delay_us(80);
			if (NRF_RADIO->EVENTS_ADDRESS == 0) { // no address match, give up
				NRF_RADIO->TASKS_STOP = 1;
				NRF_RADIO->TASKS_DISABLE = 1;
				radio_wait_disabled();
			} else { // address match
				radio_wait_disabled();
				if (NRF_RADIO->CRCSTATUS == RADIO_CRCSTATUS_CRCSTATUS_CRCOk) {
					int error = radio_time_delta;
					if (error > 4 || error < -4) {
						loop_tune_phase(error);
						loop_tune_period(error);
					}
				}
			}
			radio_mode_mouse();
		}
	}
}
