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

#include "lcd.h"

#include <sysclk.h>
#include <twi.h>
#include <pio.h>

void lcd_init(void)
{
    twi_options_t opt;
    opt.master_clk = sysclk_get_peripheral_hz();
    opt.speed      = 400000;    // TODO

    pio_configure(PIOA, PIO_PERIPH_A,
            (PIO_PA3A_TWD0 | PIO_PA4A_TWCK0), PIO_OPENDRAIN | PIO_PULLUP);
    sysclk_enable_peripheral_clock(ID_TWI0);
    pmc_enable_periph_clk(ID_TWI0);

    if (twi_master_init(TWI0, &opt) != TWI_SUCCESS) {}

    // TODO twi_probe? 
}

void lcd_write(void)
{
    const uint8_t *data = (const uint8_t*) "1234";
    twi_packet_t packet_tx;

    packet_tx.chip        = 0x11;
    packet_tx.addr[0]     = 0x1234 >> 8;
    packet_tx.addr[1]     = 0x1234 & 0xff;
    packet_tx.addr_length = 2;
    packet_tx.buffer      = (uint8_t *) data;
    packet_tx.length      = 4;

    twi_master_write(TWI0, &packet_tx);
}
