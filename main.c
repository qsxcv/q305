#include <assert.h>
#include <stdint.h>
#include <nrf.h>

#include "delay.h"
#include "led.h"
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
	delay_init();
	spi_init();
	hero_init();

	while (1) {
		__WFI();
//		LED_ON(11);
		//delay_us(1000000);
//		LED_OFF(11);
//		delay_us(500000);
	}
}
