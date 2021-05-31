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
#include "spi.h"
#include "hero.h"
#include "radio.h"

static void clock_init(void)
{
	NRF_CLOCK->EVENTS_HFCLKSTARTED = 0;
	NRF_CLOCK->TASKS_HFCLKSTART = 1;
	while (NRF_CLOCK->EVENTS_HFCLKSTARTED == 0);

	// TODO lfclk
}

static void power_init(void)
{
	NRF_POWER->TASKS_LOWPWR = 1; // can't tell difference from CONSTLAT
	NRF_POWER->DCDCEN = 1; //
}

int main(void)
{
#ifdef VECT_TAB
	SCB->VTOR = VECT_TAB;
#endif

	__disable_irq();
SET_OUTPUT(TP6);
	clock_init();
	power_init();
	delay_init();
	led_init();
	spi_init();
	if (hero_init() != 0) { // blob upload failed
		LED_ENABLE();
		LED_ON(LED_B);
		while (1) __WFI();
	}

	radio_init();

	// time in units of 1/16 us ticks
	const int period = 16 * 125; // 125us main loop period
	const int max_backshift = 16 * 10; // maximum amount of backwards shift
	int tune = 0; // positive means clock too slow
	const int tune_denom = 1000000;
	// tune/tune_denom == 1 corresponds to 1/period of clock inaccuracy
	int offset = 0; // accumulated clock error


	loop_init(period);
	// first transmission and sync
	for (;;) {
HIGH(TP6);
		loop_wait();
		radio_packet.mouse.btn = RADIO_MOUSE_FIRST | RADIO_MOUSE_SYNC;
		radio_packet.mouse.whl = 0;
		radio_packet.mouse.x = 1232;
		radio_packet.mouse.y = 0;

		NRF_RADIO->TASKS_TXEN = 1;
		radio_wait_disabled();

		radio_mode_time();
		NRF_RADIO->TASKS_RXEN = 1;
		radio_wait_disabled();
		radio_mode_mouse();

		if (NRF_RADIO->CRCSTATUS == RADIO_CRCSTATUS_CRCSTATUS_CRCOk) {
			int new_per = period - radio_packet.time.delta;
			if (new_per < period - max_backshift)
				new_per += period;
			LOOP_TIMER->CC[0] = new_per;
			break; // otherwise try again
		}
	}

	int loops = 0;
	for (uint32_t i = 0; ; i++) {
		loop_wait();
HIGH(TP6);
		loops++;

		offset += tune;
		if (offset >= tune_denom) {
			offset -= tune_denom;
			loop_set_period(period - 1); // speed up by one tick
		} else if (offset <= -tune_denom) {
			offset += tune_denom;
			loop_set_period(period + 1); // slow down by one tick
		} else {
			loop_set_period(period);
		}

		// TODO read buttons, wheel, sensor
		//int16_t dx, dy;
		//hero_motion_burst(&dx, &dy);
		//radio_packet.mouse.x += dx;
		//radio_packet.mouse.y += dy;

		radio_packet.mouse.btn = 0;
		radio_packet.mouse.whl = 0;
		radio_packet.mouse.x = 1232;
		radio_packet.mouse.y = 0;

		if (i == 8000) { // sync
			radio_packet.mouse.btn |= RADIO_MOUSE_SYNC;
		}
		// if anything is different from previous
		{
			NRF_RADIO->TASKS_TXEN = 1;
			radio_wait_disabled();
		}
		if (i == 8000) {
			i = 0;
			// since the prev. transmission and this can take longer than 125us
			// in total, LOOP_IRQn will be pending and cause the __WFI() loop in
			// radio_wait_disabled() here and the next loop_wait() to busy loop.
			// so temporarily disable loop interrupt
			LOOP_TIMER->INTENCLR = TIMER_INTENSET_COMPARE0_Enabled << TIMER_INTENSET_COMPARE0_Pos;

			radio_mode_time();
			NRF_RADIO->TASKS_RXEN = 1;
			radio_wait_disabled(); // TODO exit after maximum delay
			radio_mode_mouse();

			LOOP_TIMER->EVENTS_COMPARE[0] = 0;
			LOOP_TIMER->INTENSET = TIMER_INTENSET_COMPARE0_Enabled << TIMER_INTENSET_COMPARE0_Pos;

			if (NRF_RADIO->CRCSTATUS == RADIO_CRCSTATUS_CRCSTATUS_CRCOk) {
				int error = radio_packet.time.delta;
				if (error > 8 || error < -8) {
					tune += error*(tune_denom/loops)/2;
					loops = 0;

					int new_per = period - error;
					if (new_per < period - max_backshift)
						new_per += period;
					loop_set_period(new_per);
				}
			}
		}
LOW(TP6);
	}
}

/*
	volatile uint8_t hero_regs[0x80];
	volatile uint8_t r = 0;
	uint8_t prev = 0;
	LED_ENABLE();

	//spi_cs_low();
	//hero_reg_write(0x20, 6);
	//hero_reg_write(0x34, 0x0d);
	//spi_cs_high();
	while (1) {
		//__WFI();
		//volatile uint8_t *b = hero_motion_burst(); //2 yhi 3ylo 4xhi 5xlo
		//int16_t x = (b[4] << 8) | b[5];

		if (r == 128) r = 0;

		spi_cs_low();
		prev = hero_reg_write_readprev(r, 0x00);
		spi_cs_high();

		spi_cs_low();
		for (uint8_t i = 0; i < 0x80; i++)
			hero_regs[i] = hero_reg_read(i);
		spi_cs_high();

		delay_us(1000);
		spi_cs_low();
		hero_reg_write(r, prev);
		spi_cs_high();

		r++;
	}
*/
