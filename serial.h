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

#ifndef SERIAL_H
#define SERIAL_H

/**
 * @brief Initialize the serial port (UART0).
 * @param baud is the requested baud rate.
 */
void serial_init(unsigned int baud);

/**
 * @brief Send a single character over the serial port.
 * @param c is the character to be sent.
 */
void serial_putc(char c);

/**
 * @brief Send a null-terminated string over the serial port.
 * @param str is the string to be sent.
 */
void serial_puts(const char *str);

/**
 * @brief Send a null-terminated string over the serial port.
 * @param str is the string to be sent.
 * @param count is the string length.
 */
void serial_putsn(const char *str, unsigned int count);

#endif /* SERIAL_H */
