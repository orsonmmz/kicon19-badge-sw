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

#ifndef SPI_MASTER_H
#define SPI_MASTER_H

#include <stdint.h>

/**
 * Configures the SPI port.
 * @param clock is the SPI clock frequency [Hz]
 * @param mode is the SPI port mode:
 *      0: capture data on rising clock edge, inactive clock low
 *      1: capture data on falling clock edge, inactive clock low
 *      2: capture data on falling clock edge, inactive clock high
 *      3: capture data on rising clock edge, inactive clock high
 */
void spi_init(uint32_t clock, int mode);

#endif /* SPI_MASTER_H */
