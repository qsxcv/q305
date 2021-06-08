#pragma once

#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <nrf.h>

#define RADIO_MOUSE_FIRST 0x80
#define RADIO_MOUSE_SYNC 0x40
#define RADIO_MOUSE_IGNORE 0x20

// note: _tx and _rx suffixes are from the perspective of the mouse

// packets sent from mouse to receiver
union radio_pkt_tx_t {
	struct __PACKED {
		uint16_t x, y; // absolute
		uint8_t btn; // 3 msb bits used as flags
		uint8_t whl; // absolute
	} mouse;
	struct __PACKED { // easier to compare two packets
		uint32_t x_y;
		uint16_t btn_whl;
	} mouse_compact;
};
volatile union radio_pkt_tx_t radio_pkt_tx = {0};
#ifdef MOUSE
union radio_pkt_tx_t radio_pkt_tx_prev = {0};
#endif
static_assert(sizeof(radio_pkt_tx) == 6);

// packets sent from receiver to mouse
union radio_pkt_rx_t {
	int16_t time_diff;
	// in future, extend with data for commands from receiver to mouse
};
volatile union radio_pkt_rx_t radio_pkt_rx = {0};

static inline void radio_conf_tx(void)
{
	NRF_RADIO->PACKETPTR = (uint32_t)&radio_pkt_tx;
	NRF_RADIO->PCNF1 =
		(sizeof(radio_pkt_tx) << RADIO_PCNF1_MAXLEN_Pos) |
		(sizeof(radio_pkt_tx) << RADIO_PCNF1_STATLEN_Pos) |
		(4 << RADIO_PCNF1_BALEN_Pos) |
		(RADIO_PCNF1_ENDIAN_Little << RADIO_PCNF1_ENDIAN_Pos) |
		(RADIO_PCNF1_WHITEEN_Enabled << RADIO_PCNF1_WHITEEN_Pos);
}

static inline void radio_conf_rx(void)
{
	NRF_RADIO->PACKETPTR = (uint32_t)&radio_pkt_rx;
	NRF_RADIO->PCNF1 =
		(sizeof(radio_pkt_rx) << RADIO_PCNF1_MAXLEN_Pos) |
		(sizeof(radio_pkt_rx) << RADIO_PCNF1_STATLEN_Pos) |
		(4 << RADIO_PCNF1_BALEN_Pos) |
		(RADIO_PCNF1_ENDIAN_Little << RADIO_PCNF1_ENDIAN_Pos) |
		(RADIO_PCNF1_WHITEEN_Enabled << RADIO_PCNF1_WHITEEN_Pos);
}

static void radio_init()
{
	NRF_RADIO->MODE = RADIO_MODE_MODE_Ble_2Mbit << RADIO_MODE_MODE_Pos;

	// Configure packet with no S0, S1, or Length fields and 8-bit preamble.
	NRF_RADIO->PCNF0 =
		(0 << RADIO_PCNF0_LFLEN_Pos) |
		(0 << RADIO_PCNF0_S0LEN_Pos) |
		(0 << RADIO_PCNF0_S1LEN_Pos) |
		(RADIO_PCNF0_S1INCL_Automatic << RADIO_PCNF0_S1INCL_Pos) |
		(RADIO_PCNF0_PLEN_8bit << RADIO_PCNF0_PLEN_Pos);

	// fast rampup
	NRF_RADIO->MODECNF0 =
		(RADIO_MODECNF0_DTX_Center << RADIO_MODECNF0_DTX_Pos) |
		(RADIO_MODECNF0_RU_Fast << RADIO_MODECNF0_RU_Pos);

	NRF_RADIO->FREQUENCY =
		(20 << RADIO_FREQUENCY_FREQUENCY_Pos) |
		(RADIO_FREQUENCY_MAP_Low << RADIO_FREQUENCY_MAP_Pos);
	//NRF_RADIO->DATAWHITEIV = 0x55;
	NRF_RADIO->BASE0   = 0xE7E7E7E7;
	NRF_RADIO->BASE1   = 0x43434343;
	NRF_RADIO->PREFIX0 = 0x23C343E7;
	NRF_RADIO->PREFIX1 = 0x13E363A3;
	NRF_RADIO->CRCCNF =
		(RADIO_CRCCNF_LEN_Two << RADIO_CRCCNF_LEN_Pos) |
		(RADIO_CRCCNF_SKIPADDR_Skip << RADIO_CRCCNF_SKIPADDR_Pos);
	NRF_RADIO->CRCPOLY = 0x00011021;
	NRF_RADIO->CRCINIT = 0x0000FFFF;

#if defined(MOUSE)
	NRF_RADIO->TXADDRESS = 0 << RADIO_TXADDRESS_TXADDRESS_Pos;
	NRF_RADIO->TXPOWER = RADIO_TXPOWER_TXPOWER_0dBm << RADIO_TXPOWER_TXPOWER_Pos;
	NRF_RADIO->RXADDRESSES = RADIO_RXADDRESSES_ADDR1_Enabled << RADIO_RXADDRESSES_ADDR1_Pos;

	// Configure shortcuts to start as soon as READY event is received, and disable radio as soon as packet is sent.
	NRF_RADIO->SHORTS =
		(RADIO_SHORTS_READY_START_Enabled << RADIO_SHORTS_READY_START_Pos) |
		(RADIO_SHORTS_END_DISABLE_Enabled << RADIO_SHORTS_END_DISABLE_Pos);

	NRF_RADIO->INTENSET = RADIO_INTENSET_DISABLED_Enabled << RADIO_INTENSET_DISABLED_Pos;
	NVIC_EnableIRQ(RADIO_IRQn);
#elif defined(RECEIVER)
	NRF_RADIO->RXADDRESSES = RADIO_RXADDRESSES_ADDR0_Enabled << RADIO_RXADDRESSES_ADDR0_Pos;
	NRF_RADIO->TXADDRESS = 1 << RADIO_TXADDRESS_TXADDRESS_Pos;
	NRF_RADIO->TXPOWER = RADIO_TXPOWER_TXPOWER_Pos8dBm << RADIO_TXPOWER_TXPOWER_Pos;

	NRF_RADIO->SHORTS =
		(RADIO_SHORTS_READY_START_Enabled << RADIO_SHORTS_READY_START_Pos);
#else
	#error define either MOUSE or RECEIVER
#endif
}

#ifdef MOUSE
static inline void radio_wait_disabled()
{
	while (NRF_RADIO->EVENTS_DISABLED == 0)
		__WFI();
	NRF_RADIO->EVENTS_DISABLED = 0;
	NVIC_ClearPendingIRQ(RADIO_IRQn);
}
#endif
