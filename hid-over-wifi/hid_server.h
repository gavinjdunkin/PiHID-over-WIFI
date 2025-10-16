#pragma once
#include <stdint.h>
#include <stdbool.h>
void init_key_table(void);
void handle_key_event(uint8_t linux_keycode, bool pressed);
void hid_send_mouse_move(int8_t dx, int8_t dy, int8_t wheel);
void hid_send_mouse_button(uint8_t button_mask, bool pressed);
void hid_task(void);
