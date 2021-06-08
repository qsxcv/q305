#pragma once

#include <stdint.h>
#include <nrf.h>
#include "radio.h"

// assumes port 0
#define SPI_SCK 28
#define SPI_MISO 29
#define SPI_MOSI 30
#define SPI_CS 31

volatile uint8_t spi_rx_buf[6];

static void spi_init(void)
{
	// config
	NRF_SPIS0->CONFIG = // mode 3
		(SPIS_CONFIG_ORDER_MsbFirst << SPIS_CONFIG_ORDER_Pos) |
		(SPIS_CONFIG_CPHA_Trailing << SPIS_CONFIG_CPHA_Pos) |
		(SPIS_CONFIG_CPOL_ActiveLow << SPIS_CONFIG_CPOL_Pos);

	NRF_SPIS0->DEF = 0xFF; // default character

	// pin select
	NRF_SPIS0->PSEL.SCK =
		(SPI_SCK << SPIS_PSEL_SCK_PIN_Pos) |
		(0 << SPIS_PSEL_SCK_PORT_Pos) |
		(SPIS_PSEL_SCK_CONNECT_Connected << SPIS_PSEL_SCK_CONNECT_Pos);
	NRF_SPIS0->PSEL.MISO =
		(SPI_MISO << SPIS_PSEL_MISO_PIN_Pos) |
		(0 << SPIS_PSEL_MISO_PORT_Pos) |
		(SPIS_PSEL_MISO_CONNECT_Connected << SPIS_PSEL_MISO_CONNECT_Pos);
	NRF_SPIS0->PSEL.MOSI =
		(SPI_MOSI << SPIS_PSEL_MOSI_PIN_Pos) |
		(0 << SPIS_PSEL_MOSI_PORT_Pos) |
		(SPIS_PSEL_MOSI_CONNECT_Connected << SPIS_PSEL_MOSI_CONNECT_Pos);
	NRF_SPIS0->PSEL.CSN =
		(SPI_CS << SPIS_PSEL_CSN_PIN_Pos) |
		(0 << SPIS_PSEL_CSN_PORT_Pos) |
		(SPIS_PSEL_CSN_CONNECT_Connected << SPIS_PSEL_CSN_CONNECT_Pos);

	// gpio config
	const uint32_t in =
		(GPIO_PIN_CNF_DIR_Input << GPIO_PIN_CNF_DIR_Pos) |
		(GPIO_PIN_CNF_INPUT_Connect << GPIO_PIN_CNF_INPUT_Pos);
	NRF_P0->PIN_CNF[SPI_CS] = in |
		(GPIO_PIN_CNF_PULL_Pullup << GPIO_PIN_CNF_PULL_Pos);
	NRF_P0->PIN_CNF[SPI_SCK] = in;
	NRF_P0->PIN_CNF[SPI_MOSI] = in;
	NRF_P0->PIN_CNF[SPI_MISO] = in;

	// pointers
	NRF_SPIS0->RXD.PTR = (uint32_t)&spi_rx_buf;
	NRF_SPIS0->RXD.MAXCNT = 6;
	NRF_SPIS0->TXD.PTR = (uint32_t)&radio_pkt_tx;
	NRF_SPIS0->TXD.MAXCNT = 6;

	// enable
	NRF_SPIS0->ENABLE =
		(SPIS_ENABLE_ENABLE_Enabled << SPIM_ENABLE_ENABLE_Pos);
}
