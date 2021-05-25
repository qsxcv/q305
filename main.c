#include <assert.h>
#include <stdint.h>
#include <nrf.h>

#include "delay.h"
#include "led.h"
#include "spi.h"
#include "hero.h"

int main(void)
{
#ifdef VECT_TAB
	SCB->VTOR = VECT_TAB;
#endif

	NRF_CLOCK->EVENTS_HFCLKSTARTED = 0;
	NRF_CLOCK->TASKS_HFCLKSTART = 1;
	while (NRF_CLOCK->EVENTS_HFCLKSTARTED == 0);

	NRF_POWER->TASKS_CONSTLAT = 1;
	NRF_POWER->DCDCEN = 1; // increases current consumption when HFXO + sleep???

	led_init();

	LED_ENABLE();

	delay_init();
	spi_init();
	if (hero_init() != 0) { // blob upload failed
		LED_ON(LED_B);
		while (1) __WFI();
	}



	while (1) {
		volatile uint8_t *b = hero_motion_burst(); //2 yhi 3ylo 4xhi 5xlo
		int16_t x = (b[4] << 8) | b[5];

		 if (x > 0) {
			LED_OFF(LED_R);
			LED_ON(LED_G);
		} else if (x < 0) {
			LED_OFF(LED_G);
			LED_ON(LED_R);
		} else {
			LED_OFF(LED_R);
			LED_OFF(LED_G);
		}

		delay_us(1000);
	}
}
