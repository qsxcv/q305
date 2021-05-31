#CMSIS = ../CMSIS_5/CMSIS/Core/Include
#MDK   = ../nrfx/mdk
CMSIS = ../cmsis
MDK = ../mdk
MDK_RAM = ../mdk_ram

CC      := arm-none-eabi-gcc
OBJCOPY := arm-none-eabi-objcopy
SIZE    := arm-none-eabi-size

CFLAGS  = -g3 -O3 -std=gnu17 -Wall -Wextra -pedantic
CFLAGS += -mcpu=cortex-m4 -mthumb -mabi=aapcs
CFLAGS += -ffunction-sections -fdata-sections -fno-strict-aliasing -fno-builtin -fshort-enums

DEFINES = -DCONFIG_GPIO_AS_PINRESET -DNRF52840_XXAA -DRECEIVER

INCLUDE  = -I$(CMSIS)
INCLUDE += -I$(MDK)
INCLUDE += -I../common

LDFLAGS_FLASH = -L$(MDK) -Tnrf52840_xxaa.ld -Wl,--gc-sections --specs=nano.specs
LDFLAGS_RAM = -L$(MDK_RAM) -T$(MDK_RAM)/nrf52840_xxaa_ram.ld -Wl,--gc-sections --specs=nano.specs
LDFLAGS_RAMCOPY += -L$(MDK_RAM) -T$(MDK_RAM)/nrf52840_xxaa_ramcopy.ld -Wl,--gc-sections --specs=nano.specs

STARTUP_FLASH = $(MDK)/gcc_startup_nrf52840.S $(MDK)/system_nrf52.c
STARTUP_RAM = $(STARTUP_FLASH)
STARTUP_RAMCOPY = $(MDK_RAM)/gcc_startup_nrf52840_ramcopy.S $(MDK_RAM)/system_nrf52_ramcopy.c

SOURCES = main.c

TARGET = receiver

all: flash ram ramcopy

# runs from flash
flash:
	$(CC) $(CFLAGS) \
		$(INCLUDE) \
		$(LDFLAGS_FLASH) \
		$(STARTUP_FLASH) $(SOURCES) \
		$(DEFINES) \
		-o $(TARGET)_flash.elf
	$(OBJCOPY) -Obinary $(TARGET)_flash.elf $(TARGET)_flash.bin
	$(SIZE) $(TARGET)_flash.elf

# runs purely from code ram (0x00800000)
ram:
	$(CC) $(CFLAGS) \
		$(INCLUDE) \
		$(LDFLAGS_RAM) \
		$(STARTUP_RAM) $(SOURCES) \
		$(DEFINES) -DVECT_TAB=0x00800000 -D__STARTUP_SKIP_ETEXT \
		-o $(TARGET)_ram.elf
	$(OBJCOPY) -Obinary $(TARGET)_ram.elf $(TARGET)_ram.bin
	$(SIZE) $(TARGET)_ram.elf

# copies from flash to code ram
ramcopy:
	$(CC) $(CFLAGS) \
		$(INCLUDE) \
		$(LDFLAGS_RAMCOPY) \
		$(STARTUP_RAMCOPY) $(SOURCES) \
		$(DEFINES) -DVECT_TAB=0x00800000 \
		-o $(TARGET)_ramcopy.elf
	$(OBJCOPY) -Obinary $(TARGET)_ramcopy.elf $(TARGET)_ramcopy.bin
	$(SIZE) $(TARGET)_ramcopy.elf

clean:
	rm -f *.o *.elf *.bin *.hex
