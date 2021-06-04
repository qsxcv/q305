#pragma once

#include <stdint.h>
#include "stm32f7xx.h"

void delay_init(void);

static inline void delay_ticks(const uint32_t ticks)
{
	TIM2->CNT = ticks; // assumes TIM2CLK = 64MHz
	TIM2->CR1 = TIM_CR1_CEN | TIM_CR1_DIR;
	while (TIM2->CR1 != 0) // can comment if only interrupt is TIM2_IRQHandler
		__WFI();
}

static inline void delay_us(const uint32_t us)
{
	delay_ticks(64*us - 1);
}

#define delay_ms(x) delay_us(1000*(x))
