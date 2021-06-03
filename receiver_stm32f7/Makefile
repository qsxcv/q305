#CMSIS = ../CMSIS_5/CMSIS/Core/Include
CMSIS = ../cmsis

CC      := arm-none-eabi-gcc
OBJCOPY := arm-none-eabi-objcopy
SIZE    := arm-none-eabi-size

CFLAGS  = -g3 -O3 -std=gnu17 -Wall -Wextra -pedantic
CFLAGS += -mcpu=cortex-m7 -mthumb -mabi=aapcs
CFLAGS += -ffunction-sections -fdata-sections -fno-strict-aliasing -fno-builtin -fshort-enums

DEFINES = -DSTM32F723xx

INCLUDE = -I$(CMSIS) -I. -Istm_stuff -Istm_usb

LDFLAGS = -Tstm_stuff/STM32F723IEKX_FLASH.ld -Wl,--gc-sections --specs=nano.specs

STARTUP = stm_stuff/startup_stm32f723xx.s stm_stuff/system_stm32f7xx.c

SOURCES = main.c delay.c usb.c

SOURCES_USB = stm_usb/stm32f7xx_ll_usb.c stm_usb/usbd_conf.c stm_usb/usbd_ctlreq.c
SOURCES_USB += stm_usb/usbd_desc.c stm_usb/usbd_hid.c stm_usb/usbd_ioreq.c

TARGET = receiver_stm

all:
	$(CC) $(CFLAGS) \
		$(INCLUDE) \
		$(LDFLAGS) \
		$(STARTUP) $(SOURCES) $(SOURCES_USB) \
		$(DEFINES) \
		-o $(TARGET).elf
	$(OBJCOPY) -Obinary $(TARGET).elf $(TARGET).bin
	$(SIZE) $(TARGET).elf

clean:
	rm -f *.o *.elf *.bin *.hex
