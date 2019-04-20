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

#include "spi_master.h"
#include "command_handlers.h"
#include "io_conf.h"

#include <spi.h>
#include <pio.h>
#include <sysclk.h>

/* Possible chip selects:
 * PA9 (UART_RX)  -> NPCS1 / peripheral B
 * PB14 (DAC1)    -> NPCS1 / peripheral A
 * PA10 (UART_TX) -> NPCS2 / peripheral B
 * PA3 (I2C_SDA)  -> NPCS3 / peripheral B
 * PA22 (ADC1)    -> NPCS3 / peripheral B
 */

/* Chip select (NPCSx)*/
#define SPI_CHIP_SEL 1

/* Delay before SPCK */
#define SPI_DLYBS 0x40

/* Delay between consecutive transfers */
#define SPI_DLYBCT 0x10


void spi_init(uint32_t clock, int mode)
{
    pio_configure(PIOA, PIO_PERIPH_A,
            (PIO_PA12A_MISO | PIO_PA13A_MOSI | PIO_PA14A_SPCK),
            PIO_OPENDRAIN | PIO_PULLUP);

    pio_configure(PIOB, PIO_PERIPH_A, PIO_PB14A_NPCS1,
            PIO_OPENDRAIN | PIO_PULLUP);

    spi_enable_clock(SPI);
    spi_disable(SPI);
    spi_reset(SPI);
    spi_set_master_mode(SPI);
    spi_disable_mode_fault_detect(SPI);
    spi_disable_loopback(SPI);

    switch (mode) {
        default:
        case 0:
            spi_set_clock_polarity(SPI, SPI_CHIP_SEL, 0);
            spi_set_clock_phase(SPI, SPI_CHIP_SEL, 1);
            break;

        case 1:
            spi_set_clock_polarity(SPI, SPI_CHIP_SEL, 0);
            spi_set_clock_phase(SPI, SPI_CHIP_SEL, 0);
            break;

        case 2:
            spi_set_clock_polarity(SPI, SPI_CHIP_SEL, 1);
            spi_set_clock_phase(SPI, SPI_CHIP_SEL, 1);
            break;

        case 3:
            spi_set_clock_polarity(SPI, SPI_CHIP_SEL, 1);
            spi_set_clock_phase(SPI, SPI_CHIP_SEL, 0);
            break;
    }

    spi_set_peripheral_chip_select_value(SPI, spi_get_pcs(SPI_CHIP_SEL));
    spi_set_bits_per_transfer(SPI, SPI_CHIP_SEL, SPI_CSR_BITS_8_BIT);
    spi_set_baudrate_div(SPI, SPI_CHIP_SEL,
            (sysclk_get_peripheral_hz() / clock));
    spi_set_transfer_delay(SPI, SPI_CHIP_SEL, SPI_DLYBS, SPI_DLYBCT);
    spi_enable(SPI);
}


void cmd_spi(const uint8_t* data_in, unsigned int input_len)
{
    io_configure(IO_SPI);

    cmd_resp_init(CMD_RESP_OK);

    switch (data_in[0]) {
        case CMD_SPI_CONFIG: {
                uint32_t clock = ((uint32_t)(data_in[1]) << 8 | data_in[2]) * 1000;
                spi_init(clock, data_in[3]);
            }
            break;

        case CMD_SPI_TRANSFER: {
                uint8_t uc_pcs;
                uint16_t data;
                const uint8_t* ptr = &data_in[2];

                for (uint8_t i = 0; i < data_in[1]; i++) {
                        spi_write(SPI, *ptr, 0, i == data_in[1] - 1 ? 0 : 1);
                        ++ptr;
                        while ((spi_read_status(SPI) & SPI_SR_RDRF) == 0);
                        spi_read(SPI, &data, &uc_pcs);
                        cmd_resp_write(data);
                }
            }
            break;

        default: cmd_resp_init(CMD_RESP_INVALID_CMD); return;
    }
}
