#include "hid_server.h"
#include "tusb.h"
#include <stdio.h>
#include <string.h>


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
    
    // Send keyboard report immediately
    if (tud_hid_ready()) {
        tud_hid_keyboard_report(0, modifiers, keys);
    }
    
    // Update mouse state and send report
    if (tud_hid_ready()) {
        tud_hid_n_mouse_report(1, 0, buttons, dx, dy, wheel, 0);
    }
}
