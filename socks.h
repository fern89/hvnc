//basic sockets helper lib, nothing special here
#include <ws2tcpip.h>
#include <winsock2.h>
#include <stdio.h>
#include <windows.h>
#include <stdlib.h>
#include <string.h>
SOCKET sock;

int sock_init(){
    WSADATA wsaData;
    struct addrinfo *result = NULL, hints;
    if(WSAStartup(MAKEWORD(2,2), &wsaData) != 0)
        return -1;
    ZeroMemory(&hints, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    if(getaddrinfo(SERVER_IP, SERVER_PORT, &hints, &result) != 0)
        goto cleanup;
    sock = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
    if(sock == INVALID_SOCKET)
        goto cleanup;
    if(connect(sock, result->ai_addr, (int)result->ai_addrlen) == SOCKET_ERROR){
        closesocket(sock);
        goto cleanup;
    }
    freeaddrinfo(result);
    return 0;
cleanup:
    WSACleanup();
    return -1;
}

int sock_send(const char* data, int len, int x, int y){
    char* dat = calloc(len+12, 1);
    memcpy(dat, &len, 4);
    memcpy(dat+4, &x, 4);
    memcpy(dat+8, &y, 4);
    memcpy(dat+12, data, len);
    if(send(sock, dat, len+12, 0) == SOCKET_ERROR) goto err;
    return 0;
err:
    closesocket(sock);
    WSACleanup();
    return -1;
}

char* sock_recv(){
    char* dat = calloc(9, 1);
    if(recv(sock, dat, 9, MSG_WAITALL) <= 0){
        free(dat);
        closesocket(sock);
        WSACleanup();
        return NULL;
    }
    return dat;
}
