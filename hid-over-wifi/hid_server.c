#include "pico/stdlib.h"
#include "tusb.h"
#include <stdio.h>
#include <stdbool.h>

// ======== HID INTERFACE ========
// This implements both keyboard and mouse behavior
// You can call these from your UDP receive code.

static uint8_t last_mouse_buttons = 0;
static uint8_t last_keycode = 0;

// ===== Keyboard =====
void hid_send_key(uint8_t keycode, bool pressed) {
    if (!tud_hid_ready()) return;

    if (pressed) {
        // Send press - TinyUSB expects: report_id, modifier, key_array
        uint8_t key_array[6] = {keycode, 0, 0, 0, 0, 0};
        tud_hid_keyboard_report(1, 0, key_array);
        last_keycode = keycode;
    } else {
        // Send release (no keys pressed)
        tud_hid_keyboard_report(1, 0, NULL);
        last_keycode = 0;
    }
}

// ===== Mouse movement =====
void hid_send_mouse_move(int8_t dx, int8_t dy, int8_t wheel) {
    if (!tud_hid_ready()) return;
    tud_hid_mouse_report(2, last_mouse_buttons, dx, dy, wheel, 0);
}

// ===== Mouse button press/release =====
// Button bits (same as TinyUSB spec):
// 1 = Left, 2 = Right, 4 = Middle
void hid_send_mouse_button(uint8_t button_mask, bool pressed) {
    if (!tud_hid_ready()) return;

    if (pressed)
        last_mouse_buttons |= button_mask;
    else
        last_mouse_buttons &= ~button_mask;

    tud_hid_mouse_report(2, last_mouse_buttons, 0, 0, 0, 0);
}

// ===== Periodic USB task =====
// Call this from your main loop
void hid_task(void) {
    tud_task();
}

// ===== TinyUSB Callbacks =====
uint16_t tud_hid_get_report_cb(uint8_t itf, uint8_t report_id,
                               uint8_t report_type,
                               uint8_t *buffer, uint16_t reqlen) {
    (void) itf;
    (void) report_id;
    (void) report_type;
    (void) buffer;
    (void) reqlen;
    return 0;
}

void tud_hid_set_report_cb(uint8_t itf, uint8_t report_id,
                           uint8_t report_type,
                           const uint8_t *buffer, uint16_t bufsize) {
    (void) itf;
    (void) report_id;
    (void) report_type;
    (void) buffer;
    (void) bufsize;
}
