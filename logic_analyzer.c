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
#include "buttons.h"
#include "udi_cdc.h"
#include "commands.h"
#include "command_handlers.h"
#include "sump.h"
#include "apps_list.h"
#include "settings_list.h"
#include "buffer.h"
#include <string.h>
#include <limits.h>

#define LA_CHANNELS 8

// Samples buffer
#define LA_BUFFER_SIZE     (BUFFER_SIZE)
static uint8_t * const la_buffer = buffer.u8;
static uint8_t * const la_buffer_last = &buffer.u8[LA_BUFFER_SIZE - 1];

// Enabled channels
static uint8_t la_chan_enabled;

// Acquisition settings
static uint8_t la_trigger_mask = 0;
static uint8_t la_trigger_val = 0;
static uint32_t la_read_cnt = 0;
static uint32_t la_delay_cnt = 0;

// Detected trigger offset (UINT_MAX when not detected)
static volatile uint32_t la_trig_offset = UINT_MAX;

// Acquisition finished handler
static void la_acq_finished(int buf_idx);

static volatile enum { IDLE, RUNNING, ACQUIRED } la_state = IDLE;

// Subbuffers used byt the I/O capture routines
#define LA_IOC_BUFFERS_CNT  4
static ioc_buffer_t la_ioc_buffers[LA_IOC_BUFFERS_CNT];

// Fixes the hardware channel order
// (see the connection between the logic probes pin header and the input buffer)
#define LA_FIX_ORDER(val) ((val & 0x0f) \
            | (val & 0x80) >> 3 \
            | (val & 0x40) >> 1 \
            | (val & 0x20) << 1 \
            | (val & 0x10) << 3)

static void la_fix_channels(uint32_t offset, uint32_t size) {
    uint8_t* buf_ptr = &la_buffer[offset];

    for(unsigned int i = 0; i < size; ++i) {
        *buf_ptr = LA_FIX_ORDER(*buf_ptr);

        // Buffer wrapping
        if (++buf_ptr > la_buffer_last) {
            buf_ptr = la_buffer;
        }
    }
}


// Searches the acquisition buffer for a sample matching the configured
// trigger. Will return the sample index or UINT_MAX if nothing found.
// This function works with samples which do not have the order fixed
// (see LA_FIX_ORDER macro)
static uint32_t la_find_trigger_unfixed(const uint8_t *buf, uint32_t size) {
    if (la_trigger_mask == 0) {
        return 0;
    }

    const uint8_t mask = LA_FIX_ORDER(la_trigger_mask);
    const uint8_t val = LA_FIX_ORDER(la_trigger_val);
    const uint8_t *buf_ptr = buf;

    for (uint32_t i = 0; i < size; ++i) {
        if ((*buf_ptr & mask) == val) {
            return i;
        }

        ++buf_ptr;
    }

    return UINT_MAX;    // trigger not matched
}


void la_init(void) {
    la_chan_enabled = 0xFF;
    ioc_set_clock(F1MHZ);
    ioc_set_handler(la_acq_finished);
}


static void la_start_acq(void) {
    if (la_state != IDLE)
        return;

    la_trig_offset = UINT_MAX;

    /* Triggers require splitting the acquisition to chunks,
       to be able to seek for the trigger while next samples
       are acquired (kind of double buffering) */
    uint32_t buf_size = min(la_read_cnt * LA_IOC_BUFFERS_CNT,
            LA_BUFFER_SIZE / LA_IOC_BUFFERS_CNT);

    for (int i = 0; i < LA_IOC_BUFFERS_CNT; ++i) {
        la_ioc_buffers[i].addr = la_buffer + i * buf_size;
        la_ioc_buffers[i].size = buf_size;
        la_ioc_buffers[i].last = 0;
    }

    if (la_trigger_mask == 0) {
        /* No triggers configured, it is a single-run acquisition */
        la_ioc_buffers[LA_IOC_BUFFERS_CNT - 1].last = 1;
    }

    la_state = RUNNING;
    ioc_start(la_ioc_buffers, LA_IOC_BUFFERS_CNT);
}


static void la_display_acq(uint32_t offset) {
    // double buffering
    uint8_t lcd_page[LCD_WIDTH], lcd_page2[LCD_WIDTH];
    uint8_t chan_mask;

    // Display the acquisition on the LCD
    for(int chan = 0; chan < LA_CHANNELS; ++chan) {
        uint8_t* buf_ptr = &la_buffer[offset];
        chan_mask = (1 << chan);

        if ((la_chan_enabled & chan_mask) == 0)
            continue;   // channel disabled

        #define CURRENT_PAGE ((chan & 0x01) ? lcd_page : lcd_page2)
        uint8_t* page_ptr = CURRENT_PAGE;

        for(int x = 0; x < LCD_WIDTH; ++x) {
            *page_ptr = (*buf_ptr & chan_mask) ? 0x02 : 0x80;
            ++page_ptr;

            // Buffer wrapping
            if (++buf_ptr > la_buffer_last) {
                buf_ptr = la_buffer;
            }
        }

        while(SSD1306_isBusy());
        SSD1306_drawPage(chan, CURRENT_PAGE);
    }
}


// Finds the closest available clock frequency
// basing a 100 MHz prescaler (default OLS clock frequency)
static clock_freq_t la_get_clock(int prescaler_100M) {
    struct {
        clock_freq_t clock_freq;
        int max_prescaler;
    } freqs[] = {
        { F50MHZ,   1 },
        { F32MHZ,   2 },
        { F25MHZ,   3 },
        { F20MHZ,   4 },
        { F16MHZ,   5 },
        { F12_5MHZ, 7 },
        { F10MHZ,   10 },
        { F8MHZ,    13 },
        { F6MHZ,    17 },
        { F5MHZ,    21 },
        { F4MHZ,    27 },
        { F3MHZ,    39 },
        { F2MHZ,    65 },
        { F1MHZ,    132 },
        { F500KHZ,  265 },
        { F250KHZ,  532 },
    };

    for(unsigned int i = 0; i < sizeof(freqs); ++i) {
        if (freqs[i].max_prescaler < prescaler_100M)
            continue;

        return freqs[i].clock_freq;
    }

    // the slowest available sampling frequency
    return F125KHZ;
}


/* ID command response */
static const uint8_t SUMP_ID_RESP[] = "1ALS";
/* Device metadata */
static const uint8_t SUMP_METADATA_RESP[] =
/*  token  value */
    "\x01" "KiCon-Badge\x00"   // device name
    "\x20" "\x00\x00\x00\x08"  // number of channels
    "\x21" "\x00\x01\x00\x00"  // sample memory available [bytes] = 65536
    "\x23" "\x02\xfa\xf0\x80"  // maximum sampling rate [Hz] = 50 MHz
    "\x24" "\x00\x00\x00\x00"  // protocol version
    ;


int cmd_sump(const uint8_t* cmd, unsigned int len)
{
    if (len == 1) {
        switch (cmd[0]) {
            case METADATA:
                cmd_response(SUMP_METADATA_RESP, sizeof(SUMP_METADATA_RESP) - 1);
                break;

            case RUN:
                la_start_acq();
                break;

            case ID:
                cmd_response(SUMP_ID_RESP, sizeof(SUMP_ID_RESP) - 1);
                break;

            case XON: break;
            case XOFF: break;
            case RESET: break;
            default: return 0;   // unknown 1-byte command, perhaps incomplete
        }

        return 1;   // default handles unknown commands

    } else if (len >= 5) {
        // change the endianess
        unsigned int arg = (cmd[4] << 24) | (cmd[3] << 16) | (cmd[2] << 8) | cmd[1];

        switch (cmd[0]) {
            case SET_TRG_MASK:
                la_trigger_mask = (uint8_t)(arg & 0xff);
                break;

            case SET_TRG_VAL:
                la_trigger_val = (uint8_t)(arg & 0xff);
                break;

            case SET_TRG_CFG:
                break;

            case SET_DIV:
                ioc_set_clock(la_get_clock(arg));
                break;

            case SET_READ_DLY_CNT:
                la_read_cnt = (uint16_t)((arg & 0xffff) + 1) * 4;
                la_delay_cnt = (uint16_t)((arg >> 16) + 1) * 4;
                break;

            case SET_DELAY_COUNT:
                la_delay_cnt = arg;
                break;

            case SET_READ_COUNT:
                la_read_cnt = arg;
                break;

            // none of the flags has any meaning in this implementation
            //case SET_FLAGS: break;
        }

        // clear the buffer anyway, commands cannot be longer than 5 bytes
        return 1;
    }

    return 0;
}


/* Sends data in reverse order */
static void cdc_write_buf_reverted(const uint8_t* data, uint32_t size) {
    const uint8_t* ptr = &data[size];

    while (ptr > data) {
        --ptr;
        udi_cdc_putc(*ptr);
    }
}


static void la_usb_send(uint32_t offset, uint32_t size) {
    if (offset + size <= LA_BUFFER_SIZE) {
        cdc_write_buf_reverted(&la_buffer[offset], size);
    } else {
        unsigned int firstChunk = LA_BUFFER_SIZE - offset;
        unsigned int secondChunk = size - firstChunk;
        cdc_write_buf_reverted(la_buffer, secondChunk);
        cdc_write_buf_reverted(&la_buffer[offset], firstChunk);
    }
}


static void la_acq_finished(int buf_idx) {
    /* still waiting for the trigger */
    if (la_trig_offset == UINT_MAX) {
        uint8_t *buf_addr = la_ioc_buffers[buf_idx].addr;
        uint16_t buf_size = la_ioc_buffers[buf_idx].size;

        la_trig_offset = la_find_trigger_unfixed(buf_addr, buf_size);

        if (la_trig_offset != UINT_MAX) { /* trigger has been detected */
            /* get the last samples and stop the acquisition when it reaches
             * the current buffer
             * (needed only for acquisitions with configured trigger) */
            if (la_trigger_mask) {
                la_ioc_buffers[buf_idx].size = la_trig_offset;
                la_ioc_buffers[buf_idx].last = 1;

                /* save the trigger offset with regard to the whole buffer */
                la_trig_offset += (buf_addr - la_buffer);
            }
        }
    } else {
        /* was it the last acquisition? */
        if (la_ioc_buffers[buf_idx].last) {
            la_state = ACQUIRED;
        }
    }
}



void app_la_usb_func(void) {
    const uint8_t *resp;
    unsigned int resp_len;
    int processed;

    la_trigger_mask = 0;
    la_trigger_val = 0;
    la_read_cnt = 0;
    la_trig_offset = UINT_MAX;
    la_state = IDLE;

    cmd_set_mode(CMD_SUMP);

    while(SSD1306_isBusy());
    SSD1306_clearBufferFull();
    SSD1306_setString(5, 3, "Logic Analyzer (USB)", 20, WHITE);
    SSD1306_drawBufferDMA();

    la_trig_offset = UINT_MAX;

    while(btn_state() != BUT_LEFT) {
        /* process commands */
        if (udi_cdc_is_rx_ready()) {
            cmd_new_data(udi_cdc_getc());
            processed = cmd_try_execute();

            if (processed) {
                cmd_get_resp(&resp, &resp_len);

                if(resp && resp_len > 0) {
                    udi_cdc_write_buf(resp, resp_len);
                    cmd_resp_processed();
                }
            }
        }


        /* send samples when the acquisition is over */
        if (la_state == ACQUIRED) {
            la_fix_channels(la_trig_offset, la_read_cnt);
            la_usb_send(la_trig_offset, la_read_cnt);
            la_state = IDLE;
        }
    }

    while(btn_state());    /* wait for the button release */
}


void app_la_lcd_func(void) {
    /* configure the logic analyzer */
    switch (menu_la_lcd_sampling_freq.val) {
        case 0: ioc_set_clock(F10MHZ); break;
        case 1: ioc_set_clock(F5MHZ); break;
        case 2: ioc_set_clock(F2MHZ); break;
        case 3: ioc_set_clock(F1MHZ); break;
        case 4: ioc_set_clock(F500KHZ); break;
    }

    if (menu_la_lcd_trigger_input.val == 0) {
        /* no trigger input -> free-running mode */
        la_trigger_mask = 0;
    } else {
        la_trigger_mask = (1 << (menu_la_lcd_trigger_input.val - 1));

        SSD1306_clearBufferFull();
        SSD1306_setString(6, 0, "Waiting for trigger", 19, WHITE);
        SSD1306_drawBufferDMA();
    }

    la_trigger_val = menu_la_lcd_trigger_level.val ? la_trigger_mask : 0;
    la_read_cnt = LCD_WIDTH;        /* number of requested samples */

    la_state = IDLE;
    la_start_acq();

    while(btn_state() != BUT_LEFT) {
        if (la_state == RUNNING && !ioc_busy()) {
            /* strange.. */
            la_state = IDLE;
            la_start_acq();
        }

        if (la_state == ACQUIRED) {
            /* got all the samples, draw the results */
            la_fix_channels(la_trig_offset, la_read_cnt);
            la_display_acq(la_trig_offset);
            la_state = IDLE;
            la_start_acq();
        }
    }

    while(btn_state());    /* wait for the button release */
}
