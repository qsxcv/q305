#pragma once

#include <stdint.h>
#include "stm32f7xx.h"

static void spi_init(void)
{
	SET_BIT(RCC->AHB1ENR, RCC_AHB1ENR_GPIOAEN | RCC_AHB1ENR_GPIOBEN);
	__NOP(); __NOP();

	// PA5 SCK
	MODIFY_REG(GPIOA->MODER,
			0b11 << (2*5),
			0b10 << (2*5));
	MODIFY_REG(GPIOA->AFR[5 >= 8],
			0b1111 << ((4*5)%32),
			5 << ((4*5)%32));
	// PB4 MISO
	MODIFY_REG(GPIOB->MODER,
			0b11 << (2*4),
			0b10 << (2*4));
	MODIFY_REG(GPIOB->AFR[4 >= 8],
			0b1111 << ((4*4)%32),
			5 << ((4*4)%32));
	// PB5 MOSI
	MODIFY_REG(GPIOB->MODER,
			0b11 << (2*5),
			0b10 << (2*5));
	MODIFY_REG(GPIOB->AFR[5 >= 8],
			0b1111 << ((4*5)%32),
			5 << ((4*5)%32));
	// PA1 CS
	MODIFY_REG(GPIOA->MODER,
			0b11 << (2*1),
			0b01 << (2*1));

	// SPI config
	RCC->APB2ENR |= RCC_APB2ENR_SPI1EN;
	SPI1->CR1 = SPI_CR1_SSM | SPI_CR1_SSI // software SS
			| (0b010 << SPI_CR1_BR_Pos) // assumes PCLK2 = 64MHz. divide by 8 for 8MHz
			| SPI_CR1_MSTR // master
			| SPI_CR1_CPOL // CPOL = 1
			| SPI_CR1_CPHA; // CPHA = 1
	SPI1->CR2 = SPI_CR2_FRXTH // 8-bit level for RXNE
			| (0b0111 << SPI_CR2_DS_Pos); // 8-bit data
	SPI1->CR1 |=  SPI_CR1_SPE; // enable SPI
}

static inline void spi_cs_low(void)
{
	GPIOA->ODR &= ~(1<<1); // PA1
}

static inline void spi_cs_high(void)
{
	GPIOA->ODR |= (1<<1); // PA1
}

static inline uint8_t spi_sendrecv(uint8_t b)
{
    while (!(SPI1->SR & SPI_SR_TXE));
    *(__IO uint8_t *)&SPI1->DR = b;
    while (!(SPI1->SR & SPI_SR_RXNE));
    return *(__IO uint8_t *)&SPI1->DR;
}

#define spi_recv(x) spi_sendrecv(0)
#define spi_send(x) (void)spi_sendrecv(x)
