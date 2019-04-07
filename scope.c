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

#include "lcd.h"
#include "scope.h"
#include "buttons.h"
#include "apps_list.h"
#include "settings_list.h"
#include "io_conf.h"
#include "buffer.h"

#include <sysclk.h>
#include <twi.h>
#include <pio.h>
#include <adc.h>
#include "pdc.h"

/* The general buffer is divided in the following way:
 * start                end                     description
 * 0                    SCOPE_BUFFER_SIZE/2-1   adc_ch[0].buffer
 * SCOPE_BUFFER_SIZE/2  SCOPE_BUFFER_SIZE-1     adc_ch[1].buffer
 * SCOPE_BUFFER         SCOPE_BUFFER_SIZE*2     us_value
 */


static uint16_t * const us_value = &buffer.u16[SCOPE_BUFFER_SIZE];

/** number of lcd pages for a channel*/
static uint32_t adc_pages_per_channel;
static uint32_t adc_pixels_per_channel;
static volatile int adc_buffers_rdy=0;
static uint32_t adc_active_channels;
static uint32_t adc_buffer_size;

static struct adc_ch adc_channels[2];

/**
 * \brief Read converted data through PDC channel.
 *
 * \param p_adc The pointer of adc peripheral.
 * \param p_s_buffer The destination buffer.
 * \param ul_size The size of the buffer.
 */
static uint32_t adc_read_buffer(Adc * p_adc, uint16_t * p_s_buffer, uint32_t ul_size)
{
	/* Check if the first PDC bank is free. */
	if ((p_adc->ADC_RCR == 0) && (p_adc->ADC_RNCR == 0)) {
		p_adc->ADC_RPR = (uint32_t) p_s_buffer;
		p_adc->ADC_RCR = ul_size;
		p_adc->ADC_PTCR = ADC_PTCR_RXTEN;

		return 1;
	} else { /* Check if the second PDC bank is free. */
		if (p_adc->ADC_RNCR == 0) {
			p_adc->ADC_RNPR = (uint32_t) p_s_buffer;
			p_adc->ADC_RNCR = ul_size;

			return 1;
		} else {
			return 0;
		}
	}
}

/**
 * \brief configures ADC, interrupts and PDC transfer.
 *
 * \param adc_ch The pointer of channels names array.
 * \param ul_size Number of channels.
 */
void scope_configure(enum adc_channel_num_t *adc_ch, uint32_t ul_size, uint32_t fsampling)
{
	if (ul_size > NUM_CHANNELS) return;

	pmc_enable_periph_clk(ID_ADC);
	pio_configure(PIOA, PIO_INPUT, (PIO_PA20X1_AD3 | PIO_PA22X1_AD9), PIO_DEFAULT);

	/* Disable PDC channel interrupt. */
	adc_disable_interrupt(ADC, 0xFFFFFFFF);
	adc_disable_all_channel(ADC);

	/* Initialize variables according to number of channels used */
	if(ul_size == 1)
	{
		adc_active_channels = 1;
		adc_buffer_size = SCOPE_BUFFER_SIZE/2;
		adc_pages_per_channel = LCD_PAGES;
		adc_pixels_per_channel = adc_pages_per_channel * LCD_WIDTH;

		adc_channels[0].channel = adc_ch[0];
		adc_channels[0].offset_pages = 0;
		adc_channels[0].offset_pixels = adc_channels[0].offset_pages*LCD_PAGE_SIZE;
                adc_channels[0].buffer = &buffer.u16[0];
		adc_channels[0].draw_buffer = adc_channels[0].buffer;
                adc_channels[0].threshold = 32;   /* middle of ADC range */
	}else
	{
		adc_active_channels = 2;
		adc_buffer_size = SCOPE_BUFFER_SIZE;
		adc_pages_per_channel = LCD_PAGES/2;
		adc_pixels_per_channel = adc_pages_per_channel * LCD_WIDTH;

		adc_channels[0].channel = adc_ch[0];
		adc_channels[0].offset_pages = 4;
		adc_channels[0].offset_pixels = adc_channels[0].offset_pages*LCD_PAGE_SIZE;
                adc_channels[0].buffer = &buffer.u16[0];
		adc_channels[0].draw_buffer = adc_channels[0].buffer;
                adc_channels[0].threshold = 32;   /* middle of ADC range */

		adc_channels[1].channel = adc_ch[1];
		adc_channels[1].offset_pages=0;
		adc_channels[1].offset_pixels = adc_channels[1].offset_pages*LCD_PAGE_SIZE;
                adc_channels[1].buffer = &buffer.u16[SCOPE_BUFFER_SIZE/2];
		adc_channels[1].draw_buffer = adc_channels[1].buffer;
                adc_channels[1].threshold = 32;   /* middle of ADC range */
	}

        /* one sample takes 20 ADC clock periods */
	adc_init(ADC, sysclk_get_cpu_hz(), fsampling * 20, ADC_STARTUP_TIME_4);

	/* Formula:
	 *     Transfer Time = (TRANSFER * 2 + 3) / ADCClock
	 *     Tracking Time = (TRACKTIM + 1) / ADCClock
	 *     Settling Time = settling value / ADCClock
	 *
	 *     Transfer Time = (1 * 2 + 3) / 100kHz
	 *     Tracking Time = (1 + 1) / 100kHz
	 *     Settling Time = 3 / 100kHz
	 */
	adc_configure_timing(ADC, TRACKING_TIME, ADC_SETTLING_TIME_3, TRANSFER_PERIOD);

	/* Enable channel number tag. */
	adc_enable_tag(ADC);

	NVIC_EnableIRQ(ADC_IRQn);

	/* Enable channels. */
	for(uint32_t i = 0; i < ul_size; i++)
	{
		adc_enable_channel(ADC, adc_channels[i].channel);
	}

	adc_configure_trigger(ADC, ADC_TRIG_SW, 1);

	adc_start(ADC);

	/* Enable PDC channel interrupt. */
	adc_enable_interrupt(ADC, ADC_IER_RXBUFF);
	/* Start new pdc transfer. */
	adc_read_buffer(ADC, us_value, adc_buffer_size);
}


/**
 * \brief Displays the acquired data on the lcd
 */
void scope_draw(void)
{
	/*Checks whether new data ready and that display is not busy*/
    if(adc_buffers_rdy && !SSD1306_isBusy() )
    {
    	for(uint32_t chan_cnt=0; chan_cnt<adc_active_channels; chan_cnt++)
    	{

			/*Clears the display*/
    		SSD1306_clearBuffer(0, adc_channels[chan_cnt].offset_pages, BLACK, adc_pixels_per_channel);
			for(int i = 0; i<(LCD_WIDTH); i++)
			{
				SSD1306_setPixel(i, adc_channels[chan_cnt].draw_buffer[i], 1);
			}
    	}
		SSD1306_drawBufferDMA();
		adc_buffers_rdy=0;
    }
}


/**
 * \brief Interrupt handler for the ADC.
 */
void ADC_Handler(void)
{
	uint32_t i;
	uint8_t uc_ch_num;
	uint32_t adc0_counter;
	uint32_t adc1_counter;

	uint32_t adc0_thr_found;
	uint32_t adc1_thr_found;

	uint32_t adc0_thr_index;
	uint32_t adc1_thr_index;

	if ((adc_get_status(ADC) & ADC_ISR_RXBUFF) == ADC_ISR_RXBUFF)
	{
		/* If previous buffer not processed yet - leave */
		if(adc_buffers_rdy)
		{
			/* Start new pdc transfer */
			adc_read_buffer(ADC, us_value, adc_buffer_size);
			return;
		}

		adc0_counter=0;
		adc1_counter=0;

		adc0_thr_found=0;
		adc1_thr_found=0;

		adc0_thr_index=0;
		adc1_thr_index=0;

		adc_channels[0].draw_buffer = adc_channels[0].buffer;
		adc_channels[1].draw_buffer = adc_channels[1].buffer;

		for (i = 0; i < adc_buffer_size; i++)
		{
			/* 4MSB of ADC readout is channel number. Retrieve it */
			uc_ch_num = (us_value[i] & ADC_LCDR_CHNB_Msk) >> ADC_LCDR_CHNB_Pos;

			/* Compare the tag with a channel and put the value to the respective channel buffer*/
			if (adc_channels[0].channel == uc_ch_num)
			{
				/* Check if enough samples to display*/
				if ((adc0_counter-adc0_thr_index)>=LCD_WIDTH)
					continue;

				/* Remove the tag from the ADC readout and convert it to number of pixels*/
				adc_channels[0].buffer[adc0_counter]=(us_value[i] & ADC_LCDR_LDATA_Msk)*RESOLUTION(adc_pages_per_channel) + adc_channels[0].offset_pixels;

				/* Check if sample is above threshold - start to draw from here*/
				if(!adc0_thr_found && adc_channels[0].buffer[adc0_counter]>= adc_channels[0].threshold)
				{
					adc_channels[0].draw_buffer = adc_channels[0].buffer + adc0_counter;
					adc0_thr_index = adc0_counter;
					adc0_thr_found = 1;
				}
				adc0_counter++;
				continue;
			}
			if (adc_channels[1].channel == uc_ch_num)
			{
				/* Check if enough samples to display*/
				 if ((adc1_counter-adc1_thr_index)>=LCD_WIDTH)
					 continue;

				/*Remove the tag from the ADC readout and convert it to number of pixels*/
				adc_channels[1].buffer[adc1_counter]=(us_value[i] & ADC_LCDR_LDATA_Msk)*RESOLUTION(adc_pages_per_channel) + adc_channels[1].offset_pixels;

				/* Check if sample is above threshold - start to draw from here*/
				if(!adc1_thr_found && adc_channels[1].buffer[adc1_counter]>= adc_channels[1].threshold)
				{
					adc_channels[1].draw_buffer = adc_channels[1].buffer + adc1_counter;
					adc1_thr_index = adc1_counter;
					adc1_thr_found = 1;
				}
				adc1_counter++;
				continue;
			}
			break;
		}

		adc_buffers_rdy = 1;

		/* Start new pdc transfer */
		adc_read_buffer(ADC, us_value, adc_buffer_size);
	}
}


void app_scope_func(void)
{
    int chan_count = 0;
    enum adc_channel_num_t adc_chans[2];
    uint32_t fsampling;

    io_configure(IO_ADC);

    switch (menu_scope_channels.val) {
        case 0: /* channel 1 */
            chan_count = 1;
            adc_chans[0] = ADC_CHANNEL_3;
            break;
        case 1: /* channel 2 */
            chan_count = 1;
            adc_chans[0] = ADC_CHANNEL_9;
            break;
        case 2: /* channel 1&2 */
            chan_count = 2;
            adc_chans[0] = ADC_CHANNEL_3;
            adc_chans[1] = ADC_CHANNEL_9;
            break;
    }

    switch (menu_scope_fsampling.val) {
        default: /* fall-through */
        case 0: fsampling = 1000000; break;
        case 1: fsampling = 500000; break;
        case 2: fsampling = 200000; break;
        case 3: fsampling = 100000; break;
        case 4: fsampling = 50000; break;
    }

    scope_configure(adc_chans, chan_count, fsampling);

#if 0
    /* for some reason, the code below does not affect the acquired data */
    for (int i = 0; i < chan_count; ++i) {
        /* adding 1 to the value selected in the gain menu
         * corresponds to the selected gain value
         * (menu 0 => gain x1, menu 1 => gain x2, menu 2 => gain x4) */
        adc_set_channel_input_gain(ADC, adc_chans[i], menu_scope_gain.val + 1);
        adc_disable_channel_input_offset(ADC, adc_chans[i]);
    }
#endif

    while(btn_state() != BUT_LEFT) {
        scope_draw();
    }

    while(btn_state());    /* wait for the button release */
}
