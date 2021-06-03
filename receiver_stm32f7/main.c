#include "stm32f7xx.h"
#include "led.h"
#include "delay.h"
#include "spi.h"
#include "usbd_hid.h"
#include "usb.h"

// flags sent over radio in btn
#define RADIO_MOUSE_FIRST 0x80
#define RADIO_MOUSE_SYNC 0x40
#define RADIO_MOUSE_IGNORE 0x20

typedef union {
	struct __PACKED {
		int16_t x, y;
		uint8_t btn;
		int8_t whl;
	};
	uint8_t u8[6];
} Spi_packet;
static_assert(sizeof(Spi_packet) == 6);

typedef union {
	struct __PACKED { // use the order in the report descriptor
		uint8_t btn;
		int8_t dwhl;
		int16_t dx, dy;
		uint16_t _pad; // zero pad to 8 bytes total
	};
	uint32_t u32[2];
} Usb_packet;
static_assert(sizeof(Usb_packet) == 2*sizeof(uint32_t));

void clk_init(void)
{
	RCC->CR |= RCC_CR_HSEON; // turn on HSE
    while ((RCC->CR & RCC_CR_HSERDY) == 0);

    RCC->CR &= ~RCC_CR_PLLON; // disable PLL (off by default, not necessary)
    while ((RCC->CR & RCC_CR_PLLRDY) != 0);

    /* Configure the main PLL clock source, multiplication and division factors. */
    MODIFY_REG(RCC->PLLCFGR,
         RCC_PLLCFGR_PLLM | RCC_PLLCFGR_PLLN | RCC_PLLCFGR_PLLP | RCC_PLLCFGR_PLLSRC | RCC_PLLCFGR_PLLQ,
         _VAL2FLD(RCC_PLLCFGR_PLLM, 25) |
		 _VAL2FLD(RCC_PLLCFGR_PLLN, 256) |
		 _VAL2FLD(RCC_PLLCFGR_PLLP, 0) |  // for P = 2
		 (RCC_PLLCFGR_PLLSRC_HSE) |
		 _VAL2FLD(RCC_PLLCFGR_PLLQ, 8)
    );

    RCC->CR |= RCC_CR_PLLON; // enable PLL
    while ((RCC->CR & RCC_CR_PLLRDY) == 0);

    // flash latency
    MODIFY_REG(FLASH->ACR, FLASH_ACR_LATENCY, FLASH_ACR_LATENCY_7WS);

    /* Set the highest APBx dividers in order to ensure that we do not go through
       a non-spec phase whatever we decrease or increase HCLK. */
    MODIFY_REG(RCC->CFGR, RCC_CFGR_PPRE1, RCC_CFGR_PPRE1_DIV16);
    MODIFY_REG(RCC->CFGR, RCC_CFGR_PPRE2, RCC_CFGR_PPRE2_DIV16);

    MODIFY_REG(RCC->CFGR, RCC_CFGR_HPRE, RCC_CFGR_HPRE_DIV1); // ahb clk = sysclk
    MODIFY_REG(RCC->CFGR, RCC_CFGR_SW, RCC_CFGR_SW_PLL); // set pll as sys clock
    while ((RCC->CFGR & RCC_CFGR_SWS) != RCC_CFGR_SWS_PLL);

    MODIFY_REG(RCC->CFGR, RCC_CFGR_PPRE1, RCC_CFGR_PPRE1_DIV4); // apb1 = sysclk/4
    MODIFY_REG(RCC->CFGR, RCC_CFGR_PPRE2, RCC_CFGR_PPRE2_DIV2); // apb2 = sysclk/2
    RCC->CR &= ~RCC_CR_HSION; // turn off HSI (not necessary)
}

int main(void)
{
	SCB_EnableICache();
	SCB_EnableDCache();

	led_init();
	clk_init();
	delay_init();
	spi_init();
	usb_init(1);

	const uint32_t USBx_BASE = (uint32_t) USB_OTG_HS; // used in macros USBx_*
	// fifo space when empty, should equal 0x174, from init_usb
	const uint32_t fifo_space = (USBx_INEP(1)->DTXFSTS & USB_OTG_DTXFSTS_INEPTFSAV);

	int8_t whl_prev = 0;
	int16_t x_prev = 0, y_prev = 0;
	Spi_packet rx = {0}; // absolute coordinates received from spi
	Usb_packet new = {0}; // what's new this loop
	Usb_packet send = {0}; // what's transmitted

	USB_OTG_HS->GINTMSK |= USB_OTG_GINTMSK_SOFM; // enable SOF interrupt
	while (1) {
		// always check that usb is configured
		usb_wait_configured();

		// wait for SOF to sync to usb frames
		USB_OTG_HS->GINTSTS |= USB_OTG_GINTSTS_SOF;
		__WFI();

		// delay to read data as late as possible
		delay_us(100);

		// read sensor, wheel, buttons
		spi_cs_low();
		delay_us(1);
		rx.u8[0] = spi_recv();
		rx.u8[1] = spi_recv();
		rx.u8[2] = spi_recv();
		rx.u8[3] = spi_recv();
		rx.u8[4] = spi_recv();
		rx.u8[5] = spi_recv();
		delay_us(1);
		spi_cs_high();

		if ((rx.btn & RADIO_MOUSE_IGNORE) != 0) {
			continue;
		} else if ((rx.btn & RADIO_MOUSE_FIRST) != 0) {
			whl_prev = rx.whl;
			x_prev = rx.x;
			y_prev = rx.y;
			continue;
		} else {
			new.btn = rx.btn & 0b00011111; // mask flags
			new.dwhl = rx.whl - whl_prev;
			new.dx = rx.x - x_prev;
			new.dy = y_prev - rx.y; // invert y
			whl_prev = rx.whl;
			x_prev = rx.x;
			y_prev = rx.y;
		}

		// if last packet still sitting in fifo
		if ((USBx_INEP(1)->DTXFSTS & USB_OTG_DTXFSTS_INEPTFSAV) < fifo_space) {
			// flush fifo
			USB_OTG_HS->GRSTCTL = _VAL2FLD(USB_OTG_GRSTCTL_TXFNUM, 1) | USB_OTG_GRSTCTL_TXFFLSH;
			while ((USB_OTG_HS->GRSTCTL & USB_OTG_GRSTCTL_TXFFLSH) != 0);
		} else { // last loop transmitted successfully
			send.dwhl = 0;
			send.dx = 0;
			send.dy = 0;
		}
		send.dwhl += new.dwhl;
		send.dx += new.dx;
		send.dy += new.dy;

		// if there is data to transmitted
		if (new.btn != send.btn || send.dwhl || send.dx || send.dy) {
			send.btn = new.btn;
			// disable transfer complete interrupts, in case enabled by OTG_HS_IRQHandler
			USBx_DEVICE->DIEPMSK &= ~USB_OTG_DIEPMSK_XFRCM;
			// set up transfer size
			MODIFY_REG(USBx_INEP(1)->DIEPTSIZ,
					USB_OTG_DIEPTSIZ_PKTCNT | USB_OTG_DIEPTSIZ_XFRSIZ,
					_VAL2FLD(USB_OTG_DIEPTSIZ_PKTCNT, 1) | _VAL2FLD(USB_OTG_DIEPTSIZ_XFRSIZ, HID_EPIN_SIZE));
			// enable endpoint
			USBx_INEP(1)->DIEPCTL |= USB_OTG_DIEPCTL_CNAK | USB_OTG_DIEPCTL_EPENA;
			// write to fifo
			USBx_DFIFO(1) = send.u32[0];
			USBx_DFIFO(1) = send.u32[1];
		}
	}
}
