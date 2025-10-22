#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <linux/input.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <time.h>
#include <sys/select.h>

#define MAX_PACKET_LEN        1024
#define MAX_EVENTS_PER_BATCH  64
#define BATCH_SEND_TIMEOUT_US 5000
#define MAX_INPUT_DEVS        8  // up to 8 devices

#define MAX_KEYS 6
#define HID_UPDATE_INTERVAL_US 1000  // 1 ms

static uint8_t linux_to_hid[MAX_KEYMAP];

void init_key_table(void) {
    memset(linux_to_hid, 0, sizeof(linux_to_hid));

    // letters
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

    // digits (main keyboard)
    linux_to_hid[11] = HID_KEY_0;
    linux_to_hid[2] = HID_KEY_1;
    linux_to_hid[3] = HID_KEY_2;
    linux_to_hid[4] = HID_KEY_3;
    linux_to_hid[5] = HID_KEY_4;
    linux_to_hid[6] = HID_KEY_5;
    linux_to_hid[7] = HID_KEY_6;
    linux_to_hid[8] = HID_KEY_7;
    linux_to_hid[9] = HID_KEY_8;
    linux_to_hid[10] = HID_KEY_9;

    // symbols and punctuation
    linux_to_hid[12] = HID_KEY_MINUS;
    linux_to_hid[13] = HID_KEY_EQUAL;
    linux_to_hid[26] = HID_KEY_BRACKET_LEFT;
    linux_to_hid[27] = HID_KEY_BRACKET_RIGHT;
    linux_to_hid[43] = HID_KEY_BACKSLASH;
    linux_to_hid[39] = HID_KEY_SEMICOLON;
    linux_to_hid[40] = HID_KEY_APOSTROPHE;
    linux_to_hid[41] = HID_KEY_GRAVE;
    linux_to_hid[51] = HID_KEY_COMMA;
    linux_to_hid[52] = HID_KEY_PERIOD;
    linux_to_hid[53] = HID_KEY_SLASH;

    // control keys
    linux_to_hid[1]  = HID_KEY_ESCAPE;
    linux_to_hid[14] = HID_KEY_BACKSPACE;
    linux_to_hid[15] = HID_KEY_TAB;
    linux_to_hid[28] = HID_KEY_ENTER;
    linux_to_hid[57] = HID_KEY_SPACE;
    linux_to_hid[58] = HID_KEY_CAPS_LOCK;

    // function keys
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

    // modifier keys
    linux_to_hid[42] = HID_KEY_SHIFT_LEFT;
    linux_to_hid[54] = HID_KEY_SHIFT_RIGHT;
    linux_to_hid[29] = HID_KEY_CONTROL_LEFT;
    linux_to_hid[97] = HID_KEY_CONTROL_RIGHT;
    linux_to_hid[56] = HID_KEY_ALT_LEFT;
    linux_to_hid[100]= HID_KEY_ALT_RIGHT;
    linux_to_hid[125]= HID_KEY_GUI_LEFT;   // Windows/Super key
    linux_to_hid[126]= HID_KEY_GUI_RIGHT;

    // navigation keys
    linux_to_hid[102] = HID_KEY_HOME;
    linux_to_hid[104] = HID_KEY_PAGE_UP;
    linux_to_hid[111] = HID_KEY_DELETE;
    linux_to_hid[107] = HID_KEY_END;
    linux_to_hid[109] = HID_KEY_PAGE_DOWN;
    linux_to_hid[103] = HID_KEY_ARROW_UP;
    linux_to_hid[108] = HID_KEY_ARROW_DOWN;
    linux_to_hid[105] = HID_KEY_ARROW_LEFT;
    linux_to_hid[106] = HID_KEY_ARROW_RIGHT;

    // lock keys
    linux_to_hid[69] = HID_KEY_NUM_LOCK;
    linux_to_hid[70] = HID_KEY_SCROLL_LOCK;

    // keypad numbers
    linux_to_hid[82] = HID_KEY_KEYPAD_0;
    linux_to_hid[79] = HID_KEY_KEYPAD_1;
    linux_to_hid[80] = HID_KEY_KEYPAD_2;
    linux_to_hid[81] = HID_KEY_KEYPAD_3;
    linux_to_hid[75] = HID_KEY_KEYPAD_4;
    linux_to_hid[76] = HID_KEY_KEYPAD_5;
    linux_to_hid[77] = HID_KEY_KEYPAD_6;
    linux_to_hid[71] = HID_KEY_KEYPAD_7;
    linux_to_hid[72] = HID_KEY_KEYPAD_8;
    linux_to_hid[73] = HID_KEY_KEYPAD_9;

    // keypad operators
    linux_to_hid[83] = HID_KEY_KEYPAD_DECIMAL;
    linux_to_hid[96] = HID_KEY_KEYPAD_ENTER;
    linux_to_hid[78] = HID_KEY_KEYPAD_ADD;
    linux_to_hid[74] = HID_KEY_KEYPAD_SUBTRACT;
    linux_to_hid[55] = HID_KEY_KEYPAD_MULTIPLY;
    linux_to_hid[98] = HID_KEY_KEYPAD_DIVIDE;

    // additional keys
    linux_to_hid[110] = HID_KEY_INSERT;
    linux_to_hid[119] = HID_KEY_PAUSE;
    linux_to_hid[99] = HID_KEY_PRINT_SCREEN;
    linux_to_hid[127] = HID_KEY_APPLICATION; // Menu key
}


static uint8_t key_state[MAX_KEYS] = {0};
static uint8_t current_modifiers = 0;
static uint64_t last_hid_send = 0;
static uint8_t prev_modifiers = 0;
static uint8_t prev_keys[MAX_KEYS] = {0};

static void sleep_us_rel(unsigned int us) {
    struct timespec ts;
    ts.tv_sec = us / 1000000;
    ts.tv_nsec = (us % 1000000) * 1000;
    nanosleep(&ts, NULL);
}

static long diff_us_since(struct timespec *a, struct timespec *b) {
    return (a->tv_sec - b->tv_sec) * 1000000L + (a->tv_nsec - b->tv_nsec) / 1000L;
}

// Add a key to the key state
void hid_add_key(uint8_t keycode) {
    for (int i = 0; i < MAX_KEYS; i++) {
        if (key_state[i] == keycode) return;  // Avoid duplicates
    }
    for (int i = 0; i < MAX_KEYS; i++) {
        if (key_state[i] == 0) {
            key_state[i] = keycode;
            break;
        }
    }
}

// Remove a key from the key state
void hid_remove_key(uint8_t keycode) {
    for (int i = 0; i < MAX_KEYS; i++) {
        if (key_state[i] == keycode) {
            key_state[i] = 0;
            break;
        }
    }
    // Compact the array
    uint8_t compacted[MAX_KEYS] = {0};
    int idx = 0;
    for (int i = 0; i < MAX_KEYS; i++) {
        if (key_state[i] != 0 && idx < MAX_KEYS) {
            compacted[idx++] = key_state[i];
        }
    }
    memcpy(key_state, compacted, MAX_KEYS);
}

// Set or clear a modifier key
void hid_set_modifier(uint8_t modifier, bool pressed) {
    if (pressed)
        current_modifiers |= modifier;
    else
        current_modifiers &= ~modifier;
}

void hid_send_keycodes(int sock, struct sockaddr_in *addr, char *mouse_commands)
{
    uint64_t now = time_us_64();
    if (now - last_hid_send < HID_UPDATE_INTERVAL_US) return;
    last_hid_send = now;

    char packet[MAX_PACKET_LEN];
    int packet_len = 0;
    
    // Add keycode array (always send current state)
    packet_len = snprintf(packet, sizeof(packet), "HID,%d,", current_modifiers);
    for (int i = 0; i < MAX_KEYS; i++) {
        packet_len += snprintf(packet + packet_len, sizeof(packet) - packet_len, "%d,", key_state[i]);
    }
    
    // Add mouse commands if any
    if (mouse_commands && strlen(mouse_commands) > 0) {
        packet_len += snprintf(packet + packet_len, sizeof(packet) - packet_len, "%s", mouse_commands);
    }
    
    printf("Sending HID packet: %.*s\n", packet_len, packet);
    fflush(stdout);
    
    sendto(sock, packet, packet_len, 0, (struct sockaddr *)addr, sizeof(*addr));

    prev_modifiers = current_modifiers;
    memcpy(prev_keys, key_state, MAX_KEYS);
}

// Handle a key event
void handle_key_event(int sock, struct sockaddr_in *addr, uint8_t linux_keycode, bool pressed) {
    uint8_t hid_keycode = linux_to_hid[linux_keycode];
    if (hid_keycode == 0) return;  // Unknown key

    if (hid_keycode >= HID_KEY_CONTROL_LEFT && hid_keycode <= HID_KEY_GUI_RIGHT) {
        // Modifier key
        hid_set_modifier(1 << (hid_keycode - HID_KEY_CONTROL_LEFT), pressed);
    }

    if (pressed) {
        hid_add_key(hid_keycode);
    } else {
        hid_remove_key(hid_keycode);
    }
}

int main(int argc, char **argv) {
    if (argc < 5) {
        fprintf(stderr, "Usage: %s DEST_IP DEST_PORT /dev/input/eventX [/dev/input/eventY ...]\n", argv[0]);
        return 1;
    }

    const char *dest_ip = argv[1];
    int dest_port = atoi(argv[2]);
    int dev_count = argc - 3;
    if (dev_count > MAX_INPUT_DEVS) {
        fprintf(stderr, "Too many input devices (max %d)\n", MAX_INPUT_DEVS);
        return 1;
    }

    int fds[MAX_INPUT_DEVS];
    for (int i = 0; i < dev_count; i++) {
        const char *devpath = argv[i + 3];
        fds[i] = open(devpath, O_RDONLY | O_NONBLOCK);
        if (fds[i] < 0) {
            perror(devpath);
            return 1;
        }
        printf("Opened: %s (fd=%d)\n", devpath, fds[i]);
    }

    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) {
        perror("socket");
        return 1;
    }

    struct sockaddr_in addr = {0};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(dest_port);
    if (inet_pton(AF_INET, dest_ip, &addr.sin_addr) != 1) {
        fprintf(stderr, "Invalid IP: %s\n", dest_ip);
        return 1;
    }

    printf("Sending to %s:%d\n", dest_ip, dest_port);
    fflush(stdout);

    struct input_event ev;
    char packet[MAX_PACKET_LEN];
    int packet_len = 0, batch_events = 0, total_packets_sent = 0;

    // Mouse state accumulation
    int mouse_delta_x = 0;
    int mouse_delta_y = 0;
    int mouse_wheel = 0;
    int mouse_buttons = 0;    // Bitmask: bit 0=left, bit 1=right, bit 2=middle
    int mouse_events_pending = 0;
    init_key_table();

    struct timespec last_send;
    clock_gettime(CLOCK_MONOTONIC, &last_send);

    while (1) {
        fd_set readfds;
        FD_ZERO(&readfds);
        int max_fd = -1;
        for (int i = 0; i < dev_count; i++) {
            FD_SET(fds[i], &readfds);
            if (fds[i] > max_fd) max_fd = fds[i];
        }

        struct timeval timeout = {0, 2000};
        int ret = select(max_fd + 1, &readfds, NULL, NULL, &timeout);
        if (ret < 0) {
            perror("select");
            break;
        }

        for (int i = 0; i < dev_count; i++) {
            if (FD_ISSET(fds[i], &readfds)) {
                while (1) {
                    ssize_t r = read(fds[i], &ev, sizeof(ev));
                    if (r == sizeof(ev)) {
                        batch_events++;

                        if (ev.type == EV_REL) {
                            // Accumulate mouse movement
                            if (ev.code == 0) {        // REL_X
                                mouse_delta_x += ev.value;
                                mouse_events_pending = 1;
                            } else if (ev.code == 1) { // REL_Y
                                mouse_delta_y += ev.value;
                                mouse_events_pending = 1;
                            } else if (ev.code == 8) { // REL_WHEEL
                                mouse_wheel += ev.value;
                                mouse_events_pending = 1;
                            }
                        } else if (ev.type == EV_KEY && (ev.code == 272 || ev.code == 273 || ev.code == 274)) {
                            // Mouse button events
                            if (ev.code == 272) {      // BTN_LEFT
                                if (ev.value) mouse_buttons |= 1;
                                else mouse_buttons &= ~1;
                                mouse_events_pending = 1;
                            } else if (ev.code == 273) { // BTN_RIGHT
                                if (ev.value) mouse_buttons |= 2;
                                else mouse_buttons &= ~2;
                                mouse_events_pending = 1;
                            } else if (ev.code == 274) { // BTN_MIDDLE
                                if (ev.value) mouse_buttons |= 4;
                                else mouse_buttons &= ~4;
                                mouse_events_pending = 1;
                            }
                        } else if (ev.type == EV_KEY && ev.value < 2) {
                            // Handle keyboard events (update keycode array)
                            handle_key_event(sock, addr, ev.code, ev.value == 1);
                        } else continue;

                        if (batch_events >= MAX_EVENTS_PER_BATCH) {
                            // Send current keycode array and accumulated mouse state
                            char mouse_commands[256] = {0};
                            if (mouse_events_pending) {
                                snprintf(mouse_commands, sizeof(mouse_commands), 
                                    "M,%d,%d,%d,%d", mouse_delta_x, mouse_delta_y, mouse_wheel, mouse_buttons);
                            }
                            
                            hid_send_keycodes(sock, &addr, mouse_commands);
                            
                            // Reset mouse accumulation
                            mouse_delta_x = 0;
                            mouse_delta_y = 0;
                            mouse_wheel = 0;
                            mouse_events_pending = 0;
                            
                            packet_len = 0;
                            batch_events = 0;
                            clock_gettime(CLOCK_MONOTONIC, &last_send);
                            total_packets_sent++;
                        }
                    } else if (r < 0 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
                        break;
                    } else {
                        perror("read");
                        goto cleanup;
                    }
                }
            }
        }

        struct timespec now;
        clock_gettime(CLOCK_MONOTONIC, &now);
        long elapsed_us = diff_us_since(&now, &last_send);
        if ((batch_events > 0 || mouse_events_pending) && elapsed_us >= BATCH_SEND_TIMEOUT_US) {
            // Send current keycode array and accumulated mouse state
            char mouse_commands[256] = {0};
            if (mouse_events_pending) {
                snprintf(mouse_commands, sizeof(mouse_commands), 
                    "M,%d,%d,%d,%d", mouse_delta_x, mouse_delta_y, mouse_wheel, mouse_buttons);
            }
            
            hid_send_keycodes(sock, &addr, mouse_commands);
            
            // Reset mouse accumulation
            mouse_delta_x = 0;
            mouse_delta_y = 0;
            mouse_wheel = 0;
            mouse_events_pending = 0;
            
            packet_len = 0;
            batch_events = 0;
            clock_gettime(CLOCK_MONOTONIC, &last_send);
            total_packets_sent++;
        }
    }

cleanup:
    printf("\nTotal packets sent: %d\n", total_packets_sent);
    for (int i = 0; i < dev_count; i++) close(fds[i]);
    close(sock);
    return 0;
}
