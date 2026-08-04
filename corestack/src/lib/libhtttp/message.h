#ifndef LIBHTTTP_MESSAGE_H
#define LIBHTTTP_MESSAGE_H

#include "common.h"

typedef struct {
    uint32_t sourceId;
    uint32_t length;
    unsigned char *content;
} Message;

// byte reading functions
int readBytes(int sockfd, unsigned char **returnBuf, uint64_t length);
int sendBytes(int sockfd, const unsigned char *buf, uint64_t length);

// messaging functions
int receiveMessageTCP(int sockfd, Message **returnPtr);
int receiveMessageUDP(int sockfd, Message **returnPtr);
int sendMessageTCP(int sockfd, const Message completeMsg);
int sendMessageUDP(int sockfd, const Message completeMsg);

#endif