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

#include "serial.h"
#include "buttons.h"
#include "lcd.h"
#include "command_handlers.h"
#include "apps_list.h"
#include "settings_list.h"

#include <uart.h>
#include <sysclk.h>
#include <pio.h>
#include <udi_cdc.h>

void serial_init(unsigned int baud) {
    sam_uart_opt_t uart_settings;
    uart_settings.ul_mck = sysclk_get_peripheral_hz();
    uart_settings.ul_baudrate = baud;
    uart_settings.ul_mode = UART_MR_PAR_NO;

    pio_configure(PIOA, PIO_PERIPH_A, (PIO_PA9A_URXD0 | PIO_PA10A_UTXD0), PIO_DEFAULT);
    sysclk_enable_peripheral_clock(ID_UART0);
    uart_init(UART0, &uart_settings);
}


void serial_putc(char c) {
    while(!uart_is_tx_buf_empty(UART0));
    uart_write(UART0, c);
}


void serial_puts(const char *str) {
    while(str) {
        while(!uart_is_tx_buf_empty(UART0));
        uart_write(UART0, *str++);
    }
}


void serial_putsn(const char *str, unsigned int count) {
    for (unsigned int i = 0; i < count; ++i) {
        while(!uart_is_tx_buf_empty(UART0));
        uart_write(UART0, *str++);
    }
}


void cmd_uart(const uint8_t* data_in, unsigned int input_len) {
    for (unsigned int i = 0; i < input_len; ++i) {
        while(!uart_is_tx_buf_empty(UART0));
        uart_write(UART0, data_in[i]);
    }

    cmd_resp_init(CMD_RESP_OK);
}


void app_uart_func(void) {
    uint8_t c;

    while(SSD1306_isBusy());
    SSD1306_clearBufferFull();
    SSD1306_setString(15, 3, "USB-UART adapter", 16, WHITE);
    SSD1306_drawBufferDMA();

    switch (menu_uart_baud.val) {
        case 0: serial_init(115200); break;
        case 1: serial_init(57600); break;
        case 2: serial_init(38400); break;
        case 3: serial_init(9600); break;
    }

    while (btn_state() != BUT_LEFT) {
        // USB -> serial
        if (udi_cdc_is_rx_ready()) {
            serial_putc(udi_cdc_getc());
        }

        // serial -> USB
        if (!uart_read(UART0, &c)) {
            udi_cdc_putc(c);
        }
    }

    while (btn_state());    // wait for the button release
}
