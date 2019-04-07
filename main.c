/*
 * Copyright (c) 2019 Maciej Suminski <orson@orson.net.pl>
 *
 * This source code is free software; you can redistribute it
 * and/or modify it in source code form under the terms of the GNU
 * General Public License as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#include "asf.h"
#include "buttons.h"
#include "io_capture.h"
#include "logic_analyzer.h"
#include "udc.h"
#include "udi_cdc.h"
#include "commands.h"
#include "spi_master.h"
#include "lcd.h"
#include "gfx.h"
#include "SSD1306_commands.h"
#include "scope.h"
#include "serial.h"
#include "menu.h"
#include "led.h"
#include "buffer.h"

/* Global large buffer */
buffer_t buffer;

volatile bool g_interrupt_enabled = true;

/**
 *  \brief Handler for System Tick interrupt.
 *
 *  Process System Tick Event
 */
void SysTick_Handler(void)
{
}


/**
 * \brief initialize pins, watchdog, etc.
 */
static void init_system(void)
{
    /* Disable the watchdog */
    wdt_disable(WDT);

    irq_initialize_vectors();
    cpu_irq_enable();

    /* Initialize the system clock */
    sysclk_init();

    /* Configure timer ISR to fire regularly (100 ms) */
    /* SysTick_Config((sysclk_get_cpu_hz() / 1000) * 100); */

    serial_init(115200);
    led_init();
    btn_init();
    spi_init(1000000, 0);
    SSD1306_init();
    ioc_init();
    la_init();
    udc_start();

    led_blink(LED2, 4);
}


int main(void)
{
    init_system();

    /* Splash screen */
    SSD1306_drawBitmap(0, 0, kicon_logo, 128, 32);
    SSD1306_drawBufferDMA();

    /* wait ~3s or till a button is pressed */
    for(int i = 0; i < (1 << 20); ++i) {
        if (btn_state()) {
            break;
        }
    }

    while(1) {
        menu();
    }
}
