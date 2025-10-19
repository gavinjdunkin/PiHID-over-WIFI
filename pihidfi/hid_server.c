#include "hid_server.h"
#include "tusb.h"
#include <stdio.h>
#include <string.h>

// ───────────────────────────────
// Keycode translation
// ───────────────────────────────
#define MAX_KEYMAP 128
static uint8_t linux_to_hid[MAX_KEYMAP];

void init_key_table(void) {
    memset(linux_to_hid, 0, sizeof(linux_to_hid));

    // letters (example subset — add others as needed)
    linux_to_hid[30] = HID_KEY_A;
    linux_to_hid[48] = HID_KEY_B;
    linux_to_hid[46] = HID_KEY_C;
    linux_to_hid[32] = HID_KEY_D;
    linux_to_hid[18] = HID_KEY_E;
    linux_to_hid[33] = HID_KEY_F;
    linux_to_hid[34] = HID_KEY_G;
    linux_to_hid[35] = HID_KEY_H;
    linux_to_hid[23] = HID_KEY_I;
    linux_to_hid[36] = HID_KEY_J;
    linux_to_hid[37] = HID_KEY_K;
    linux_to_hid[38] = HID_KEY_L;
    linux_to_hid[50] = HID_KEY_M;
    linux_to_hid[49] = HID_KEY_N;
    linux_to_hid[24] = HID_KEY_O;
    linux_to_hid[25] = HID_KEY_P;
    linux_to_hid[16] = HID_KEY_Q;
    linux_to_hid[19] = HID_KEY_R;
    linux_to_hid[31] = HID_KEY_S;
    linux_to_hid[20] = HID_KEY_T;
    linux_to_hid[22] = HID_KEY_U;
    linux_to_hid[47] = HID_KEY_V;
    linux_to_hid[17] = HID_KEY_W;
    linux_to_hid[45] = HID_KEY_X;
    linux_to_hid[21] = HID_KEY_Y;
    linux_to_hid[44] = HID_KEY_Z;

    // digits
    linux_to_hid[2] = HID_KEY_1;
    linux_to_hid[3] = HID_KEY_2;
    linux_to_hid[4] = HID_KEY_3;
    linux_to_hid[5] = HID_KEY_4;
    linux_to_hid[6] = HID_KEY_5;
    linux_to_hid[7] = HID_KEY_6;
    linux_to_hid[8] = HID_KEY_7;
    linux_to_hid[9] = HID_KEY_8;
    linux_to_hid[10] = HID_KEY_9;
    linux_to_hid[11] = HID_KEY_0;

    // common control keys
    linux_to_hid[28] = HID_KEY_ENTER;
    linux_to_hid[1]  = HID_KEY_ESCAPE;
    linux_to_hid[14] = HID_KEY_BACKSPACE;
    linux_to_hid[15] = HID_KEY_TAB;
    linux_to_hid[57] = HID_KEY_SPACE;

    // shift/control/alt
    linux_to_hid[42] = HID_KEY_SHIFT_LEFT;
    linux_to_hid[54] = HID_KEY_SHIFT_RIGHT;
    linux_to_hid[29] = HID_KEY_CONTROL_LEFT;
    linux_to_hid[97] = HID_KEY_CONTROL_RIGHT;
    linux_to_hid[56] = HID_KEY_ALT_LEFT;
    linux_to_hid[100]= HID_KEY_ALT_RIGHT;
}

// ───────────────────────────────
// Keyboard handling
// ───────────────────────────────
void handle_key_event(uint8_t linux_keycode, bool pressed) {
    if (linux_keycode >= MAX_KEYMAP) return;
    uint8_t hid_code = linux_to_hid[linux_keycode];
    if (!hid_code) return;

    uint8_t keycode[6] = {0};
    if (pressed) {
        keycode[0] = hid_code;
        tud_hid_n_keyboard_report(0, 0, 0, keycode);  // Interface 0 = keyboard
    } else {
        tud_hid_n_keyboard_report(0, 0, 0, keycode);  // all zeros = no keys pressed
    }
}

// ───────────────────────────────
// Mouse handling
// ───────────────────────────────
static uint8_t mouse_buttons = 0;

void hid_send_mouse_button(uint8_t button_mask, bool pressed) {
    if (pressed) {
        mouse_buttons |= button_mask;
    } else {
        mouse_buttons &= ~button_mask;
    }
    tud_hid_n_mouse_report(1, 0, mouse_buttons, 0, 0, 0, 0);  // Interface 1 = mouse
}

void hid_send_mouse_move(int8_t dx, int8_t dy, int8_t wheel) {
    tud_hid_n_mouse_report(1, 0, mouse_buttons, dx, dy, wheel, 0);  // Interface 1 = mouse
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
