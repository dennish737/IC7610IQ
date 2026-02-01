
//
// g++ -I../include cs16TCPRandomGenerator.cpp  -o cs16TCPRandomGenerator.exe -L ../libs -lws2_32
//  ./cs16TCPRandomGenerator.exe

#include <winsock2.h>
#include <ws2tcpip.h>
#include <iostream>
#include <vector>
#include <cstdlib>
#include <ctime>

// Link with Ws2_32.lib
#pragma comment(lib, "Ws2_32.lib")

int main() {
    WSADATA wsaData;
    SOCKET listenSocket = INVALID_SOCKET, clientSocket = INVALID_SOCKET;
    struct sockaddr_in serverAddr, clientAddr;
    int clientAddrLen = sizeof(clientAddr);

    // Initialize Winsock
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cerr << "WSAStartup failed." << std::endl;
        return 1;
    }

    // Create socket
    listenSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(12345); // Port for GNURadio

    // Bind
    bind(listenSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr));
    listen(listenSocket, 1);
    std::cout << "Server listening on port 12345..." << std::endl;

    // Accept
    clientSocket = accept(listenSocket, (struct sockaddr*)&clientAddr, &clientAddrLen);
    std::cout << "Client connected." << std::endl;
    closesocket(listenSocket); // Close listener

    srand((unsigned)time(NULL));
    std::vector<short> buffer(1024); // Buffer to send

    while (true) {
        // Generate random s16 samples (-32768 to 32767)
        for (int i = 0; i < 1024; ++i) {
            buffer[i] = (short)(rand() % 65536 - 32768);
        }

        // Send data
        int result = send(clientSocket, (char*)buffer.data(), buffer.size() * sizeof(short), 0);
        if (result == SOCKET_ERROR) break;
    }

    closesocket(clientSocket);
    WSACleanup();
    return 0;
}

