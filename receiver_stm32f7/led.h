#pragma once

#include "stm32f7xx.h"

#define LED_R_TOGGLE() GPIOA->ODR ^= GPIO_ODR_OD7
#define LED_G_TOGGLE() GPIOB->ODR ^= GPIO_ODR_OD1
#define LED_B_TOGGLE() GPIOA->ODR ^= GPIO_ODR_OD5

void led_init(void)
{
	// LED5 (red) on PA7, LED6 (green) on PB1
	SET_BIT(RCC->AHB1ENR, RCC_AHB1ENR_GPIOAEN | RCC_AHB1ENR_GPIOBEN);
	__NOP(); __NOP();
	MODIFY_REG(GPIOA->MODER,
			GPIO_MODER_MODER7_Msk,
			GPIO_MODER_MODER7_0);
	MODIFY_REG(GPIOB->MODER,
			GPIO_MODER_MODER1_Msk,
			GPIO_MODER_MODER1_0);
}