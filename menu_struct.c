/*
 * Copyright (C) 2014 Julian Lewis
 * @author Maciej Suminski <maciej.suminski@cern.ch>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 3
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you may find one here: http://www.gnu.org/licenses/gpl-3.0-standalone.html
 * or you may search the http://www.gnu.org website for the version 3 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

/**
 * Menu structure.
 */

#include "menu_struct.h"
#include "apps_list.h"
#include <stddef.h>

int get_menu_size(const menu_list_t *menu) {
    int len = 0;
    const menu_entry_t* ptr = menu->entries;

    // Look for sentinel
    while((*ptr++).type != END) ++len;

    return len;
}


application_t app_la_usb = { "Logic analyzer (USB)", app_la_usb_func };


menu_list_t menu_la_lcd_sampling_freq = {
    "Sampling frequency", 2, {
        { SETTING,  { .setting = "10 MHz" } },
        { SETTING,  { .setting = "5 MHz" } },
        { SETTING,  { .setting = "2 MHz" } },
        { SETTING,  { .setting = "1 MHz" } },
        { SETTING,  { .setting = "500 kHz" } },
        { END,      { NULL } }
    }
};

menu_list_t menu_la_lcd_trigger_input = {
    "Trigger input", 0, {
        { SETTING,  { .setting = "Free run" } },
        { SETTING,  { .setting = "Input 0" } },
        { SETTING,  { .setting = "Input 1" } },
        { SETTING,  { .setting = "Input 2" } },
        { SETTING,  { .setting = "Input 3" } },
        { SETTING,  { .setting = "Input 4" } },
        { SETTING,  { .setting = "Input 5" } },
        { SETTING,  { .setting = "Input 6" } },
        { SETTING,  { .setting = "Input 7" } },
        { END,      { NULL } }
    }
};

menu_list_t menu_la_lcd_trigger_level = {
    "Trigger level", 1,
    {
        { SETTING,  { .setting = "Low" } },
        { SETTING,  { .setting = "High" } },
        { END,      { NULL } }
    }
};

application_t app_la_lcd = { "RUN", app_la_lcd_func };

menu_list_t menu_la_lcd = {
    "Logic analyzer (LCD)", 0,
    {
        { APP,       { .app     = &app_la_lcd } },
        { SUBMENU,   { .submenu = &menu_la_lcd_sampling_freq } },
        { SUBMENU,   { .submenu = &menu_la_lcd_trigger_input } },
        { SUBMENU,   { .submenu = &menu_la_lcd_trigger_level } },
        { END,      { NULL } }
    }
};


menu_list_t menu_scope_channels = {
    "Channels", 0, {
        { SETTING,  { .setting = "1" } },
        { SETTING,  { .setting = "2" } },
        { SETTING,  { .setting = "1 & 2" } },
        { END,      { NULL } }
    }
};

menu_list_t menu_scope_fsampling = {
    "Sampling frequency", 0, {
        { SETTING,  { .setting = "1 MHz" } },
        { SETTING,  { .setting = "500 kHz" } },
        { SETTING,  { .setting = "200 kHz" } },
        { SETTING,  { .setting = "100 kHz" } },
        { SETTING,  { .setting = "50 kHz" } },
        { END,      { NULL } }
    }
};

/* gain setting does not work.. */
/*menu_list_t menu_scope_gain = {
    "Gain", 0, {
        { SETTING,  { .setting = "x1" } },
        { SETTING,  { .setting = "x2" } },
        { SETTING,  { .setting = "x4" } },
        { END,      { NULL } }
    }
};*/

application_t app_scope = { "RUN", app_scope_func };

menu_list_t menu_scope = {
    "Scope", 0,
    {
        { APP,       { .app     = &app_scope } },
        { SUBMENU,   { .submenu = &menu_scope_channels } },
        { SUBMENU,   { .submenu = &menu_scope_fsampling } },
        /*{ SUBMENU,   { .submenu = &menu_scope_gain } },*/
        { END,      { NULL } }
    }
};


application_t app_command = { "Command interface", app_command_func };


application_t app_uart = { "RUN", app_uart_func };

menu_list_t menu_uart_baud = {
    "Baud rate", 0, {
        { SETTING,  { .setting = "115200" } },
        { SETTING,  { .setting = "57600" } },
        { SETTING,  { .setting = "38400" } },
        { SETTING,  { .setting = "9600" } },
        { END,      { NULL } }
    }
};

menu_list_t menu_uart = {
    "USB-UART adapter", 0,
    {
        { APP,      { .app     = &app_uart } },
        { SUBMENU,  { .submenu = &menu_uart_baud } },
        { END,      { NULL } }
    }
};

menu_list_t main_menu = {
    "Main menu", 0,
    {
       { APP,       { .app     = &app_la_usb } },
       { SUBMENU,   { .submenu = &menu_la_lcd } },
       { SUBMENU,   { .submenu = &menu_scope } },
       { APP,       { .app     = &app_command } },
       { SUBMENU,   { .submenu = &menu_uart } },
       { END,       { NULL } }
    }
};

