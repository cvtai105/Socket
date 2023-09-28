// web_server.cpp : Defines the entry point for the console application.
// Ref: https://learn.microsoft.com/en-us/windows/win32/winsock/complete-server-code?source=recommendations
// Build application: g++ main.cpp -o main.exe -lWs2_32
#undef UNICODE
#define WIN32_LEAN_AND_MEAN
#include <ws2tcpip.h>
#include <stdlib.h>
#include <stdio.h>
#include <iostream>
#include "http.h"
#include <thread>
// Need to link with Ws2_32.lib
#pragma comment (lib, "Ws2_32.lib")
// #pragma comment (lib, "Mswsock.lib")

#define DEFAULT_PORT "8080"

void ShutUp(SOCKET sListen,int* flag) {
    while (1) {
        cin >> *flag;
        if (*flag == 1) {
            closesocket(sListen);
            WSACleanup();
            return;
        }
    }
}

int __cdecl main(void)
{   
   
    WSADATA wsaData;
    int iResult;

    SOCKET ListenSocket = INVALID_SOCKET;
    SOCKET newConnection = INVALID_SOCKET;

    struct addrinfo* result = NULL;
    struct addrinfo hints;



    // Initialize Winsock
    iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (iResult != 0) {
        printf("WSAStartup failed with error: %d\n", iResult);
        return 1;
    }

    ZeroMemory(&hints, sizeof(hints));
    hints.ai_family = AF_INET;      //internetwork: tcp, udp,...
    hints.ai_socktype = SOCK_STREAM;    //stream socket
    hints.ai_protocol = IPPROTO_TCP;    //tcp
    hints.ai_flags = AI_PASSIVE;    //bind-socket

    // Resolve the server address and port
    iResult = getaddrinfo(NULL, DEFAULT_PORT, &hints, &result);
    if (iResult != 0) {
        printf("getaddrinfo failed with error: %d\n", iResult);
        WSACleanup();
        return 1;
    }

    // Create a SOCKET for the server to listen for client connections.
    ListenSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
    if (ListenSocket == INVALID_SOCKET) {
        printf("socket failed with error: %ld\n", WSAGetLastError());
        freeaddrinfo(result);
        WSACleanup();
        return 1;
    }

    //Setup the TCP listening socket
    iResult = bind(ListenSocket, result->ai_addr, (int)result->ai_addrlen);
    if (iResult == SOCKET_ERROR) {
        printf("bind failed with error: %d\n", WSAGetLastError());
        freeaddrinfo(result);
        closesocket(ListenSocket);
        WSACleanup();
        return 1;
    }

    freeaddrinfo(result);

    iResult = listen(ListenSocket, SOMAXCONN);
    if (iResult == SOCKET_ERROR) {
        printf("listen failed with error: %d\n", WSAGetLastError());
        closesocket(ListenSocket);
        WSACleanup();
        return 1;
    }

    int flag = 0;
    thread in(ShutUp, ListenSocket, &flag);
    in.detach();
    printf("Waiting Input from keyboard. Press 1 to CLEANUP Winsock API and exit in 3seconds\n");
    printf("--------------------------------------------------------------------------\n");
    printf("Waiting for connection from client\n");

    //handle multiconnections
    int id = 0;
    SOCKADDR_IN addr;
    int addrlen = sizeof(addr);
    unsigned short port;
    char* ip;
    do
    {
        //accepte, get ip and port of client
        newConnection = accept(ListenSocket, (SOCKADDR*)&addr, &addrlen);
        if (newConnection == INVALID_SOCKET) {
            printf("Closing program on accept() error: %d\n", WSAGetLastError());
            closesocket(ListenSocket);
            WSACleanup();
            this_thread::sleep_for(chrono::milliseconds(3000));
            return 1;
        }
        ip = new char[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &(addr.sin_addr), ip, INET_ADDRSTRLEN);
        port = ntohs(addr.sin_port);
        id++;
        thread thr1(Communicate, newConnection, ip , port);
        thr1.detach();

    } while (1);

    return 0;
}