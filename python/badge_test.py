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

badge = KiconBadge('/dev/ttyACM0')
badge.reset()
uart_data = badge.uart_transfer(bytes('badge test', 'ascii'))

badge.lcd_clear()
badge.lcd_pixel(2, 2, 1)
badge.lcd_text(1, 1, "aaa")
badge.lcd_refresh()
