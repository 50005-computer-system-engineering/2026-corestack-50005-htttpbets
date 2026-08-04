#ifndef LIBHTTTP_MESSAGE_H
#define LIBHTTTP_MESSAGE_H

#include "common.h"

typedef enum {
    REQ_ADMIN,
    REQ_JOIN,
    REQ_LEAVE,
    REQ_START,
    REQ_UPDATE // replaces move, rotate, drop, state (these instructions are handled by upper layer application)
    // TODO add response codes
} MessageCode;

typedef struct {
    uint32_t sourceId;
    uint32_t pathLen;
    unsigned char *path`;
} Message;

// messaging functions
int receiveMessage(int sockfd, Message **returnPtr);
int sendMessage(int sockfd, Message completeMsg);
int receiveBroadcast(int sockfd, Message **returnPtr);
int sendBroadcast(int sockfd, Message completeMsg);

// registration functions
// TODO remove
int registerNewClient(Endpoint *newClient);
int registerWithServer(Endpoint *myClient);

#endif