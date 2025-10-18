#ifndef HID_SERVER_H
#define HID_SERVER_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// Initialize key translation table (Linux → HID)
void init_key_table(void);

// Handle keyboard events coming from UDP
//   linux_keycode: key code (from Linux input.h style numbers)
//   pressed: true if key pressed, false if released
void handle_key_event(uint8_t linux_keycode, bool pressed);

// Send mouse movement or wheel scroll
// dx: delta X, dy: delta Y, wheel: scroll wheel movement
void hid_send_mouse_move(int8_t dx, int8_t dy, int8_t wheel);

// Send mouse button press/release
// button_mask: 1=left, 2=right, 4=middle
// pressed: true=press, false=release
void hid_send_mouse_button(uint8_t button_mask, bool pressed);

// Called each main loop iteration to run TinyUSB tasks
void hid_task(void);

#ifdef __cplusplus
}
#endif

#endif // HID_SERVER_H
