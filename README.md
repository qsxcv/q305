# q305

demonstration of wireless 8kHz on g305.

## how?

well, logitech conveniently included [test points on the g305 pcb](https://github.com/perigoso/g305-re#testpoints)
that break out the mcu's debug port.

### mouse

the mcu of the g305 is the [nrf52810](https://www.nordicsemi.com/Products/Low-power-short-range-wireless/nRF52810).

the nrf52 radio has a [fast ramp-up option](https://infocenter.nordicsemi.com/topic/ps_nrf52810/radio.html?cp=4_5_0_5_13_14_7#unique_345893154)
that allows the radio to start transmission 40us after enabling.

the radio packet size is 13 bytes (preamble:1, address:4, data:6, crc:2),
corresponding to 13*8 = 104bits. at 2 Mbps, this equates 52us on-air time.

so it is easily possible to transmit packets every 100us, which is well under
125us. actually it's possible to transmit even more frequently if the radio is
never disabled, but that wastes power. it is also possible to use a shorter
address and/or a 1 byte crc, at the cost of lower tolerance to
noise/interference.

### receiver

it is necessary to use two mcu's as currently there is no mcu supporting both a
wireless radio and a high-speed usb controller.

here, the receiver consists of the [nrf52840 dk](https://www.nordicsemi.com/Software-and-Tools/Development-Kits/nRF52840-DK)
and [stm32f723 disco](https://www.st.com/en/evaluation-tools/32f723ediscovery.html)
boards.

they are connected with a couple jumpers for communication over SPI
|      | nrf52840 | stm32f7 |
| ---- | -------- | ------- |
| SCLK |    P0.28 |     PA5 |
| MISO |    P0.29 |     PB4 |
| MOSI |    P0.30 |     PB5 |
| CS   |    P0.31 |     PA1 |

## details

### hero sensor "overclocking"

by default the hero sensor in the g305 is configured to run at roughly
1000 frames/s, and scales up to 12000 frames/s depending on the motion speed.

apparently, [register 0x20](https://github.com/perigoso/hero-re/#registers)
controls the maximum frame period (and hence, the minimum framerate) and can be
set such that the minimum framerate is slightly above 8000 frames/s.

### main loop synchronization

both the phase and frequency of the main loop in the mouse are synchronized to
the CS pin in the receiver. this is accomplished by requesting and receiving
timing information at 1 second intervals, whenever the mouse is idle. (possible
to increase intervals to >30s once locked in.)

packets are received by the receiver in a timing window of less than 1us. this
allows the time between receiving the wireless data and putting it in the usb
cable to be minimized.

### power consumption/battery life

will fill out details later. at 8kHz with "overclocked" sensor, roughly 3x
higher consumption than stock firmware. at 1kHz, roughly 30-40% lower
consumption than stock firmware.

## todo

### easy things

- dpi button
- powersaving and sleep modes

### harder things

- pairing with receiver
- frequency selection/hopping?
- configuration software
- firmware updates (over SPI for the receiver's wireless mcu, and over-the-air
  for the mouse)
- encryption (not sure if possible without compromising performance and/or
  power consumption)

## related repositories

information about g305 and its hero sensor:

https://github.com/perigoso/g305-re

https://github.com/perigoso/hero-re

receiver board using nrf52810 + atsam3u (work in progress):

https://github.com/perigoso/russian-woodpecker/

## contact

wtflood#6025 on discord

if you're interested in technical details, ask to join our group chat.
