#ifndef LIBHTTTP_MESSAGE_H
#define LIBHTTTP_MESSAGE_H

#include <arpa/inet.h>
#include <unistd.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>


// message structs
typedef struct {
    uint32_t sourceId;       // member determines session token to be used (0 refers to server)
    uint32_t length;        // member specifies number of bytes of content to be received
    unsigned char *content; // raw bytes to be handled by functions or higher level protocol
} Message;
typedef void ClientRecord;

// send/recv functions
int readBytes(int sockfd, unsigned char **returnBuf, uint64_t length);
int sendBytes(int sockfd, const unsigned char *buf, uint64_t length);

// messaging functions
int receiveMessage(int sockfd, Message **returnPtr);
int sendMessage(int sockfd, Message completeMsg);

#endif 