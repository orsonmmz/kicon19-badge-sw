/*
 * Copyright (c) 2019 Katarzyna Stachyra <kas.stachyra@gmail.com>
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

#ifndef SCOPE_H_
#define SCOPE_H_

#include <stdint.h>
#include <adc.h>

/** Tracking Time*/
#define TRACKING_TIME			1
/** Transfer Period */
#define TRANSFER_PERIOD			1
#define STARTUP_TIME			3
/** Sample & Hold Time */
#define SAMPLE_HOLD_TIME		6
/** Total number of ADC channels in use */
#define NUM_CHANNELS			2
/** Size of the receive buffer and transmit buffer. */
#define SCOPE_BUFFER_SIZE			NUM_CHANNELS*LCD_WIDTH*2
/** Reference voltage for ADC, in mv. */
#define VOLT_REF				3300
/** Maximum number of counts */
#define MAX_DIGITAL				4095
/** Number of pixels per count - to convert raw adc to number of pixels on the display*/
#define RESOLUTION(PAGES)		(PAGES*8-1)/MAX_DIGITAL //optimize the pixels conversion?
/** Number of mV per count - to convert raw adc to voltage */
#define V_RESOLUTION			VOLT_REF/MAX_DIGITAL

struct adc_ch
{
	enum adc_channel_num_t channel;
	uint8_t offset_pages;
	uint8_t offset_pixels;
	uint16_t *buffer;
	uint16_t *draw_buffer;
        uint32_t threshold;
};


void scope_configure(enum adc_channel_num_t *adc_ch, uint32_t ul_size, uint32_t fsampling);
void scope_draw(void);

#endif /* SCOPE_H_ */
