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

#include <sysclk.h>
#include <twi.h>
#include <pio.h>
#include <adc.h>
#include "pdc.h"


static uint16_t us_value[BUFFER_SIZE];

/** number of lcd pages for a channel*/
static uint32_t adc_pages_per_channel;
static uint32_t adc_pixels_per_channel;
static volatile int adc_buffers_rdy=0;
static uint32_t adc_active_channels;
static uint32_t adc_buffer_size;

uint32_t adc0_threshold=45; //into struct? not sure what api is needed
uint32_t adc1_threshold=20;

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
void scope_configure(enum adc_channel_num_t *adc_ch, uint32_t ul_size)
{
	if (ul_size > NUM_CHANNELS) return;

	/* Disable PDC channel interrupt. */
	adc_disable_interrupt(ADC, 0xFFFFFFFF);
	adc_disable_all_channel(ADC);

	/* Initialize variables according to number of channels used */
	if(ul_size == 1)
	{
		adc_active_channels = 1;
		adc_buffer_size = BUFFER_SIZE/2;
		adc_pages_per_channel = LCD_PAGES;
		adc_pixels_per_channel = adc_pages_per_channel * LCD_WIDTH;

		adc_channels[0].channel = adc_ch[0];
		adc_channels[0].offset_pages = 0;
		adc_channels[0].offset_pixels = adc_channels[0].offset_pages*LCD_PAGE_SIZE;
		adc_channels[0].draw_buffer = adc_channels[0].buffer;
	}else
	{
		adc_active_channels = 2;
		adc_buffer_size = BUFFER_SIZE;
		adc_pages_per_channel = LCD_PAGES/2;
		adc_pixels_per_channel = adc_pages_per_channel * LCD_WIDTH;

		adc_channels[0].channel = adc_ch[0];
		adc_channels[0].offset_pages = 4;
		adc_channels[0].offset_pixels = adc_channels[0].offset_pages*LCD_PAGE_SIZE;
		adc_channels[0].draw_buffer = adc_channels[0].buffer;

		adc_channels[1].channel = adc_ch[1];
		adc_channels[1].offset_pages=0;
		adc_channels[1].offset_pixels = adc_channels[1].offset_pages*LCD_PAGE_SIZE;
		adc_channels[1].draw_buffer = adc_channels[1].buffer;
	}

	/* Formula:
	 * ADCClock = MCK / ( (PRESCAL+1) * 2 )
	 * For example, MCK = 64MHZ, PRESCAL = 4, then:
	 * ADCClock = 64 / ((4+1) * 2) = 6.4MHz;
	 *
	 * Formula:
	 *     Startup  Time = startup value / ADCClock
	 *     Startup time = 64 / 100kHz
	 *     here 64/100kHz
	 */
	adc_init(ADC, sysclk_get_cpu_hz(), 1000, ADC_STARTUP_TIME_4);

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
 * \brief Initializes the clocks, pio and configures ADC, interrupts and PDC transfer.
 *
 * \param adc_ch The pointer of channels names array.
 * \param ul_size Number of channels.
 */
void scope_initialize(enum adc_channel_num_t *adc_ch, uint32_t ul_size)
{
    pmc_enable_periph_clk(ID_ADC);
    pio_configure(PIOA, PIO_INPUT, PIO_PA20X1_AD3, PIO_DEFAULT);
    pio_configure(PIOA, PIO_INPUT, PIO_PA22X1_AD9, PIO_DEFAULT);

    scope_configure(adc_ch, ul_size);
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
		SSD1306_drawBitmapDMA();
		adc_buffers_rdy=0;
    }
}

int osc_buffer_rdy(void)
{
	return adc_buffers_rdy;
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
				if(!adc0_thr_found && adc_channels[0].buffer[adc0_counter]>= adc0_threshold)
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
				if(!adc1_thr_found && adc_channels[1].buffer[adc1_counter]>= adc1_threshold)
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
