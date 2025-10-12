#include <windows.h>
static WORD linux_to_windows[512] = {0};
void init_key_table(void)
{
    // Common alphanumeric keys
    linux_to_windows[30] = 'A';           // KEY_A
    linux_to_windows[48] = 'B';           // KEY_B
    linux_to_windows[46] = 'C';           // KEY_C
    linux_to_windows[32] = 'D';           // KEY_D
    linux_to_windows[18] = 'E';           // KEY_E
    linux_to_windows[33] = 'F';           // KEY_F
    linux_to_windows[34] = 'G';           // KEY_G
    linux_to_windows[35] = 'H';           // KEY_H
    linux_to_windows[23] = 'I';           // KEY_I
    linux_to_windows[36] = 'J';           // KEY_J
    linux_to_windows[37] = 'K';           // KEY_K
    linux_to_windows[38] = 'L';           // KEY_L
    linux_to_windows[50] = 'M';           // KEY_M
    linux_to_windows[49] = 'N';           // KEY_N
    linux_to_windows[24] = 'O';           // KEY_O
    linux_to_windows[25] = 'P';           // KEY_P
    linux_to_windows[16] = 'Q';           // KEY_Q
    linux_to_windows[19] = 'R';           // KEY_R
    linux_to_windows[31] = 'S';           // KEY_S
    linux_to_windows[20] = 'T';           // KEY_T
    linux_to_windows[22] = 'U';           // KEY_U
    linux_to_windows[47] = 'V';           // KEY_V
    linux_to_windows[17] = 'W';           // KEY_W
    linux_to_windows[45] = 'X';           // KEY_X
    linux_to_windows[21] = 'Y';           // KEY_Y
    linux_to_windows[44] = 'Z';           // KEY_Z

    // Numbers
    linux_to_windows[11] = '0';           // KEY_0
    linux_to_windows[2] = '1';            // KEY_1
    linux_to_windows[3] = '2';            // KEY_2
    linux_to_windows[4] = '3';            // KEY_3
    linux_to_windows[5] = '4';            // KEY_4
    linux_to_windows[6] = '5';            // KEY_5
    linux_to_windows[7] = '6';            // KEY_6
    linux_to_windows[8] = '7';            // KEY_7
    linux_to_windows[9] = '8';            // KEY_8
    linux_to_windows[10] = '9';           // KEY_9

    // Function keys
    linux_to_windows[59] = VK_F1;          // KEY_F1
    linux_to_windows[60] = VK_F2;          // KEY_F2
    linux_to_windows[61] = VK_F3;          // KEY_F3
    linux_to_windows[62] = VK_F4;          // KEY_F4
    linux_to_windows[63] = VK_F5;          // KEY_F5
    linux_to_windows[64] = VK_F6;          // KEY_F6
    linux_to_windows[65] = VK_F7;          // KEY_F7
    linux_to_windows[66] = VK_F8;          // KEY_F8
    linux_to_windows[67] = VK_F9;          // KEY_F9
    linux_to_windows[68] = VK_F10;         // KEY_F10
    linux_to_windows[87] = VK_F11;         // KEY_F11
    linux_to_windows[88] = VK_F12;         // KEY_F12

    // Modifier keys
    linux_to_windows[42] = VK_LSHIFT;      // KEY_LEFTSHIFT
    linux_to_windows[54] = VK_RSHIFT;      // KEY_RIGHTSHIFT
    linux_to_windows[29] = VK_LCONTROL;    // KEY_LEFTCTRL
    linux_to_windows[97] = VK_RCONTROL;    // KEY_RIGHTCTRL
    linux_to_windows[56] = VK_LMENU;       // KEY_LEFTALT (Alt)
    linux_to_windows[100] = VK_RMENU;      // KEY_RIGHTALT
    linux_to_windows[125] = VK_LWIN;       // KEY_LEFTMETA (Windows key)
    linux_to_windows[126] = VK_RWIN;       // KEY_RIGHTMETA

    // Special keys
    linux_to_windows[1] = VK_ESCAPE;       // KEY_ESC
    linux_to_windows[15] = VK_TAB;         // KEY_TAB
    linux_to_windows[58] = VK_CAPITAL;     // KEY_CAPSLOCK
    linux_to_windows[28] = VK_RETURN;      // KEY_ENTER
    linux_to_windows[57] = VK_SPACE;       // KEY_SPACE
    linux_to_windows[14] = VK_BACK;        // KEY_BACKSPACE
    linux_to_windows[111] = VK_DELETE;     // KEY_DELETE

    // Arrow keys
    linux_to_windows[103] = VK_UP;         // KEY_UP
    linux_to_windows[108] = VK_DOWN;       // KEY_DOWN
    linux_to_windows[105] = VK_LEFT;       // KEY_LEFT
    linux_to_windows[106] = VK_RIGHT;      // KEY_RIGHT

    // Navigation
    linux_to_windows[102] = VK_HOME;       // KEY_HOME
    linux_to_windows[107] = VK_END;        // KEY_END
    linux_to_windows[104] = VK_PRIOR;      // KEY_PAGEUP
    linux_to_windows[109] = VK_NEXT;       // KEY_PAGEDOWN
    linux_to_windows[110] = VK_INSERT;     // KEY_INSERT

    // Symbols (US layout)
    linux_to_windows[12] = VK_OEM_MINUS;   // KEY_MINUS (-)
    linux_to_windows[13] = VK_OEM_PLUS;    // KEY_EQUAL (=)
    linux_to_windows[26] = VK_OEM_4;       // KEY_LEFTBRACE ([)
    linux_to_windows[27] = VK_OEM_6;       // KEY_RIGHTBRACE (])
    linux_to_windows[39] = VK_OEM_1;       // KEY_SEMICOLON (;)
    linux_to_windows[40] = VK_OEM_7;       // KEY_APOSTROPHE (')
    linux_to_windows[41] = VK_OEM_3;       // KEY_GRAVE (`)
    linux_to_windows[43] = VK_OEM_5;       // KEY_BACKSLASH (\)
    linux_to_windows[51] = VK_OEM_COMMA;   // KEY_COMMA (,)
    linux_to_windows[52] = VK_OEM_PERIOD;  // KEY_DOT (.)
    linux_to_windows[53] = VK_OEM_2;       // KEY_SLASH (/)
}
WORD get_windows_vk(int linux_code)
{
    if (linux_code < 0 || linux_code >= 512) return 0;
    return linux_to_windows[linux_code];
}