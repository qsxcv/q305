#pragma once

#include <stddef.h>
#include <stdint.h>
#include <nrf.h>
#include "delay.h"
#include "spi.h"
#include "hero_blob.h"

static void hero_reg_write(const uint8_t reg, const uint8_t val)
{
	(void)spi_txrx(reg & ~0x80);
	(void)spi_txrx(val);
}
/*
static uint8_t hero_reg_write_readprev(const uint8_t reg, const uint8_t val)
{
	(void)spi_txrx(reg & ~0x80);
	return spi_txrx(val);
}
*/
static uint8_t hero_reg_read(const uint8_t reg)
{
	(void)spi_txrx(reg | 0x80);
	return spi_txrx(0x80);
}

struct two_bytes { uint8_t val1, val2; };
static struct two_bytes hero_reg_read2(const uint8_t reg1, const uint8_t reg2)
{
	struct two_bytes ret;
	(void)spi_txrx(reg1 | 0x80);
	ret.val1 = spi_txrx(reg2 | 0x80);
	ret.val2 = spi_txrx(0x80);
	return ret;
}

union motion_data {
	struct { int16_t dx; int16_t dy; };
	uint32_t u32;
};
static inline union motion_data hero_motion_burst(int set_ptrs)
{
	static struct __PACKED {
		uint8_t pad0, pad1, pad2; // pad for alignment of uint32_t later
		uint8_t junk;
		uint32_t yhi_ylo_xhi_xlo;
	} rx_buf;
	static uint8_t tx_buf[5] = {0x85, 0x86, 0x87, 0x88, 0x80};

	if (set_ptrs) {
		NRF_SPIM0->RXD.PTR = (uint32_t)&rx_buf.junk;
		NRF_SPIM0->RXD.MAXCNT = 5;
		NRF_SPIM0->TXD.PTR = (uint32_t)tx_buf;
		NRF_SPIM0->TXD.MAXCNT = 5;
	}

	spi_cs_low();
	__NOP();__NOP();__NOP();__NOP();__NOP();__NOP(); // not necessary
	__NOP();__NOP();__NOP();__NOP();__NOP();__NOP();
	NRF_SPIM0->TASKS_START = 1;
	spi_wait_end();
	spi_cs_high();

	union motion_data motion = {
		.u32=__REV(rx_buf.yhi_ylo_xhi_xlo) // big endian to little endian
	};
	return motion;
}

static void hero_set_dpi(const uint32_t dpi)
{
	const uint8_t val = (uint8_t)((dpi / 50) - 1);
	spi_cs_low();
	hero_reg_write(0x0C, val); // could also make x, y dpi independent
	hero_reg_write(0x0D, val);
	spi_cs_high();
}

// often called in changing run/sleep mode
static void hero_83_80_03_x(const uint8_t x)
{
	spi_cs_low();
	(void)hero_reg_read(0x03);
	spi_cs_high();
	delay_us(1);
	spi_cs_low();
	hero_reg_write(0x03, x);
	spi_cs_high();
}
/*
static void hero_sleep(void)
{
	spi_cs_low();
	hero_reg_write(0x0b, 0x70);
	spi_cs_high();
	hero_83_80_03_x(0x30);
	// TODO configure waking from sleep
}

static void hero_wake_from_sleep(void)
{
	// TODO disable waking
	hero_83_80_03_x(0x20);
	delay_us(10);
	(void)hero_motion_burst(1);
	delay_us(20);
	hero_83_80_03_x(0x20);
}

static void hero_deepsleep(void)
{
	spi_cs_low();
	hero_reg_write(0x0b, 0x70);
	spi_cs_high();
	hero_83_80_03_x(0x28);
	// TODO configure waking from deepsleep
}
*/

__attribute__((optimize("Os")))
static int hero_init(void)
{
	delay_us(1000); // probably not necessary

	// 0
	spi_cs_low();
	hero_reg_write(0x0A, 0x40);
	spi_cs_high();

	delay_us(1000);

	// 1 write blob
	spi_cs_low();
	hero_reg_write(0x2A, 0xCF); delay_us(20);
	hero_reg_write(0x2B, 0x06); delay_us(20);
	hero_reg_write(0x2C, 0x08); delay_us(20);
	hero_reg_write(0x2D, 0x00); delay_us(20);
	for (size_t i = 0; i < sizeof(hero_blob)/sizeof(hero_blob[0]); i += 2) {
		hero_reg_write(0x2E, hero_blob[i]);
		hero_reg_write(0x2F, hero_blob[i + 1]);
	}
	(void)spi_txrx(0x80);
	spi_cs_high();

	delay_us(1);

	// 2 verify blob
	spi_cs_low();
	hero_reg_write(0x2A, 0xCF); delay_us(20);
	hero_reg_write(0x2B, 0x02); delay_us(20);
	hero_reg_write(0x2C, 0x08); delay_us(20);
	hero_reg_write(0x2D, 0x00); delay_us(20);
	for (size_t i = 0; i < sizeof(hero_blob)/sizeof(hero_blob[0]); i += 2) {
		const struct two_bytes val = hero_reg_read2(0x2E, 0x2F);
		if (val.val1 != hero_blob[i] || val.val2 != hero_blob[i + 1])
			return 1;
	}
	spi_cs_high();

	delay_us(1);

	// 3
	spi_cs_low();
	hero_reg_write(0x2A, 0x00);
	spi_cs_high();

	delay_us(1);

	// 4
	spi_cs_low();
	hero_reg_write(0x76, 0x88);
	hero_reg_write(0x40, 0x82);
	hero_reg_write(0x43, 0x01);
	hero_reg_write(0x43, 0x00);
	(void)hero_reg_read2(0x4A, 0x4B);
	hero_reg_write(0x40, 0x83);
	hero_reg_write(0x43, 0x01);
	hero_reg_write(0x43, 0x00);
	(void)hero_reg_read2(0x4A, 0x4B);
	hero_reg_write(0x40, 0x84);
	hero_reg_write(0x43, 0x01);
	hero_reg_write(0x43, 0x00);
	(void)hero_reg_read2(0x4A, 0x4B);
	hero_reg_write(0x40, 0x85);
	hero_reg_write(0x43, 0x01);
	hero_reg_write(0x43, 0x00);
	(void)hero_reg_read2(0x4A, 0x4B);
	hero_reg_write(0x76, 0x00);
	spi_cs_high();

	delay_us(1);

	// 5
	spi_cs_low();
	hero_reg_write(0x30, 0x17);
	hero_reg_write(0x31, 0x40);
	hero_reg_write(0x32, 0x09);
	hero_reg_write(0x33, 0x09);
	hero_reg_write(0x34, 0x0F);
	hero_reg_write(0x35, 0x0B);
	spi_cs_high();

	delay_us(1);

	// 6 configure sleep?
	spi_cs_low();
	hero_reg_write(0x03, 0x40); delay_us(20);
	hero_reg_write(0x0B, 0x40); delay_us(20);
	hero_reg_write(0x0E, 0x11); delay_us(20);
	hero_reg_write(0x22, 0x08); delay_us(20);
	hero_reg_write(0x23, 0x97); delay_us(20);
	hero_reg_write(0x24, 0xB6); delay_us(20);
	hero_reg_write(0x25, 0x36); delay_us(20);
	hero_reg_write(0x26, 0xDA); delay_us(20);
	hero_reg_write(0x27, 0x50); delay_us(20);
	hero_reg_write(0x28, 0xC4); delay_us(20);
	hero_reg_write(0x29, 0x06); delay_us(20);
	hero_reg_write(0x64, 0x06); delay_us(20);
	spi_cs_high();

	delay_us(1);

	// 7
	spi_cs_low();
	(void)hero_reg_read(0x02);
	hero_reg_write(0x02, 0x80);
	spi_cs_high();

	delay_us(764);

	// 8 and 9
	hero_83_80_03_x(0x40);

	delay_us(7095);

	// 10 (dpi)
	hero_set_dpi(800);

	delay_us(2);

	// 11 (read motion counts)
	(void)hero_motion_burst(1);

	delay_us(4480);

	// 12 (similar to 6) configure run?
	spi_cs_low();
	hero_reg_write(0x03, 0x20); delay_us(20);
	hero_reg_write(0x0B, 0x40); delay_us(20);
	hero_reg_write(0x0E, 0x11); delay_us(20);
	hero_reg_write(0x22, 0xC8); delay_us(20);
	hero_reg_write(0x23, 0x97); delay_us(20);
	hero_reg_write(0x24, 0xB6); delay_us(20);
	hero_reg_write(0x25, 0x36); delay_us(20);
	hero_reg_write(0x26, 0xDA); delay_us(20);
	hero_reg_write(0x27, 0x50); delay_us(20);
	hero_reg_write(0x28, 0xC4); delay_us(20);
	hero_reg_write(0x29, 0x00); delay_us(20);
	hero_reg_write(0x64, 0x06); delay_us(20);
	spi_cs_high();

	delay_us(20);

	(void)hero_motion_burst(1);

	delay_us(50);

	spi_cs_low();
	hero_reg_write(0x20, 6); // framerate
	spi_cs_high();

	return 0;
}
