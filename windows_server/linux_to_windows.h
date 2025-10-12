#ifndef LINUX_TO_WINDOWS_H
#define LINUX_TO_WINDOWS_H

#include <windows.h>

// Function declarations
void init_key_table(void);
WORD get_windows_vk(int linux_code);
WORD get_mouse_button_vk(int linux_code);

// Constants
#define MAX_LINUX_KEYCODE 512

#endif // LINUX_TO_WINDOWS_H