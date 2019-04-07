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

#ifndef IO_CAPTURE_H
#define IO_CAPTURE_H

#include <stdint.h>

/* Available sampling frequencies */
typedef enum { F50MHZ, F40MHZ, F32MHZ, F25MHZ, F20MHZ, F16MHZ, F12_5MHZ,
    F10MHZ, F8MHZ, F6MHZ, F5MHZ, F4MHZ, F3MHZ, F2MHZ, F1MHZ, F500KHZ,
    F250KHZ, F125KHZ } clock_freq_t;

/* Structure to define an acquisition buffer */
typedef struct {
    uint8_t *addr;  ///< Buffer address
    uint16_t size;  ///< Buffer size (bytes)
    int last;       ///< Will stop acquisition after this buffer when enabled
} ioc_buffer_t;

/**
 * Initializes the I/O capture module */
void ioc_init(void);

/**
 * Configures the sampling clock.
 * @param freq is the requested sampling clock frequency.
 */
void ioc_set_clock(clock_freq_t freq);

/**
 * Starts the acquisition.
 *
 * Buffers will be switched in a circular mode until one of them has 'last'
 * field set to 1.
 * @param buffers is an array of acquisition buffers.
 * @param count is the array size (number of acquisition buffers)
 */
void ioc_start(ioc_buffer_t *buffers, int count);

/**
 * Returns 1 when acquisition is in progress, 0 otherwise.
 */
int  ioc_busy(void);

/**
 * Sets a handler which will be called every time a buffer is acquired.
 * The handler will be called with the acquired buffer index.
 */
void ioc_set_handler(void (*func)(int));

#endif /* IO_CAPTURE_H */
