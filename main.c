#include <assert.h>
#include <stdint.h>
#include <nrf.h>

#define LED_POWER 3
#define LED_R 5
#define LED_G 6
#define LED_B 7
#define LED_ON(L) NRF_P0->OUTSET = (1 << (L))
#define LED_OFF(L) NRF_P0->OUTCLR = (1 << (L))
#define LED_TOGGLE(L) NRF_P0->OUT ^= (1 << (L))

static void led_init(void)
{
	const uint32_t out =
			(GPIO_PIN_CNF_DIR_Output << GPIO_PIN_CNF_DIR_Pos) |
			(GPIO_PIN_CNF_DRIVE_S0S1 << GPIO_PIN_CNF_DRIVE_Pos) |
			(GPIO_PIN_CNF_INPUT_Disconnect << GPIO_PIN_CNF_INPUT_Pos) |
			(GPIO_PIN_CNF_PULL_Disabled << GPIO_PIN_CNF_PULL_Pos) |
			(GPIO_PIN_CNF_SENSE_Disabled << GPIO_PIN_CNF_SENSE_Pos);
	NRF_P0->PIN_CNF[LED_POWER] = out;
	LED_ON(LED_POWER);
	LED_OFF(LED_R);
	LED_OFF(LED_G);
	LED_OFF(LED_B);
	NRF_P0->PIN_CNF[LED_R] = out;
	NRF_P0->PIN_CNF[LED_G] = out;
	NRF_P0->PIN_CNF[LED_B] = out;
}

//#define DELAY_IRQ

void delay_init()
{
	// 32-bit timer
	NRF_TIMER0->BITMODE = TIMER_BITMODE_BITMODE_32Bit << TIMER_BITMODE_BITMODE_Pos;
	// 1us timer period
	NRF_TIMER0->PRESCALER = 4 << TIMER_PRESCALER_PRESCALER_Pos;
	// Enable IRQ on EVENTS_COMPARE[0]
	NRF_TIMER0->INTENSET = TIMER_INTENSET_COMPARE0_Enabled << TIMER_INTENSET_COMPARE0_Pos;
	// Clear the timer when COMPARE0 event is triggered
	NRF_TIMER0->SHORTS =
		(TIMER_SHORTS_COMPARE0_CLEAR_Enabled << TIMER_SHORTS_COMPARE0_CLEAR_Pos) |
		(TIMER_SHORTS_COMPARE0_STOP_Enabled << TIMER_SHORTS_COMPARE0_STOP_Pos);
#ifndef DELAY_IRQ
	__disable_irq();
#endif
	NVIC_EnableIRQ(TIMER0_IRQn);
}

static inline void delay_us(const uint32_t us)
{
	NRF_TIMER0->CC[0] = us;
	NRF_TIMER0->TASKS_START = 1;
	__WFI();
#ifndef DELAY_IRQ
	NRF_TIMER0->EVENTS_COMPARE[0] = 0;
	NVIC_ClearPendingIRQ(TIMER0_IRQn);
#endif
}

#ifdef DELAY_IRQ
void TIMER0_IRQHandler(void)
{
	NRF_TIMER0->EVENTS_COMPARE[0] = 0;
	__DSB();
}
#endif

int main(void)
{
#ifdef VECT_TAB
	SCB->VTOR = VECT_TAB;
#endif	
	NRF_CLOCK->EVENTS_HFCLKSTARTED = 0;
	NRF_CLOCK->TASKS_HFCLKSTART = 1;
	while (NRF_CLOCK->EVENTS_HFCLKSTARTED == 0);

	NRF_POWER->TASKS_CONSTLAT = 1;

	led_init();
	delay_init();

	while (1) {
		LED_ON(LED_R);
		delay_us(500000);
		LED_OFF(LED_R);
		delay_us(500000);
	}
}
