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

#ifndef SUMP_H
#define SUMP_H

typedef enum {
/* SUMP short commands (1 byte) */
RESET               = 0x00,
RUN                 = 0x01,
ID                  = 0x02,
METADATA            = 0x04,
XON                 = 0x11,
XOFF                = 0x13,

/* SUMP short commands (5 bytes) */
SET_DIV             = 0x80,
SET_READ_DLY_CNT    = 0x81,
SET_FLAGS           = 0x82,
SET_DELAY_COUNT     = 0x83,
SET_READ_COUNT      = 0x84,
SET_TRG_MASK        = 0xc0,
SET_TRG_VAL         = 0xc1,
SET_TRG_CFG         = 0xc2,
SET_TRG_MASK2       = 0xc4,
SET_TRG_VAL2        = 0xc5,
SET_TRG_CFG2        = 0xc6,
SET_TRG_MASK3       = 0xc8,
SET_TRG_VAL3        = 0xc9,
SET_TRG_CFG3        = 0xca,
SET_TRG_MASK4       = 0xcc,
SET_TRG_VAL4        = 0xcd,
SET_TRG_CFG4        = 0xce,
} sump_cmd_t;

#endif /* SUMP_H */
