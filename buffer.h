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

#ifndef BUFFER_H
#define BUFFER_H

#define BUFFER_SIZE (64 * 1024)

/* Large buffer that might be shared between different applications
 * (not at the same time obviously!) */
typedef union {
    uint32_t u32[BUFFER_SIZE / 4];
    uint16_t u16[BUFFER_SIZE / 2];
    uint8_t u8[BUFFER_SIZE];
} buffer_t;

extern buffer_t buffer;

#endif /* BUFFER_H */
