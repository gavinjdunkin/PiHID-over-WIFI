// windows_udp_server.c
#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <winsock2.h>
#include <windows.h>

#pragma comment(lib, "ws2_32.lib")  // for MSVC; ignored by MinGW

int main(void)
{
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

    for (int i = 0; i < 4; ++i) {
        int length = recvfrom(sock, buffer, (int)sizeof(buffer) - 1, 0,
                              (struct sockaddr*)&clientaddr, &clientlen);
        if (length == SOCKET_ERROR) {
            fprintf(stderr, "recvfrom failed: %d\n", WSAGetLastError());
            break;
        }

        buffer[length] = '\0';

        char clientip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &clientaddr.sin_addr, clientip, sizeof(clientip));
        printf("From %s:%d — %d bytes: '%s'\n",
               clientip, ntohs(clientaddr.sin_port), length, buffer);
    }

    closesocket(sock);
    WSACleanup();
    return 0;
}
