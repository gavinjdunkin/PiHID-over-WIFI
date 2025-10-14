#ifndef HID_HANDLER_H
#define HID_HANDLER_H

#include <stdint.h>
#include <stdbool.h>
#include "tusb.h"

// HID action types
typedef enum {
    HID_ACTION_KEY_PRESS,
    HID_ACTION_KEY_RELEASE,
    HID_ACTION_MOUSE_MOVE,
    HID_ACTION_MOUSE_CLICK,
    HID_ACTION_MOUSE_RELEASE
} hid_action_type_t;

// HID action structure
typedef struct {
    hid_action_type_t type;
    union {
        struct {
            uint8_t keycode;
            uint8_t modifier;
        } key;
        struct {
            int8_t x;
            int8_t y;
            uint8_t buttons;
            int8_t scroll;
        } mouse;
    } data;
} hid_action_t;

// Function declarations
void hid_handler_init(void);
void hid_handler_task(void);
void hid_send_keyboard_report(uint8_t modifier, uint8_t* keycodes, uint8_t keycode_count);
void hid_send_mouse_report(uint8_t buttons, int8_t x, int8_t y, int8_t scroll, int8_t pan);
void hid_process_action(const hid_action_t* action);
void hid_send_key_press(uint8_t keycode, uint8_t modifier);
void hid_send_key_release(void);
void hid_send_mouse_move(int8_t x, int8_t y);
void hid_send_mouse_click(uint8_t buttons);
void hid_send_mouse_release(void);

// Parse string command to HID action
bool hid_parse_command(const char* command, hid_action_t* action);

#endif // HID_HANDLER_H