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


uint8_t SSD1306_orientation = 0;//not used
uint8_t SSD1306_maxX = 0;		//not used
uint8_t SSD1306_maxY = 0;		//not used


/*
 * buffer to store image and send to display
 */
static uint8_t displayBuffer[LCD_WIDTH*LCD_PAGES];

/*
 * initializes: twi connection, display,
 * fills displayBuffer with 0xff (pixels on)
 * clears the display
 */
void lcd_init(void)
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

	//init twi - fcn?
    twi_options_t opt;
    opt.master_clk = sysclk_get_peripheral_hz();
    opt.speed      = 200000;    // TODO

    pio_configure(PIOA, PIO_PERIPH_A,
            (PIO_PA3A_TWD0 | PIO_PA4A_TWCK0), PIO_OPENDRAIN | PIO_PULLUP);
    sysclk_enable_peripheral_clock(ID_TWI0);
    pmc_enable_periph_clk(ID_TWI0);

    twi_master_init(TWI0, &opt);

    // TODO twi_probe?

    //init buffer
	uint16_t i;

	for (i=0; i<LCD_WIDTH*LCD_PAGES; i++)
	{
		displayBuffer[i]=0xff;
	}

    lcd_writeCmd(init, sizeof(init));
//    SSD1306_setOrientation(0);
    SSD1306_clear();
}

/*
 * Writes data over twi
 * buffer - bytes to send
 * size - number of bytes
 * ctrl_b - indicates whether it is cmd or data
 */
void lcd_write(uint8_t * buffer,int size, control_byte ctrl_b)
{
    twi_packet_t packet_tx;

    packet_tx.chip        = SSD1306_address;

    packet_tx.addr[0] = (uint8_t) ctrl_b;

    packet_tx.addr_length = 1;
    packet_tx.buffer      = (uint8_t *) buffer;
    packet_tx.length      = size;

    twi_master_write(TWI0, &packet_tx);
}

/*
 * Writes a CMD over twi
 * buffer - bytes to send
 * size - number of bytes
 */
void lcd_writeCmd(uint8_t * buffer, int size)
{
	control_byte ctrl = cmd;
	lcd_write(buffer, size, ctrl);
}

/*
 * Writes data over twi
 * buffer - bytes to send
 * size - number of bytes
 */
void lcd_writeData(uint8_t * buffer, int size)
{
	control_byte ctrl = data;
	lcd_write(buffer, size, ctrl);
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
	}
	else
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
	uint8_t *chrPtr;
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
void SSD1306_drawPage( uint8_t pageIndex, uint8_t * pageBuffer)
{
	//commands to set page address and column starting point at 2: this display is shifted by 2 pixels so the column ranges from 2-129
	uint8_t cmds[5]={SSD1306_PAGESTART+pageIndex, SSD1306_SETLOWCOLUMN, 0x02,SSD1306_SETHIGHCOLUMN,0x10};

	lcd_writeCmd(cmds,sizeof(cmds));
	lcd_writeData(pageBuffer, LCD_WIDTH);
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

