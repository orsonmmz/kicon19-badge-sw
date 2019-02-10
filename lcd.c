/*
 * Copyright (c) 2019 Maciej Suminski <orson@orson.net.pl>
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

#include <sysclk.h>
#include <twi.h>
#include <pio.h>

#include "SSD1306_commands.h"
#include "fonts.h"
#include "uart.h"
#include "pdc.h"


uint8_t SSD1306_orientation = 0;//not used
uint8_t SSD1306_maxX = 0;		//not used
uint8_t SSD1306_maxY = 0;		//not used

/*
 * Pointer to TWI PDC register base
 */
static Pdc *g_p_twim_pdc;

/*
 * buffer to store image and send to display
 */
static uint8_t displayBuffer[LCD_WIDTH*LCD_PAGES];

static inline uint32_t twi_read_status(Twi *p_twi)
{
	return p_twi->TWI_SR;
}

static int busy = 0;
static int dma_transfers = 0;

/*
 * \brief Write multiple bytes to a TWI compatible slave device.
 *
 * \param p_twi Pointer to a TWI instance.
 * \param p_packet Packet information and data (see \ref twi_packet_t).
 */
static void twi_master_pdc_write(twi_packet_t *p_packet)
{
	pdc_packet_t pdc_twi_packet;
    uint8_t *buffer = p_packet->buffer;

	pdc_disable_transfer(g_p_twim_pdc, PERIPH_PTCR_TXTDIS | PERIPH_PTCR_RXTDIS);

	pdc_twi_packet.ul_addr = (uint32_t)buffer;

    pdc_twi_packet.ul_size = (p_packet->length);

	pdc_tx_init(g_p_twim_pdc, &pdc_twi_packet, NULL);

	/* Set write mode, slave address and 3 internal address byte lengths */
	TWI0->TWI_MMR = 0;
	TWI0->TWI_MMR = TWI_MMR_DADR(p_packet->chip) |
	((p_packet->addr_length << TWI_MMR_IADRSZ_Pos) &
	TWI_MMR_IADRSZ_Msk);

	/* Set internal address for remote chip */
	TWI0->TWI_IADR = 0;
	TWI0->TWI_IADR = twi_mk_addr(p_packet->addr, p_packet->addr_length);

	twi_enable_interrupt(TWI0, TWI_SR_ENDTX);

	/* Enable the TX PDC transfer requests */
	pdc_enable_transfer(g_p_twim_pdc, PERIPH_PTCR_TXTEN);
}

/*
 * Writes data over twi
 * buffer - bytes to send
 * size - number of bytes
 * ctrl_b - indicates whether it is cmd or data
 * DMA - 1 to use DMA for transfer
 */
static void SSD1306_write(uint8_t * buffer,int size, control_byte ctrl_b, int DMA)
{
    twi_packet_t packet_tx;

    packet_tx.chip        = SSD1306_address;

    packet_tx.addr[0] = (uint8_t) ctrl_b;

    packet_tx.addr_length = 1;
    packet_tx.buffer      = (uint8_t *) buffer;
    packet_tx.length      = size;

    if (DMA)
    {
    	twi_master_pdc_write(&packet_tx);
    }else
    {
    	twi_master_write(TWI0, &packet_tx);
    }
}

/*
 * Writes a CMD over twi
 * buffer - bytes to send
 * size - number of bytes
 */
static void SSD1306_writeCmd(uint8_t * buffer, int size)
{
	control_byte ctrl = CMD;
	SSD1306_write(buffer, size, ctrl, 0);
}

/*
 * Writes data over twi
 * buffer - bytes to send
 * size - number of bytes
 */
static void SSD1306_writeData(uint8_t * buffer, int size, int DMA)
{
	control_byte ctrl = DATA;
	SSD1306_write(buffer, size, ctrl, DMA);
}


/*
 * initializes: twi connection, display,
 * fills displayBuffer with 0xff (pixels on)
 * clears the display
 */
void SSD1306_init(void)
{
	uint8_t init[] =
	{
		SSD1306_DISPLAYOFF,					// display off
		SSD1306_SETMULTIPLEX, 0x3F,			// mux ratio
		SSD1306_SETDISPLAYOFFSET, 0x00,		// display offset
		SSD1306_SETSTARTLINE,				// display start line
		SSD1306_SEGREMAP,					// segment re-map
		SSD1306_COMSCANINC, 				// com output scan direction
		SSD1306_SETCOMPINS, 0x12,			// com pins hw configuration
		SSD1306_SETCONTRAST, 0x7F, 			// contrast control
		SSD1306_DISPLAYALLON_RESUME,		// disable entire display on
		SSD1306_NORMALDISPLAY, 				// normal display
		SSD1306_SETDISPLAYCLOCKDIV, 0x80,	// osc frequency
		SSD1306_CHARGEPUMP, 0x14, 			// en charge pump regulator
		SSD1306_DISPLAYON					// display on
//		SSD1306_DISPLAYALLON 				// clear all
	};

	//init twi
    twi_options_t opt;
    opt.master_clk = sysclk_get_peripheral_hz();
    opt.speed      = 200000;    // TODO

    pio_configure(PIOA, PIO_PERIPH_A,
            (PIO_PA3A_TWD0 | PIO_PA4A_TWCK0), PIO_OPENDRAIN | PIO_PULLUP);
    sysclk_enable_peripheral_clock(ID_TWI0);
    pmc_enable_periph_clk(ID_TWI0);

    twi_master_init(TWI0, &opt);

    /* Configure TWI interrupts */
	NVIC_DisableIRQ(TWI0_IRQn);
	NVIC_ClearPendingIRQ(TWI0_IRQn);
	NVIC_SetPriority(TWI0_IRQn, 0);
	NVIC_EnableIRQ(TWI0_IRQn);

	/* Get pointer to TWI master PDC register base */
	g_p_twim_pdc = twi_get_pdc_base(TWI0);

    // TODO twi_probe?

    //init buffer
	uint16_t i;

	//init displayBuffer to all on
	for (i=0; i<LCD_WIDTH*LCD_PAGES; i++)
	{
		displayBuffer[i]=0xff;
	}

    SSD1306_writeCmd(init, sizeof(init));
//    SSD1306_setOrientation(0);
    SSD1306_clear();
}

//not used
void SSD1306_setOrientation(uint8_t orientation)
{

	SSD1306_orientation = orientation % 4;

    switch (SSD1306_orientation) {
    case 0:
    	SSD1306_maxX = LCD_WIDTH;
    	SSD1306_maxY = LCD_HEIGHT;
        break;
    case 1:
    	SSD1306_maxX = LCD_HEIGHT;
    	SSD1306_maxY = LCD_WIDTH;
        break;
    case 2:
    	SSD1306_maxX = LCD_WIDTH;
    	SSD1306_maxY = LCD_HEIGHT;
        break;
    case 3:
    	SSD1306_maxX = LCD_HEIGHT;
    	SSD1306_maxY = LCD_WIDTH;
        break;
    }
}

/*
 * Swaps the values of the variables
 */
void SSD1306_swap(uint8_t *a, uint8_t *b)
{
    uint8_t w = *a;
    *a = *b;
    *b = w;
}

//not used
void SSD1306_orientCoordinates(uint8_t *x1, uint8_t *y1)
{
    switch (SSD1306_orientation) {
    case 0:  // ok
        break;
    case 1: // ok
        *y1 = SSD1306_maxY - *y1 - 1;
        SSD1306_swap(x1, y1);
        break;
    case 2: // ok
        *x1 = SSD1306_maxX - *x1 - 1;
        *y1 = SSD1306_maxY - *y1 - 1;
        break;
    case 3: // ok
        *x1 = SSD1306_maxX - *x1 - 1;
        SSD1306_swap(x1, y1);
        break;
    }
}

/*
 * Functions operating on the display buffer
 * drawing function must be called afterwards!
 */

/*
 * Sets a value of the pixel
 * x - horizontal coordinate
 * y - vertical coordinate
 * color - on/off/invert
 */
void SSD1306_setPixel(uint8_t x, uint8_t y, uint8_t color)
{
	// check if within bounds
	if ((x >= LCD_WIDTH) || (y >= LCD_HEIGHT)) return;

    switch(color)
    {
    	case WHITE:   displayBuffer[x + (y/LCD_PAGE_SIZE)*LCD_WIDTH] |=  (1 << (y&7)); break;
    	case BLACK:   displayBuffer[x + (y/LCD_PAGE_SIZE)*LCD_WIDTH] &= ~(1 << (y&7)); break;
    	case INVERSE: displayBuffer[x + (y/LCD_PAGE_SIZE)*LCD_WIDTH] ^=  (1 << (y&7)); break;
    }
}

/*
 * Sets a line in the buffer
 * x0 - beginning of the line in horizontal direction
 * y0 - beginning of the line in vertical direction
 * x1 - end of the line in horizontal direction
 * y1 - end of the line in vertical direction
 */
void SSD1306_setLine(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1, uint8_t color)
{
	// check if within bounds
	if ((x0 >= LCD_WIDTH) || (y0 >= LCD_HEIGHT) || (x1 >= LCD_WIDTH) || (y1 >= LCD_HEIGHT)) return;

	int i=0;
	//check if coordinates are not swapped
	if (x0 > x1)
	{
		SSD1306_swap(&x0, &x1);
	}

	if (y0 > y1)
	{
		SSD1306_swap(&y0, &y1);
	}

	//check if vertical line
	if (x0 == x1)
	{
		for (i=0; i<= (y1-y0); i++)
		{
			SSD1306_setPixel(x0, y0+i, color);
		}
		return;
	}

	//check if horizontal line
	if (y0 == y1)
	{
		for (i=0; i<= (x1-x0); i++)
		{
			SSD1306_setPixel(x0+i, y0, color);
		}
		return;
	}

	//check the angle
	if (y1<x1)
	{
		//iterate over horizontal direction
		for (i=0; i<= (x1-x0); i++)
		{
			SSD1306_setPixel(x0+i, (y1*(x0+i))/x1, color);
		}
	}else
	{
		//iterate over vertical direction
		for (i=0; i<= (y1-y0); i++)
		{
			SSD1306_setPixel((x1*(x0+i))/y1, y0+i, color);
		}
	}
}

/*
 * Updates the displayBuffer
 * x - coordinates in horizontal plane
 * pageIndex - coordinates in verical plane - index of page
 */
void SSD1306_setBuffer(uint8_t x, uint8_t pageIndex,  uint8_t *buffer, int size)
{
	// check if within bounds
	if ((pageIndex >= LCD_PAGES) || (x >= LCD_WIDTH)) return;
	int i;

	for(i=0; i< size; i++)
	{
		displayBuffer[pageIndex*LCD_WIDTH+x+i] = buffer[i];
	}
}

/*
 * Sets a string in the displayBuffer
 * x - beginning of the string in the horizontal plane
 * pageIndex - beginning of the string in vertical plane
 * string - string to set
 * size - number of characters
 * color - black/white string
 */
void SSD1306_setString(uint8_t x, uint8_t pageIndex, uint8_t *string, int size, uint8_t color)
{
	// check if within bounds. check also the size?
	if ((pageIndex >= LCD_PAGES) || (x >= LCD_WIDTH)) return;

	int i;
	int j;

	uint8_t stringBuffer [size*6];
	uint8_t *strPtr=stringBuffer;
	const uint8_t *chrPtr;
	uint8_t temp;

	for(i=0; i<size; i++)
	{
		chrPtr = &SSD1306_font6x8[(string[i] - ' ') * 6];

		for(j=0; j<6; j++)
		{
			temp = *chrPtr;

			if(!color)
			{
				temp=~temp;
			}
			*strPtr = temp;
			++strPtr;
			++chrPtr;
		}

	}
	SSD1306_setBuffer(x, pageIndex,stringBuffer, size*6);
}

/*
 * Drawing Functions
 */

/*
 * Draws a page on the display
 * pageIndex - index of the page to draw
 * pageBuffer - data to send
 */
void SSD1306_drawPage(uint8_t pageIndex, uint8_t * pageBuffer)
{
	//commands to set page address and column starting point at 2: this display is shifted by 2 pixels so the column ranges from 2-129
	uint8_t cmds[5]={SSD1306_PAGESTART+pageIndex, SSD1306_SETLOWCOLUMN, SSD1306_Offset,SSD1306_SETHIGHCOLUMN,0x10};

	SSD1306_writeCmd(cmds,sizeof(cmds));
	SSD1306_writeData(pageBuffer, LCD_WIDTH, 0);
}

/*
 * Draws a page on the display using DMA for transfer
 * pageIndex - index of the page to draw
 * pageBuffer - data to send
 */
void SSD1306_drawPageDMA(uint8_t pageIndex, uint8_t * pageBuffer)
{
	busy = 1;
	//commands to set page address and column starting point at 2: this display is shifted by 2 pixels so the column ranges from 2-129
	uint8_t cmds[5]={SSD1306_PAGESTART+pageIndex, SSD1306_SETLOWCOLUMN, SSD1306_Offset,SSD1306_SETHIGHCOLUMN,0x10};

	SSD1306_writeCmd(cmds,sizeof(cmds));
	SSD1306_writeData(pageBuffer, LCD_WIDTH, 1);
}

/*
 * Sends a displayBuffer to the display
 */
void SSD1306_drawBitmap(void)
{
	uint16_t i;

	for (i=0; i<LCD_PAGES; i++)
	{
		SSD1306_drawPage(i, displayBuffer+(i*LCD_WIDTH));
	}
}

/*
 * Sends a displayBuffer to the display using DMA for transfer
 */
void SSD1306_drawBitmapDMA(void)
{
	busy = 1;
	dma_transfers = LCD_PAGES;

	SSD1306_drawPageDMA(0, displayBuffer);

}

int SSD1306_isBusy(void)
{
	return busy;
}

/*
 * Clears the display
 */
void SSD1306_clear(void)
{
	uint16_t i;

	uint8_t buffer[LCD_WIDTH];

	for (i=0; i<LCD_WIDTH; i++)
	{
		buffer[i]=0xff;
	}

	for (i=0; i<LCD_PAGES; i++)
	{
		SSD1306_drawPage(i, buffer);
	}
}

void TWI0_Handler(void)
{
	uint32_t status;

	status = twi_get_interrupt_mask(TWI0);

	int pIndex;

	if((status & TWI_IMR_ENDTX) == TWI_IMR_ENDTX)
	{
		uart_write(UART0, 'z');
		twi_disable_interrupt(TWI0, TWI_SR_ENDTX);
	}

	dma_transfers--;

	/* Disable the RX and TX PDC transfer requests */
	pdc_disable_transfer(g_p_twim_pdc, PERIPH_PTCR_TXTDIS | PERIPH_PTCR_RXTDIS);

	while((twi_read_status(TWI0) & TWI_SR_TXRDY) == 0);

	TWI0->TWI_CR = TWI_CR_STOP;
	while (!(TWI0->TWI_SR & TWI_SR_TXCOMP)) {
	}

	if(dma_transfers>0)
	{
		pIndex=LCD_PAGES-dma_transfers;
		SSD1306_drawPageDMA(pIndex, displayBuffer+(pIndex*LCD_WIDTH));
	}
	else
	{
		busy = 0;
		dma_transfers = 0;
	}
}

