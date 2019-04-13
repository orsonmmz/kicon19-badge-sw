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

#ifndef LCD_H
#define LCD_H

#include <stdint.h>

#define LCD_WIDTH           128
#define LCD_HEIGHT          64
#define LCD_PAGE_SIZE       8
#define LCD_PAGES           LCD_HEIGHT/LCD_PAGE_SIZE

typedef enum {
    BLACK,      ///< Draw 'off' pixels
    WHITE,      ///< Draw 'on' pixels
    INVERSE     ///< Invert pixels
} color_t;


/**
 * Initializes: TWI (aka I2C), display, clears the buffer..
 */
void SSD1306_init(void);


/**
 * Immediately clears the display (buffer is untouched).
 */
void SSD1306_clear(void);

/**
 * Clears a selected region of the buffer.
 *
 * @param x is the coordinate in horizontal plane (from 0 to LCD_WIDTH - 1).
 * @param pageIndex is the coordinate in the vertical plane (from 0 to LCD_PAGES - 1).
 * @param color the pixels should be lit or not.
 * @param size is the number of segments to clear.
 */
void SSD1306_clearBuffer(uint8_t x, uint8_t pageIndex, color_t color, int size);

/**
 * Clears whole buffer.
 */
void SSD1306_clearBufferFull(void);

/**
 * Sets a pixel value.
 *
 * @param x is the horizontal coordinate.
 * @param y is the vertical coordinate.
 * @param color is the new pixel color.
 */
void SSD1306_setPixel(uint8_t x, uint8_t y, color_t color);

/**
 * Draws a line segment.
 *
 * @param x0 is the horizontal coordinate of the origin.
 * @param y0 is the vertical coordinate of the origin.
 * @param x1 is the horizontal coordinate of the end.
 * @param y1 is the vertical coordinate of the end.
 * @param color defines the line color.
 */
void SSD1306_setLine(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1, color_t color);

/**
 * Draws a text.
 *
 * @param x is the horizontal offset (from 0 to LCD_WIDTH - 1).
 * @param pageIndex is the row (from 0 to LCD_PAGES - 1).
 * @param string is the text to display.
 * @param size is the text length.
 * @param color is the text color.
 */
void SSD1306_setString(uint8_t x, uint8_t pageIndex, const char *string,
        int size, color_t color);

/**
 * Copies a bitmap to the buffer.
 *
 * @param x0 is the horizontal coordinate of the origin (top-left corner).
 * @param y0 is the vertical coordinate of the origin (top-left corner).
 * @param bitmap is the bitmap data (1-bit per pixel, rows then cols).
 * @param width is the bitmap width (pixels).
 * @param height is the bitmap height (pixels).
 */
void SSD1306_drawBitmap(uint8_t x0, uint8_t y0, const uint8_t *bitmap,
        uint8_t width, uint8_t height);

/**
 * Blocking call that draws a page using the provided data.
 *
 * @param pageIndex is the page number (from 0 to LCD_PAGES - 1).
 * @param pageBuffer is the data to be drawn.
 */
void SSD1306_drawPage(uint8_t pageIndex, const uint8_t *pageBuffer);

/**
 * Blocking call that copies the buffer contents to the display.
 */
void SSD1306_drawBuffer(void);

/**
 * Starts a DMA transfer that draws a page using the provided data.
 *
 * @param pageIndex is the page number (from 0 to LCD_PAGES - 1).
 * @param pageBuffer is the data to be drawn.
 */
void SSD1306_drawPageDMA(uint8_t pageIndex, const uint8_t *pageBuffer);

/**
 * Starts a DMA transfer to copy the buffer contents to the display.
 */
void SSD1306_drawBufferDMA(void);

/**
 * Returns 1 if the buffer is transferred to the display. One should not
 * modify the buffer contents during the data transfer.
 */
int SSD1306_isBusy(void);

#endif /* LCD_H */
