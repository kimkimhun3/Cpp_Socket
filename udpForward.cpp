#include <iostream>
#include <vector>
#include <string>
#include <winsock2.h>
#include <windows.h>
#include <memory>
#include <ctime>

#pragma comment(lib, "ws2_32.lib")

#define RECEIVER_PORT 5004         // Port where we will receive packets
#define DECODER_IP "192.168.25.89" // IP of the Decoder
#define DECODER_PORT 5004          // Port of the Decoder
#define BUFFER_SIZE 65535          // Max size of a single UDP packet
#define MAX_PACKETS 100000         // Maximum number of packets to buffer

struct Packet {
    std::unique_ptr<char[]> data;
    int size;
};

int main() {
    WSADATA wsaData;
    SOCKET receiverSocket, senderSocket;
    sockaddr_in receiverAddr{}, decoderAddr{};
    std::vector<Packet> packetBuffer;
    int bufferIndex = 0;
    int buffering = 0;

    // Initialize Winsock
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cerr << "WSAStartup failed: " << WSAGetLastError() << std::endl;
        return 1;
    }

    // Create receiver socket
    receiverSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (receiverSocket == INVALID_SOCKET) {
        std::cerr << "socket() failed: " << WSAGetLastError() << std::endl;
        WSACleanup();
        return 1;
    }

    // Setup receiver address structure
    receiverAddr.sin_family = AF_INET;
    receiverAddr.sin_port = htons(RECEIVER_PORT);
    receiverAddr.sin_addr.s_addr = INADDR_ANY;

    // Bind the receiver socket
    if (bind(receiverSocket, reinterpret_cast<sockaddr*>(&receiverAddr), sizeof(receiverAddr)) == SOCKET_ERROR) {
        std::cerr << "bind() failed: " << WSAGetLastError() << std::endl;
        closesocket(receiverSocket);
        WSACleanup();
        return 1;
    }

    // Create sender socket
    senderSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (senderSocket == INVALID_SOCKET) {
        std::cerr << "socket() failed: " << WSAGetLastError() << std::endl;
        closesocket(receiverSocket);
        WSACleanup();
        return 1;
    }

    // Setup decoder address structure
    decoderAddr.sin_family = AF_INET;
    decoderAddr.sin_port = htons(DECODER_PORT);
    decoderAddr.sin_addr.s_addr = inet_addr(DECODER_IP);

    std::cout << "Server started. Receiver listening on port " << RECEIVER_PORT << std::endl;

    // Main loop to receive and forward packets
    while (true) {
        sockaddr_in senderAddr{};
        int senderAddrSize = sizeof(senderAddr);
        char buffer[BUFFER_SIZE];
        int recvLen = recvfrom(receiverSocket, buffer, BUFFER_SIZE, 0, reinterpret_cast<sockaddr*>(&senderAddr), &senderAddrSize);
        // std::cout << "Received packet " << recvLen << std::endl;
        if (recvLen == SOCKET_ERROR) {
            std::cerr << "recvfrom() failed: " << WSAGetLastError() << std::endl;
            continue;
        }

        int result = sendto(senderSocket, buffer, recvLen, 0, reinterpret_cast<sockaddr*>(&decoderAddr), sizeof(decoderAddr));
        if (result == SOCKET_ERROR) {
            std::cerr << "sendto() failed: " << WSAGetLastError() << std::endl;
        }
    }

    // Cleanup
    closesocket(receiverSocket);
    closesocket(senderSocket);
    WSACleanup();
    return 0;
}
