#include "input_handler.h"
#include <windows.h>

int simulate_key_event(WORD virtual_key, int is_keyup) {
    INPUT input;
    input.type = INPUT_KEYBOARD;
    input.ki.wVk = virtual_key;
    input.ki.wScan = 0; // Let system determine scan code
    input.ki.dwFlags = is_keyup ? KEYEVENTF_KEYUP : 0;
    input.ki.time = 0; // Current time
    input.ki.dwExtraInfo = 0; // No extra info

    return SendInput(1, &input, sizeof(INPUT));
}
int simulate_mouse_event(int dx, int dy, int wheel, DWORD flags) {
    INPUT input;
    input.type = INPUT_MOUSE;
    input.mi.dx = dx;
    input.mi.dy = dy;
    input.mi.mouseData = wheel;
    input.mi.dwFlags = flags;
    input.mi.time = 0;
    input.mi.dwExtraInfo = 0;

    return SendInput(1, &input, sizeof(INPUT));
}