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

#ifndef LOGIC_ANALYZER_H
#define LOGIC_ANALYZER_H

#include <stdint.h>

#define LA_CHANNELS     8

///> Logic analyzer target (LCD display or SUMP protocol over USB)
typedef enum { LA_NONE, LA_LCD, LA_USB } la_target_t;

void la_init(void);
void la_trigger(void);
void la_set_target(la_target_t target);
void la_set_trigger(uint8_t trigger_mask, uint8_t trigger_val);
void la_send_data(void);

#endif /* LOGIC_ANALYZER_H */
