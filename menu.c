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

#include "menu.h"
#include "menu_struct.h"
#include "buttons.h"
#include "lcd.h"
#include "SSD1306_commands.h"
#include "gfx.h"

#include <stddef.h>
#include <string.h>


#define MENU_ENTRY_HEIGHT   8
#define MENU_ENTRY_COUNT    (LCD_HEIGHT / MENU_ENTRY_HEIGHT)

static menu_list_t *menu_current = &main_menu;

#define MENU_STACK_DEPTH    8
typedef struct _menu_stack_entry_t {
    menu_list_t *menu;
    int selection;
} menu_stack_entry_t;
static menu_stack_entry_t menu_stack[MENU_STACK_DEPTH] = { { NULL, 0 }, };

static int offset = 0;
static int selection = 0;
static int down = 0;

static void menu_stack_push(menu_list_t *menu, int sel) {
    menu_stack_entry_t *ptr = menu_stack;

    for (int i = 0; i < MENU_STACK_DEPTH; ++i) {
        if (ptr->menu == NULL) {
            ptr->menu = menu;
            ptr->selection = sel;
            break;
        }

        ++ptr;
    }
}

static menu_stack_entry_t menu_stack_pop(void) {
    menu_stack_entry_t ret = { NULL, 0 };

    // starting the loop from 1, since it
    // subtracts one to get the last non-NULL pointer
    menu_stack_entry_t *ptr = menu_stack + 1;

    for (int i = 1; i < MENU_STACK_DEPTH; ++i) {
        if (ptr->menu == NULL) {
            --ptr;
            ret = *ptr;
            ptr->menu = NULL;
            break;
        }

        ++ptr;
    }

    return ret;
}

static void menu_draw(void) {
    while(SSD1306_isBusy());
    SSD1306_clearBufferFull();

    for (int i = 0; i < MENU_ENTRY_COUNT; ++i) {
        int entry_idx = i + offset;
        const menu_entry_t *entry = &menu_current->entries[entry_idx];
        const char *entryName = NULL;

        switch (entry->type) {
            case SETTING: entryName = entry->data.setting; break;
            case APP: entryName = entry->data.app->name; break;
            case SUBMENU: entryName = entry->data.submenu->name; break;
            case END: break;
        }

        if (!entryName) {
            break;
        }

        SSD1306_setString(0, i, entryName, strlen(entryName),
                entry_idx == selection ? BLACK : WHITE);

        if (entry->type == SETTING && entry_idx == menu_current->val) {
            SSD1306_setString(128 - 6, i, "<", 1,
                entry_idx == selection ? BLACK : WHITE);
        }
    }

    SSD1306_drawBufferDMA();
}


static void menu_enter(menu_list_t *new_menu) {
    menu_stack_push(menu_current, selection);
    offset = 0;
    selection = new_menu->val;
    menu_current = new_menu;
}


static void menu_leave(void) {
    menu_stack_entry_t prev = menu_stack_pop();

    if (prev.menu) {
        offset = 0;
        selection = prev.selection;
        menu_current = prev.menu;
    }
}


void menu(void) {
    int buttons = 0;

    menu_draw();

    // wait for a button press
    while (buttons == 0) {
        buttons = btn_state();
    }

    // now wait until the pressed button is released
    while (btn_state() != 0);

    if (buttons & BUT_UP) {
        if (selection == 0) {
            // move *past* last menu entry (so it will be decremented later)
            selection = 0;
            while (menu_current->entries[selection].type != END) ++selection;
        }

        --selection;
    }

    else if (buttons & BUT_DOWN) {
        if (menu_current->entries[selection + 1].type == END) {
            selection = 0;
        } else {
            ++selection;
        }
    }

    else if (buttons & BUT_RIGHT) {
        const menu_entry_t *sel_entry = &menu_current->entries[selection];

        switch (sel_entry->type) {
            case APP:
                sel_entry->data.app->func();
                break;

            case SUBMENU:
                menu_enter(sel_entry->data.submenu);
                break;

            case SETTING:
                menu_current->val = selection;
                menu_leave();
                break;

            case END: break;    // mute warnings
        }
    }

    else if (buttons & BUT_LEFT) {
        menu_leave();
    }

    // adjust the display offset
    if (selection < offset)
        offset = selection;
    else if (selection >= offset + MENU_ENTRY_COUNT) {
        offset = selection - MENU_ENTRY_COUNT + 1;
    }

    if (buttons & BUT_DOWN) {
        if (++down == 20) {
            SSD1306_clearBufferFull();
            SSD1306_drawBitmap(16, 0, gfx_test, 96, 64);
            SSD1306_drawBufferDMA();
            while (!btn_state());
            down = 0;
        }
    } else {
        down = 0;
    }
}
