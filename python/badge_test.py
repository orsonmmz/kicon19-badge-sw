#!/usr/bin/env python3

# Copyright (c) 2019 Maciej Suminski <orson@orson.net.pl>
#
# This source code is free software; you can redistribute it
# and/or modify it in source code form under the terms of the GNU
# General Public License as published by the Free Software
# Foundation; either version 2 of the License, or (at your option)
# any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.

from kicon_badge import KiconBadge
import time

badge = KiconBadge('/dev/ttyACM0')
badge.init()

# send data over UART
uart_data = badge.uart_transfer(bytes('badge test', 'ascii'))

# LCD functions demo
badge.lcd_clear()

# draw a checker pattern
for y in range(12):
    for x in range(badge.LCD_WIDTH):
        badge.lcd_pixel(x, y, (x ^ y) & 0x02)

badge.lcd_text(1, 2, "hey, it is scripted")
badge.lcd_text(4, 3, "with Python!")
badge.lcd_refresh()

# LEDs
badge.led_set(badge.LED1, 1)
badge.led_blink(badge.LED2, 2)

# Buttons
print("Buttons state: %x" % badge.buttons())

# I2C transfers
badge.i2c_set_clock_khz(200)
# read the LCD status
#badge.i2c_read(0x3c, 0x00, 1)
# invert the LCD colors
badge.i2c_write(0x3c, 0x00, bytes([0xA7]))
time.sleep(1)
# restore the LCD colors
badge.i2c_write(0x3c, 0x00, bytes([0xA6]))

 # SPI transfers
badge.spi_config(1000, 0)
print(badge.spi_transfer(bytes([0x01, 0x02, 0x03])))
