#pragma once

#include <stdint.h>
#include "hero.h"

#define DPI_INDEX_BOOT 1
const uint32_t dpi_list[] = {50, 800, 3200, 12800};
const int dpi_list_len = sizeof(dpi_list)/sizeof(dpi_list[0]);

void dpi_process(uint8_t btn_now) {
	static uint8_t btn_prev = 0;
    static int dpi_index = 1;

    if ((btn_now & ~btn_prev) & (1 << 5)) {
        dpi_index++;
        if (dpi_index == dpi_list_len) dpi_index = 0;
        hero_set_dpi(dpi_list[dpi_index]);
        delay_us(1);
        hero_conf_motion();
    }
    btn_prev = btn_now;
}
