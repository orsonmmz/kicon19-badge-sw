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

#ifndef BUTTONS_H
#define BUTTONS_H

typedef enum {
    BUT1 = 0x01,
    BUT2 = 0x02,
    BUT3 = 0x04,
    BUT4 = 0x08
} button_t;

#define BUT_UP      BUT4
#define BUT_DOWN    BUT3
#define BUT_LEFT    BUT2
#define BUT_RIGHT   BUT1

void btn_init(void);
int btn_state(void);
int btn_is_pressed(button_t button);

#endif /* BUTTONS_H */
