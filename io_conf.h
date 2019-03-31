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

#ifndef IO_CONF_H
#define IO_CONF_H

typedef enum {
    IO_UNINITIALIZED,
    IO_SPI,
    IO_I2C,
    IO_UART,
    IO_ADC,
    IO_DAC
} io_config_t;

/**
 * Reconfigures J2 header pins to a certain function.
 */
void io_configure(io_config_t conf);

#endif /* IO_CONF_H */
