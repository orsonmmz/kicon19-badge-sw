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
 * Multiprotocol commands use the following format:
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

typedef enum { CMD_MULTIPROTOCOL, CMD_SUMP } cmd_mode_t;

/**
 * @brief Adds a byte to the command buffer.
 * Call whenever a new data is received.
 * @param data is the received character.
 */
void cmd_new_data(uint8_t data);

/**
 * @brief Try to execute the buffer contents.
 * @return 1 when the command has been processed, 0 if waits for more data
 */
int cmd_try_execute(void);

/**
 * @brief Returns the command response (if any) and its length.
 * If there is no response, data will be set to NULL and len to 0.
 */
void cmd_get_resp(const uint8_t **data, unsigned int *len);

/**
 * @brief Notify that the command response has been processed and might be
 * disposed. Should be called when the response data is no longer needed.
 */
void cmd_resp_processed(void);

/**
 * @brief Calculates raw command length.
 * @return Command length in bytes.
 */
static inline unsigned cmd_raw_len(const uint8_t *cmd)
{
    /* payload length is stored in the first byte
       +2 stands for the length and crc fields */
    return cmd[0] + 2;
}

static inline unsigned cmd_payload_len(const uint8_t *cmd)
{
    return cmd[0];
}

/**
 * @brief Switches between different command modes.
 */
void cmd_set_mode(cmd_mode_t mode);

/**
 * @brief Return the current command mode.
 */
cmd_mode_t cmd_get_mode(void);

#endif /* COMMANDS_H */
