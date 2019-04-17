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

#include "io_conf.h"
#include "i2c.h"
#include "lcd.h"
#include "pio.h"

static io_config_t current_conf = IO_UNINITIALIZED;
static unsigned int user_i2c_clk = 100000;

void io_configure(io_config_t conf) {
    if (current_conf == conf) {
        return;
    }

    /* save the user I2C clock settings before switching to a new mode */
    if (current_conf == IO_I2C_CMD) {
        user_i2c_clk = twi_get_clock();
    }

    switch (conf) {
        case IO_UNINITIALIZED: break;

        case IO_SPI:
            pio_configure(PIOA, PIO_PERIPH_A,
                    (PIO_PA12A_MISO | PIO_PA13A_MOSI | PIO_PA14A_SPCK),
                    PIO_OPENDRAIN | PIO_PULLUP);

            pio_configure(PIOB, PIO_PERIPH_A, PIO_PB14A_NPCS1,
                    PIO_OPENDRAIN | PIO_PULLUP);
            break;

        case IO_I2C_CMD: /* fall through */
        case IO_I2C_LCD:
            twi_set_clock(conf == IO_I2C_LCD ? LCD_I2C_CLOCK : user_i2c_clk);

            pio_configure(PIOA, PIO_PERIPH_A,
                (PIO_PA3A_TWD0 | PIO_PA4A_TWCK0), PIO_OPENDRAIN | PIO_PULLUP);
            break;

        case IO_UART:
            pio_configure(PIOA, PIO_PERIPH_A, (PIO_PA9A_URXD0 | PIO_PA10A_UTXD0), PIO_DEFAULT);
            break;

        case IO_ADC:
            pio_configure(PIOA, PIO_INPUT, (PIO_PA20X1_AD3 | PIO_PA22X1_AD9), PIO_DEFAULT);
            break;

        case IO_DAC:
            pio_configure(PIOB, PIO_INPUT, (PIO_PB13X1_DAC0 | PIO_PB14X1_DAC1), PIO_DEFAULT);
            break;
    }

    current_conf = conf;
}
