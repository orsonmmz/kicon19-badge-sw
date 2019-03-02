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

/**
 * Commands use the following format:
 *
 * offset | description
 * -------+------------
 *      1 | command length (when 1 <= length <= 255) or reset request (when 0)
 *      2 | command type (see cmd_type_t)                      \__ payload
 *  3-255 | actual command data, specific to the command type  /
 *    n+1 | crc (all payload bytes xored together)
 *
 * In order to reset the command buffer to a known state, keep sending 0x00
 * until CMD_RESET_ACK is received.
 */

#ifndef COMMANDS_H
#define COMMANDS_H

#include <stdint.h>
#include "commands_def.h"

/**
 * @brief Adds new data to the command buffer.
 * Call whenever a new data is received.
 * @param data is the received character.
 */
void cmd_new_data(uint8_t data);

/**
 * @brief Try to execute the buffer contents.
 * @return Command interpreter response following the command format.
 * (ownership is not transferred, do not free the pointer).
 * @see cmd_resp_t
 */
const uint8_t* cmd_try_execute(void);

/**
 * @brief Calculates raw command length.
 * @return Command length in bytes.
 */
static inline unsigned cmd_raw_len(uint8_t *cmd)
{
    /* payload length is stored in the first byte
       +2 stands for the length and crc fields */
    return cmd[0] + 2;
}

static inline unsigned cmd_payload_len(uint8_t *cmd)
{
    return cmd[0];
}

/* Command handlers */
void cmd_uart(const uint8_t* data_in, unsigned int input_len, uint8_t *data_out);
/*void cmd_i2c(const uint8_t* data_in, unsigned int input_len, uint8_t *data_out);*/
/*void cmd_spi(const uint8_t* data_in, unsigned int input_len, uint8_t *data_out);*/
/*void cmd_pio(const uint8_t* data_in, unsigned int input_len, uint8_t *data_out);*/
/*void cmd_led(const uint8_t* data_in, unsigned int input_len, uint8_t *data_out);*/
/*void cmd_lcd(const uint8_t* data_in, unsigned int input_len, uint8_t *data_out);*/
/*void cmd_btn(const uint8_t* data_in, unsigned int input_len, uint8_t *data_out);*/
/*void cmd_adc(const uint8_t* data_in, unsigned int input_len, uint8_t *data_out);*/
/*void cmd_dac(const uint8_t* data_in, unsigned int input_len, uint8_t *data_out);*/

#endif /* COMMANDS_H */
