


// g++ -I../include cs16TCPClientRandomGen.cpp  -o cs16TCPClientRandomGen.exe -L ../libs -lws2_32
//  ./cs16TCPClientRandomGen.exe

#include <winsock2.h>
#include <ws2tcpip.h>
#include <iostream>
#include <vector>
#include <cstdlib>
#include <ctime>

// Link with Ws2_32.lib
#pragma comment(lib, "Ws2_32.lib")

#define DEFAULT_PORT "12345"
#define DEFAULT_IP "127.0.0.1"

int main() {
    WSADATA wsaData;
    SOCKET ConnectSocket = INVALID_SOCKET;
    struct addrinfo *result = NULL, hints;
    
    // Initialize Winsock
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cerr << "WSAStartup failed\n";
        return 1;
    }

    ZeroMemory(&hints, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;

    // Resolve server address and port
    if (getaddrinfo(DEFAULT_IP, DEFAULT_PORT, &hints, &result) != 0) {
        std::cerr << "getaddrinfo failed\n";
        WSACleanup();
        return 1;
    }

    // Attempt to connect to server
    ConnectSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
    if (connect(ConnectSocket, result->ai_addr, (int)result->ai_addrlen) == SOCKET_ERROR) {
        std::cerr << "Unable to connect to server\n";
        closesocket(ConnectSocket);
        freeaddrinfo(result);
        WSACleanup();
        return 1;
    }
    freeaddrinfo(result);

    std::cout << "Connected. Sending random s16 data...\n";
    srand((unsigned int)time(NULL));

    // Send loop
    while (true) {
        // Generate 1000 random shorts (-32768 to 32767) [3]
        std::vector<short> buffer(1000);
        for (int i = 0; i < 1000; ++i) {
            buffer[i] = static_cast<short>(rand() % 65536 - 32768);
        }

        // Send data
        int bytesToSend = buffer.size() * sizeof(short);
        if (send(ConnectSocket, reinterpret_cast<const char*>(buffer.data()), bytesToSend, 0) == SOCKET_ERROR) {
            std::cerr << "Send failed\n";
            break;
        }
        Sleep(10); // Control rate
    }

    closesocket(ConnectSocket);
    WSACleanup();
    return 0;
}
