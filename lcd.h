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

#ifndef LCD_H
#define LCD_H

#include <stdint.h>

#define LCD_WIDTH					128
#define LCD_HEIGHT					64
#define LCD_PAGE_SIZE				8
#define LCD_PAGES					LCD_HEIGHT/LCD_PAGE_SIZE

#define SSD1306_address				0x3C
#define SSD1306_DC					6		//0=data 1=command
#define SSD1306_CO					7 		//continuation bit
#define SSD1306_data				0x40
#define SSD1306_cmd					0x00

#define BLACK                       0 ///< Draw 'off' pixels
#define WHITE                       1 ///< Draw 'on' pixels
#define INVERSE 					2 ///< Invert pixels

typedef enum
{
	data = SSD1306_data,
	cmd = SSD1306_cmd
} control_byte;

void lcd_init(void);

void lcd_write(uint8_t * buffer,int size, control_byte ctrl_b);

void lcd_writeCmd(uint8_t * buffer, int size);
void lcd_writeData(uint8_t * buffer, int size);

void SSD1306_setOrientation(uint8_t orientation);
void SSD1306_swap(uint8_t *a, uint8_t *b);
void SSD1306_orientCoordinates(uint8_t *x1, uint8_t *y1);
void SSD1306_clear(void);

void SSD1306_setPixel(uint8_t x, uint8_t y, uint8_t color);
void SSD1306_setLine(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1, uint8_t color);
void SSD1306_setBuffer(uint8_t x, uint8_t pageIndex, uint8_t *buffer, int size);
void SSD1306_setString(uint8_t x, uint8_t pageIndex,  uint8_t *string, int size,uint8_t color);

void SSD1306_drawPage(uint8_t pageIndex, uint8_t * pageBuffer);
void SSD1306_drawBitmap(void);

#endif /* LCD_H */
