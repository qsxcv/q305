/* Linker script to configure memory regions. */

SEARCH_DIR(.)
GROUP(-lgcc -lc -lnosys)

MEMORY
{
  FLASH (rx) : ORIGIN = 0x00000000, LENGTH = 0x30000
  RAM (rwx) : ORIGIN = 0x20003000, LENGTH = 0x3000
  CODE_RAM (rx) : ORIGIN = 0x00800000, LENGTH = 0x3000
}


INCLUDE "nrf_common_ram.ld"
