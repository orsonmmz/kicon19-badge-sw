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

#include "logic_analyzer.h"
#include "io_capture.h"
#include "lcd.h"
#include <string.h>

// Samples buffer
#define MAX_LA_BUF_SIZE     128
static uint8_t la_buffer[MAX_LA_BUF_SIZE];
static uint32_t la_acq_size;
static uint8_t la_chan_enabled;

// Acquisition finished handler
static void la_acq_finished(void* param);

void la_init(void) {
    la_acq_size = MAX_LA_BUF_SIZE;
    la_chan_enabled = 0xFF;
    ioc_set_clock(F1MHZ);
    ioc_set_handler(la_acq_finished, la_buffer);
}


void la_trigger(void) {
    /*memset(la_buffer, 0x00, sizeof(la_buffer));*/
    while(ioc_busy());
    ioc_fetch(la_buffer, la_acq_size);
}


static void la_acq_finished(void* param) {
    (void) param;
    //uint8_t* data = (uint8_t*)(param);

    // double buffering
    uint8_t lcd_page[LCD_WIDTH], lcd_page2[LCD_WIDTH];
    uint8_t chan_mask;

    // Display the acquisition on the LCD
    for(int chan = 0; chan < LA_CHANNELS; ++chan) {
        uint8_t* buf_ptr = la_buffer;
        chan_mask = (1 << chan);

        if ((la_chan_enabled & chan_mask) == 0)
            continue;   // channel disabled

        #define CURRENT_PAGE ((chan & 0x01) ? lcd_page : lcd_page2)
        uint8_t* page_ptr = CURRENT_PAGE;

        for(int x = 0; x < LCD_WIDTH; ++x) {
            *page_ptr = (*buf_ptr & chan_mask) ? 0x02 : 0x80;
            ++page_ptr;
            ++buf_ptr;
        }

        while(SSD1306_isBusy());
        SSD1306_drawPage(chan, CURRENT_PAGE);
    }
}
