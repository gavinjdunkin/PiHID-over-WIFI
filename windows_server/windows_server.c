// windows_udp_server.c
#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <winsock2.h>
#include <windows.h>
#include "linux_to_windows.h"
#include "input_handler.h"
#include "common.h"

#pragma comment(lib, "ws2_32.lib")  // for MSVC; ignored by MinGW

int main(void)
{
    init_key_table();
    printf("Server started, listening on port 50037...\n");
    fflush(stdout);

    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2,2), &wsa) != 0) {
        fprintf(stderr, "WSAStartup failed\n");
        return 1;
    }

    SOCKET sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (sock == INVALID_SOCKET) {
        fprintf(stderr, "socket failed: %d\n", WSAGetLastError());
        WSACleanup();
        return 1;
    }

    struct sockaddr_in serveraddr;
    memset(&serveraddr, 0, sizeof(serveraddr));
    serveraddr.sin_family = AF_INET;
    serveraddr.sin_port = htons(50037);
    serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);

    if (bind(sock, (struct sockaddr*)&serveraddr, sizeof(serveraddr)) == SOCKET_ERROR) {
        fprintf(stderr, "bind failed: %d\n", WSAGetLastError());
        closesocket(sock);
        WSACleanup();
        return 1;
    }

    char buffer[200];
    struct sockaddr_in clientaddr;
    int clientlen = sizeof(clientaddr);

    while (1) {
        int length = recvfrom(sock, buffer, (int)sizeof(buffer) - 1, 0,
                              (struct sockaddr*)&clientaddr, &clientlen);
        if (length == SOCKET_ERROR) {
            fprintf(stderr, "recvfrom failed: %d\n", WSAGetLastError());
            break;
        }

        buffer[length] = '\0';

        printf("Received %d bytes: '%s'\n", length, buffer);
        fflush(stdout);

        parsed_message_t msg;
        if (parse_message(buffer, &msg) == 0) {
            printf("Parsed: type=%c, code=%d, value=%d\n", msg.type, msg.code, msg.value);
            fflush(stdout);
            
            if (msg.type == 'K') {
                WORD vk = get_windows_vk(msg.code);
                printf("Linux code %d -> Windows VK %d\n", msg.code, vk);
                fflush(stdout);
                
                if (vk != 0) {
                    printf("Simulating key: VK=%d, keyup=%d\n", vk, msg.value == 0);
                    fflush(stdout);
                    
                    int result = simulate_key_event(vk, msg.value == 0);
                    printf("Key simulation result: %d\n", result);
                    fflush(stdout);
                } else {
                    printf("Warning: No mapping for Linux key code %d\n", msg.code);
                }
            } else if (msg.type == 'M') {
                DWORD flags = 0;
                switch (msg.code) {
                    case 0: // X movement
                        flags = MOUSEEVENTF_MOVE;
                        simulate_mouse_event(msg.value, 0, 0, flags);
                        break;
                    case 1: // Y movement
                        flags = MOUSEEVENTF_MOVE;
                        simulate_mouse_event(0, msg.value, 0, flags);
                        break;
                    case 11: // Wheel
                        flags = MOUSEEVENTF_WHEEL;
                        simulate_mouse_event(0, 0, msg.value, flags | (msg.value << 16));
                        break;
                    case 272: // Left button
                        flags = msg.value ? MOUSEEVENTF_LEFTDOWN : MOUSEEVENTF_LEFTUP;
                        simulate_mouse_event(0, 0, 0, flags);
                        break;
                    case 273: // Right button
                        flags = msg.value ? MOUSEEVENTF_RIGHTDOWN : MOUSEEVENTF_RIGHTUP;
                        simulate_mouse_event(0, 0, 0, flags);
                        break;
                    case 274: // Middle button
                        flags = msg.value ? MOUSEEVENTF_MIDDLEDOWN : MOUSEEVENTF_MIDDLEUP;
                        simulate_mouse_event(0, 0, 0, flags);
                        break;
                    default:
                        // Unsupported mouse event
                        break;
                }
            }
        } else {
            fprintf(stderr, "Failed to parse message: %s\n", buffer);
        }
    }

    closesocket(sock);
    WSACleanup();
    return 0;
}
