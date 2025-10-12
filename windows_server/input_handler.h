#ifndef INPUT_HANDLER_H
#define INPUT_HANDLER_H

#include <windows.h>

// Function declarations
int simulate_key_event(WORD virtual_key, int is_keyup);
int simulate_mouse_event(int dx, int dy, int wheel, DWORD flags);
#endif // INPUT_HANDLER_H