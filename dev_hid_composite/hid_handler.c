#include "hid_handler.h"
#include "bsp/board_api.h"
#include "tusb.h"
#include "usb_descriptors.h"
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdint.h>

// Static variables for HID state
static bool has_keyboard_key = false;
static uint32_t last_report_time = 0;

void hid_handler_init(void) {
    has_keyboard_key = false;
    last_report_time = 0;
}

void hid_handler_task(void) {
    // This function can be called periodically to handle any pending HID tasks
    // Currently empty, but can be extended for queued actions
}

void hid_send_keyboard_report(uint8_t modifier, uint8_t* keycodes, uint8_t keycode_count) {
    if (!tud_hid_ready()) return;
    
    uint8_t keycode_array[6] = {0};
    uint8_t count = keycode_count > 6 ? 6 : keycode_count;
    
    if (keycodes && count > 0) {
        memcpy(keycode_array, keycodes, count);
        tud_hid_keyboard_report(REPORT_ID_KEYBOARD, modifier, keycode_array);
        has_keyboard_key = true;
    } else {
        // Send empty key report
        if (has_keyboard_key) {
            tud_hid_keyboard_report(REPORT_ID_KEYBOARD, 0, NULL);
        }
        has_keyboard_key = false;
    }
}

void hid_send_mouse_report(uint8_t buttons, int8_t x, int8_t y, int8_t scroll, int8_t pan) {
    if (!tud_hid_ready()) return;
    
    tud_hid_mouse_report(REPORT_ID_MOUSE, buttons, x, y, scroll, pan);
}

void hid_send_key_press(uint8_t keycode, uint8_t modifier) {
    hid_send_keyboard_report(modifier, &keycode, 1);
}

void hid_send_key_release(void) {
    hid_send_keyboard_report(0, NULL, 0);
}

void hid_send_mouse_move(int8_t x, int8_t y) {
    hid_send_mouse_report(0x00, x, y, 0, 0);
}

void hid_send_mouse_click(uint8_t buttons) {
    hid_send_mouse_report(buttons, 0, 0, 0, 0);
}

void hid_send_mouse_release(void) {
    hid_send_mouse_report(0x00, 0, 0, 0, 0);
}

void hid_process_action(const hid_action_t* action) {
    if (!action) return;
    
    switch (action->type) {
        case HID_ACTION_KEY_PRESS:
            hid_send_key_press(action->data.key.keycode, action->data.key.modifier);
            break;
            
        case HID_ACTION_KEY_RELEASE:
            hid_send_key_release();
            break;
            
        case HID_ACTION_MOUSE_MOVE:
            hid_send_mouse_move(action->data.mouse.x, action->data.mouse.y);
            break;
            
        case HID_ACTION_MOUSE_CLICK:
            hid_send_mouse_click(action->data.mouse.buttons);
            break;
            
        case HID_ACTION_MOUSE_RELEASE:
            hid_send_mouse_release();
            break;
            
        default:
            break;
    }
}

// Simple command parser
// Format examples: 
// "key:a" - press key 'a'
// "key:a+ctrl" - press key 'a' with ctrl modifier
// "mouse:10,5" - move mouse by x=10, y=5
// "click:left" - left mouse button click
// "release" - release all keys/buttons
bool hid_parse_command(const char* command, hid_action_t* action) {
    if (!command || !action) return false;
    
    // Convert to lowercase for easier parsing
    char cmd_lower[64];
    strncpy(cmd_lower, command, sizeof(cmd_lower) - 1);
    cmd_lower[sizeof(cmd_lower) - 1] = '\0';
    
    for (int i = 0; cmd_lower[i]; i++) {
        cmd_lower[i] = tolower(cmd_lower[i]);
    }
    
    if (strncmp(cmd_lower, "key:", 4) == 0) {
        action->type = HID_ACTION_KEY_PRESS;
        action->data.key.modifier = 0;
        
        char* key_part = cmd_lower + 4;
        
        // Check for modifier
        if (strstr(key_part, "+ctrl")) {
            action->data.key.modifier |= KEYBOARD_MODIFIER_LEFTCTRL;
            // Remove modifier from key_part
            char* plus_pos = strchr(key_part, '+');
            if (plus_pos) *plus_pos = '\0';
        }
        if (strstr(key_part, "+shift")) {
            action->data.key.modifier |= KEYBOARD_MODIFIER_LEFTSHIFT;
            char* plus_pos = strchr(key_part, '+');
            if (plus_pos) *plus_pos = '\0';
        }
        if (strstr(key_part, "+alt")) {
            action->data.key.modifier |= KEYBOARD_MODIFIER_LEFTALT;
            char* plus_pos = strchr(key_part, '+');
            if (plus_pos) *plus_pos = '\0';
        }
        
        // Map key character to HID keycode
        if (strlen(key_part) == 1 && key_part[0] >= 'a' && key_part[0] <= 'z') {
            action->data.key.keycode = HID_KEY_A + (key_part[0] - 'a');
        } else if (strcmp(key_part, "space") == 0) {
            action->data.key.keycode = HID_KEY_SPACE;
        } else if (strcmp(key_part, "enter") == 0) {
            action->data.key.keycode = HID_KEY_ENTER;
        } else if (strcmp(key_part, "tab") == 0) {
            action->data.key.keycode = HID_KEY_TAB;
        } else if (strcmp(key_part, "escape") == 0) {
            action->data.key.keycode = HID_KEY_ESCAPE;
        } else {
            return false; // Unknown key
        }
        
        return true;
    }
    
    if (strncmp(cmd_lower, "mouse:", 6) == 0) {
        action->type = HID_ACTION_MOUSE_MOVE;
        
        char* coords = cmd_lower + 6;
        int x, y;
        if (sscanf(coords, "%d,%d", &x, &y) == 2) {
            action->data.mouse.x = (int8_t)x;
            action->data.mouse.y = (int8_t)y;
            action->data.mouse.buttons = 0;
            action->data.mouse.scroll = 0;
            return true;
        }
        return false;
    }
    
    if (strncmp(cmd_lower, "click:", 6) == 0) {
        action->type = HID_ACTION_MOUSE_CLICK;
        action->data.mouse.x = 0;
        action->data.mouse.y = 0;
        action->data.mouse.scroll = 0;
        
        char* button = cmd_lower + 6;
        if (strcmp(button, "left") == 0) {
            action->data.mouse.buttons = MOUSE_BUTTON_LEFT;
        } else if (strcmp(button, "right") == 0) {
            action->data.mouse.buttons = MOUSE_BUTTON_RIGHT;
        } else if (strcmp(button, "middle") == 0) {
            action->data.mouse.buttons = MOUSE_BUTTON_MIDDLE;
        } else {
            return false;
        }
        return true;
    }
    
    if (strcmp(cmd_lower, "release") == 0) {
        action->type = HID_ACTION_KEY_RELEASE;
        return true;
    }
    
    return false; // Unknown command
}