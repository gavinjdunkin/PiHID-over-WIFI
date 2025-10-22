#include "hid_server.h"
#include "tusb.h"
#include <stdio.h>
#include <string.h>

// ───────────────────────────────
// Keycode translation
// ───────────────────────────────
#define MAX_KEYMAP 128


#define MAX_KEYS 6

static uint8_t key_state[MAX_KEYS] = {0};
static uint8_t current_modifiers = 0;

#define HID_UPDATE_INTERVAL_US 1000  // 1 ms
static uint64_t last_hid_send = 0;
static uint8_t prev_modifiers = 0;
static uint8_t prev_keys[MAX_KEYS] = {0};

void hid_send_report(void) {
    if (!tud_hid_ready()) return;

    uint64_t now = time_us_64();
    if (now - last_hid_send < HID_UPDATE_INTERVAL_US) return;
    last_hid_send = now;
    
    if (memcmp(prev_keys, key_state, MAX_KEYS) == 0 &&
        prev_modifiers == current_modifiers) {
        return; // nothing changed
    }

    tud_hid_keyboard_report(1, current_modifiers, key_state);

    prev_modifiers = current_modifiers;
    memcpy(prev_keys, key_state, MAX_KEYS);
}


// ───────────────────────────────
// Mouse handling
// ───────────────────────────────
static uint8_t mouse_buttons = 0;

void hid_send_mouse_button(uint8_t button_mask, bool pressed) {
    if (pressed) {
        mouse_buttons |= button_mask;  // Set the button bit
    } else {
        mouse_buttons &= ~button_mask; // Clear the button bit
    }
    
    // Send mouse report with current button state and no movement
    tud_hid_n_mouse_report(1, 0, mouse_buttons, 0, 0, 0, 0);
}

void hid_send_mouse_move(int8_t dx, int8_t dy, int8_t wheel) {
    // Use standard TinyUSB mouse report for interface 1  
    // Include current button state so drag operations work
    tud_hid_n_mouse_report(1, 0, mouse_buttons, dx, dy, wheel, 0);
}

// New function to send mouse report with explicit button state
void hid_send_mouse_report(uint8_t buttons, int8_t dx, int8_t dy, int8_t wheel) {
    mouse_buttons = buttons;  // Update internal state
    tud_hid_n_mouse_report(1, 0, buttons, dx, dy, wheel, 0);
}

// ───────────────────────────────
// TinyUSB periodic poll
// ───────────────────────────────
void hid_task(void) {
    tud_task(); // handle USB events
}

// ───────────────────────────────
// TinyUSB HID Callbacks (required)
// ───────────────────────────────
uint16_t tud_hid_get_report_cb(uint8_t itf, uint8_t report_id,
                               hid_report_type_t report_type,
                               uint8_t *buffer, uint16_t reqlen) {
    // Not used
    return 0;
}

void tud_hid_set_report_cb(uint8_t itf, uint8_t report_id,
                           hid_report_type_t report_type,
                           const uint8_t *buffer, uint16_t bufsize) {
    // Not used
}

// ───────────────────────────────
// Complete HID state update
// ───────────────────────────────
void hid_set_complete_state(uint8_t modifiers, uint8_t keys[6], int8_t dx, int8_t dy, int8_t wheel, uint8_t buttons) {
    // Update keyboard state
    current_modifiers = modifiers;
    memcpy(key_state, keys, MAX_KEYS);
    
    // Send keyboard report immediately
    if (tud_hid_ready()) {
        tud_hid_keyboard_report(1, current_modifiers, key_state);
    }
    
    // Update mouse state and send report
    mouse_buttons = buttons;
    if (tud_hid_ready()) {
        tud_hid_n_mouse_report(1, 0, buttons, dx, dy, wheel, 0);
    }
}
