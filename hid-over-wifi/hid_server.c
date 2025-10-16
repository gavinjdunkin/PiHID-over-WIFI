#include "pico/stdlib.h"
#include "tusb.h"
#include <stdio.h>
#include <stdbool.h>
#include <string.h>

#include "pico/cyw43_arch.h"

// ======== HID INTERFACE ========
// This implements both keyboard and mouse behavior
// You can call these from your UDP receive code.

static uint8_t last_mouse_buttons = 0;
static uint8_t last_keycode = 0;

static uint8_t linux_to_hid[256];
static bool key_table_initialized = false;


#define MAX_KEYS 6

static uint8_t key_state[MAX_KEYS] = {0};
static uint8_t current_modifiers = 0;

void init_key_table(void) {
    if (key_table_initialized) return;
    memset(linux_to_hid, 0, sizeof(linux_to_hid));

    // Letters
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

    // Numbers
    linux_to_hid[11] = HID_KEY_0;
    linux_to_hid[2]  = HID_KEY_1;
    linux_to_hid[3]  = HID_KEY_2;
    linux_to_hid[4]  = HID_KEY_3;
    linux_to_hid[5]  = HID_KEY_4;
    linux_to_hid[6]  = HID_KEY_5;
    linux_to_hid[7]  = HID_KEY_6;
    linux_to_hid[8]  = HID_KEY_7;
    linux_to_hid[9]  = HID_KEY_8;
    linux_to_hid[10] = HID_KEY_9;

    // Space and common
    linux_to_hid[57] = HID_KEY_SPACE;
    linux_to_hid[28] = HID_KEY_ENTER;
    linux_to_hid[15] = HID_KEY_TAB;
    linux_to_hid[1]  = HID_KEY_ESCAPE;
    linux_to_hid[14] = HID_KEY_BACKSPACE;

    // Arrow keys
    linux_to_hid[103] = HID_KEY_ARROW_UP;
    linux_to_hid[108] = HID_KEY_ARROW_DOWN;
    linux_to_hid[105] = HID_KEY_ARROW_LEFT;
    linux_to_hid[106] = HID_KEY_ARROW_RIGHT;

    // Function keys
    linux_to_hid[59] = HID_KEY_F1;
    linux_to_hid[60] = HID_KEY_F2;
    linux_to_hid[61] = HID_KEY_F3;
    linux_to_hid[62] = HID_KEY_F4;
    linux_to_hid[63] = HID_KEY_F5;
    linux_to_hid[64] = HID_KEY_F6;
    linux_to_hid[65] = HID_KEY_F7;
    linux_to_hid[66] = HID_KEY_F8;
    linux_to_hid[67] = HID_KEY_F9;
    linux_to_hid[68] = HID_KEY_F10;
    linux_to_hid[87] = HID_KEY_F11;
    linux_to_hid[88] = HID_KEY_F12;

    // Modifiers
    linux_to_hid[42]  = HID_KEY_SHIFT_LEFT;
    linux_to_hid[54]  = HID_KEY_SHIFT_RIGHT;
    linux_to_hid[29]  = HID_KEY_CONTROL_LEFT;
    linux_to_hid[97]  = HID_KEY_CONTROL_RIGHT;
    linux_to_hid[56]  = HID_KEY_ALT_LEFT;
    linux_to_hid[100] = HID_KEY_ALT_RIGHT;
    linux_to_hid[125] = HID_KEY_GUI_LEFT;
    linux_to_hid[126] = HID_KEY_GUI_RIGHT;
    
    key_table_initialized = true;
}

void hid_add_key(uint8_t keycode) {
    // Avoid duplicates
    for (int i = 0; i < MAX_KEYS; i++) {
        if (key_state[i] == keycode) return;
    }

    // Find empty slot
    for (int i = 0; i < MAX_KEYS; i++) {
        if (key_state[i] == 0) {
            key_state[i] = keycode;
            break;
        }
    }
}

void hid_remove_key(uint8_t keycode) {
    for (int i = 0; i < MAX_KEYS; i++) {
        if (key_state[i] == keycode) {
            key_state[i] = 0;
            break;
        }
    }

    // Compact array
    uint8_t compacted[MAX_KEYS] = {0};
    int idx = 0;
    for (int i = 0; i < MAX_KEYS; i++) {
        if (key_state[i] != 0 && idx < MAX_KEYS) {
            compacted[idx++] = key_state[i];
        }
    }
    memcpy(key_state, compacted, MAX_KEYS);
}


void hid_set_modifier(uint8_t modifier, bool pressed) {
    if (pressed)
        current_modifiers |= modifier;
    else
        current_modifiers &= ~modifier;
}

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

void handle_key_event(uint8_t linux_keycode, bool pressed) {
    if (!key_table_initialized) return;
    uint8_t hid_keycode = linux_to_hid[linux_keycode];
    if (hid_keycode == 0) return; // Unknown key

    if (hid_keycode >= HID_KEY_CONTROL_LEFT && hid_keycode <= HID_KEY_GUI_RIGHT) {
        // Modifier key
        hid_set_modifier(1 << (hid_keycode - HID_KEY_CONTROL_LEFT), pressed);
    } else {
        // Regular key
        if (pressed) {
            hid_add_key(hid_keycode);
        } else {
            hid_remove_key(hid_keycode);
        }
    }
    hid_send_report();
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
    
    // clear stuck keys if no packet in >300 ms
    if (absolute_time_diff_us(last_hid_send, get_absolute_time()) > 300000) {
        memset(key_state, 0, sizeof(key_state));
        current_modifiers = 0;
        tud_hid_keyboard_report(1, 0, key_state);
    }
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
