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

#include "asf.h"
#include "buttons.h"

#define BUTTON_ENABLE_INT

#define BUT1_IDX    PIO_PA1_IDX
#define BUT2_IDX    PIO_PA0_IDX
#define BUT3_IDX    PIO_PB0_IDX
#define BUT4_IDX    PIO_PB1_IDX

#ifdef BUTTON_ENABLE_INT
  #define BUTTON_PIO_ATTR (PIO_INPUT | PIO_PULLUP | PIO_DEBOUNCE | PIO_IT_LOW_LEVEL)
#else
  #define BUTTON_PIO_ATTR (PIO_INPUT | PIO_PULLUP | PIO_DEBOUNCE)
#endif



void btn_init(void)
{
    pio_configure_pin(BUT1_IDX, BUTTON_PIO_ATTR);
    pio_configure_pin(BUT2_IDX, BUTTON_PIO_ATTR);
    pio_configure_pin(BUT3_IDX, BUTTON_PIO_ATTR);
    pio_configure_pin(BUT4_IDX, BUTTON_PIO_ATTR);
}

int btn_state(void)
{
    int ret = 0;

    if(btn_is_pressed(BUT1))
        ret |= BUT1;

    if(btn_is_pressed(BUT2))
        ret |= BUT2;

    if(btn_is_pressed(BUT3))
        ret |= BUT3;

    if(btn_is_pressed(BUT4))
        ret |= BUT4;

    return ret;
}

int btn_is_pressed(button_t button)
{
    switch(button) {
        case BUT1: return !pio_get_pin_value(BUT1_IDX);
        case BUT2: return !pio_get_pin_value(BUT2_IDX);
        case BUT3: return !pio_get_pin_value(BUT3_IDX);
        case BUT4: return !pio_get_pin_value(BUT4_IDX);
    }

    return 0;
}
