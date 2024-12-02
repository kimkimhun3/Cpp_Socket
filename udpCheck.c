#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <winsock2.h>
#include <windows.h>
#include <time.h>

#pragma comment(lib, "ws2_32.lib")

#define RECEIVER_PORT 5004         // Port where we will receive packets
#define DECODER_IP "192.168.25.89" // IP of the Decoder
#define DECODER_PORT 5004          // Port of the Decoder
#define BUFFER_SIZE 65535          // Max size of a single UDP packet
#define MAX_PACKETS 100000         // Maximum number of packets to buffer
#define START_BUFFERING_TIME 5000  // wait time
#define BUFFERING_DURATION 2000    // buffering time

typedef struct {
    char *data;
    int size;
} Packet;

int main() {
    WSADATA wsaData;
    SOCKET receiverSocket, senderSocket;
    struct sockaddr_in receiverAddr, decoderAddr;
    Packet packetBuffer[MAX_PACKETS];
    int bufferIndex = 0;
    int buffering = 0;
    int result;
    clock_t startTime = clock();       // Track the start time for initial normal operation
    clock_t lastBufferingTime = 0;     // Track the time of the last packet buffered
    clock_t startSendingTime = 0;      // Track the time when packets start sending after buffering
    clock_t endSendingTime = 0;  
    clock_t startBufferingTime = 0;

    // Initialize Winsock
    result = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (result != 0) {
        printf("WSAStartup failed: %d\n", result);
        return 1;
    }

    // Create receiver socket
    receiverSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (receiverSocket == INVALID_SOCKET) {
        printf("socket() failed: %d\n", WSAGetLastError());
        WSACleanup();
        return 1;
    }

    // Setup receiver address structure
    memset(&receiverAddr, 0, sizeof(receiverAddr));
    receiverAddr.sin_family = AF_INET;
    receiverAddr.sin_port = htons(RECEIVER_PORT);
    receiverAddr.sin_addr.s_addr = INADDR_ANY;

    // Bind the receiver socket
    result = bind(receiverSocket, (struct sockaddr *)&receiverAddr, sizeof(receiverAddr));
    if (result == SOCKET_ERROR) {
        printf("bind() failed: %d\n", WSAGetLastError());
        closesocket(receiverSocket);
        WSACleanup();
        return 1;
    }

    // Create sender socket
    senderSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (senderSocket == INVALID_SOCKET) {
        printf("socket() failed: %d\n", WSAGetLastError());
        closesocket(receiverSocket);
        WSACleanup();
        return 1;
    }

    // Setup decoder address structure
    memset(&decoderAddr, 0, sizeof(decoderAddr));
    decoderAddr.sin_family = AF_INET;
    decoderAddr.sin_port = htons(DECODER_PORT);
    decoderAddr.sin_addr.s_addr = inet_addr(DECODER_IP);

    printf("Server started. Receiver listening on port %d\n", RECEIVER_PORT);

    // Main loop to receive and forward packets
    while (1) {
        struct sockaddr_in senderAddr;
        int senderAddrSize = sizeof(senderAddr);
        char buffer[BUFFER_SIZE];
        int recvLen = recvfrom(receiverSocket, buffer, BUFFER_SIZE, 0, (struct sockaddr *)&senderAddr, &senderAddrSize);

        if (recvLen == SOCKET_ERROR) {
            printf("recvfrom() failed: %d\n", WSAGetLastError());
            continue;
        }

        if (!buffering) {
            // double elapsedTime = (double)(clock() - startTime) / CLOCKS_PER_SEC;
            // printf("Normal Operation Time: %.3f seconds\n", elapsedTime);
            // Forward packets directly to the decoder during initial normal operation
            result = sendto(senderSocket, buffer, recvLen, 0, (struct sockaddr *)&decoderAddr, sizeof(decoderAddr));
            if (result == SOCKET_ERROR) {
                printf("sendto() failed: %d\n", WSAGetLastError());
            }

            // Check if it's time to start buffering, kanode areba
            if (clock() - startTime >= START_BUFFERING_TIME * CLOCKS_PER_SEC / 1000) {
                double actualTime = (double)(clock()) / CLOCKS_PER_SEC; // Record the real time from the program
                printf("START_BUFFERING_TIME triggered at: %.3f seconds\n", actualTime);
                printf("Starting 2-second buffering period\n");
                buffering = 1;
                startBufferingTime = clock();
                startTime = clock();  // Reset start time for buffering duration
                bufferIndex = 0;      // Reset buffer index
                // but because of that yarai, not it can't be used. 
            }
        } else {
            // Buffer the incoming packets
            if (bufferIndex < MAX_PACKETS) {
                
                packetBuffer[bufferIndex].data = malloc(recvLen);
                if (packetBuffer[bufferIndex].data == NULL) {
                    printf("malloc() failed\n");
                    continue;
                }
                memcpy(packetBuffer[bufferIndex].data, buffer, recvLen);
                packetBuffer[bufferIndex].size = recvLen;
                bufferIndex++;
                lastBufferingTime = clock(); // Record the time of the last packet buffered
            } else {
                printf("Buffer overflow, discarding packet\n");
            }

            // Check if buffering duration is over
            if (clock() - startTime >= BUFFERING_DURATION * CLOCKS_PER_SEC / 1000) {
                double bufferingDuration = (double)(lastBufferingTime - startBufferingTime) / CLOCKS_PER_SEC;
                printf("Buffering took: %.3f seconds\n", bufferingDuration);
                printf("Last buffering time: %.3f seconds\n", (double)(lastBufferingTime) / CLOCKS_PER_SEC);
                //printf("Sending buffered packets\n");
                startSendingTime = clock(); // Record the start time for sending packets
                printf("Start sending time: %.3f seconds\n", (double)(startSendingTime) / CLOCKS_PER_SEC);
                // double gap = startSendingTime - lastBufferingTime;
                // printf("GAP: %.3f", gap);

                // Send buffered packets
                for (int i = 0; i < bufferIndex; i++) {
                    result = sendto(senderSocket, packetBuffer[i].data, packetBuffer[i].size, 0, (struct sockaddr *)&decoderAddr, sizeof(decoderAddr));
                    if (result == SOCKET_ERROR) {
                        printf("sendto() failed: %d\n", WSAGetLastError());
                    }
                    free(packetBuffer[i].data);
                }
                endSendingTime = clock(); // Record the end time for sending packets
                double sendingDuration = (double)(endSendingTime - startSendingTime) / CLOCKS_PER_SEC;
                printf("Sending duration: %.3f seconds\n", sendingDuration);
                // Clear the buffer
                bufferIndex = 0;

                // Switch back to normal operation
                buffering = 0;
                startTime = clock();  // Reset start time for the next buffering period
                printf("_________________________________________________\n");
            }
        }
    }

    // Cleanup
    closesocket(receiverSocket);
    closesocket(senderSocket);
    WSACleanup();
    return 0;
}
