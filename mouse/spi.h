#pragma once

#include <stdint.h>
#include <nrf.h>
#include "pins.h"

static void spi_init(void)
{
	// config
	NRF_SPIM0->CONFIG = // mode 3
		(SPIM_CONFIG_ORDER_MsbFirst << SPIM_CONFIG_ORDER_Pos) |
		(SPIM_CONFIG_CPHA_Trailing << SPIM_CONFIG_CPHA_Pos) |
		(SPIM_CONFIG_CPOL_ActiveLow << SPIM_CONFIG_CPOL_Pos);
	NRF_SPIM0->FREQUENCY = // 8mbps
		(SPIM_FREQUENCY_FREQUENCY_M8 << SPIM_FREQUENCY_FREQUENCY_Pos);

	// pin select
	NRF_SPIM0->PSEL.SCK =
		(SPI_SCK << SPIM_PSEL_SCK_PIN_Pos) |
		(SPIM_PSEL_SCK_CONNECT_Connected << SPIM_PSEL_SCK_CONNECT_Pos);
	NRF_SPIM0->PSEL.MOSI =
		(SPI_MOSI << SPIM_PSEL_MOSI_PIN_Pos) |
		(SPIM_PSEL_MOSI_CONNECT_Connected << SPIM_PSEL_MOSI_CONNECT_Pos);
	NRF_SPIM0->PSEL.MISO =
		(SPI_MISO << SPIM_PSEL_MISO_PIN_Pos) |
		(SPIM_PSEL_MISO_CONNECT_Connected << SPIM_PSEL_MISO_CONNECT_Pos);

	// gpio config
	const uint32_t out =
		(GPIO_PIN_CNF_DIR_Output << GPIO_PIN_CNF_DIR_Pos) |
		(GPIO_PIN_CNF_INPUT_Disconnect << GPIO_PIN_CNF_INPUT_Pos);
	NRF_P0->PIN_CNF[SPI_CS] = out;
	NRF_P0->PIN_CNF[SPI_SCK] = out;
	NRF_P0->PIN_CNF[SPI_MOSI] = out;
	NRF_P0->PIN_CNF[SPI_MISO] =
		(GPIO_PIN_CNF_DIR_Input << GPIO_PIN_CNF_DIR_Pos) |
		(GPIO_PIN_CNF_PULL_Pulldown << GPIO_PIN_CNF_PULL_Pos);
	NRF_P0->OUTSET = (1 << SPI_CS) | (1 << SPI_SCK);
	NRF_P0->OUTCLR = (1 << SPI_MOSI);

	NRF_SPIM0->INTENSET = SPIM_INTENSET_END_Enabled << SPIM_INTENSET_END_Pos;
	NVIC_EnableIRQ(SPIM0_SPIS0_SPI0_IRQn);

	// enable
	NRF_SPIM0->ENABLE =
		(SPIM_ENABLE_ENABLE_Enabled << SPIM_ENABLE_ENABLE_Pos);
}

static inline void spi_cs_low(void)
{
	NRF_P0->OUTCLR = (1 << SPI_CS);
}

static inline void spi_cs_high(void)
{
	NRF_P0->OUTSET = (1 << SPI_CS);
}

static inline void spi_wait_end()
{
	do {
		__WFI();
	} while (NRF_SPIM0->EVENTS_END == 0);
	NRF_SPIM0->EVENTS_END = 0;
	NVIC_ClearPendingIRQ(SPIM0_SPIS0_SPI0_IRQn);
}


static uint8_t spi_txrx(const uint8_t b)
{
	static volatile uint8_t rx_byte, tx_byte;
	tx_byte = b;

	NRF_SPIM0->RXD.PTR = (uint32_t)&rx_byte;
	NRF_SPIM0->RXD.MAXCNT = 1;
	NRF_SPIM0->TXD.PTR = (uint32_t)&tx_byte;
	NRF_SPIM0->TXD.MAXCNT = 1;

	NRF_SPIM0->TASKS_START = 1;
	spi_wait_end();

	return rx_byte;
}
