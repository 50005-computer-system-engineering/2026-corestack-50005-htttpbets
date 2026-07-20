#ifndef LIBHTTTP_MESSAGE_H
#define LIBHTTTP_MESSAGE_H

#include "common.h"

typedef struct {
    uint32_t sourceId;
    uint32_t length;
    unsigned char *content;
} Message;

// messaging functions
int receiveMessage(int sockfd, Message **returnPtr);
int sendMessage(int sockfd, Message completeMsg);
int getMessageContent(Message completeMsg, unsigned char **buffer);

// registration functions
int registerNewClient(Endpoint *newClient);
int registerWithServer(Endpoint *myClient);

#endif