#ifndef LIBBR_MESSAGE_H
#define LIBBR_MESSAGE_H

#include "common.h"

#define MSG_CONTENT_LENGTH 512

typedef enum {
    MSG_JOIN,
    MSG_LEAVE,
    MSG_START,
    MSG_APP
} MessageType;

typedef struct {
    uint32_t sourceId;
    MessageType msgType;
    unsigned char msgContent[MSG_CONTENT_LENGTH];
} Message;

DEFINE_QUEUE(Message, Message, 500);

// byte reading functions
int readBytes(int sockfd, unsigned char **returnBuf, uint64_t length);
int sendBytes(int sockfd, const unsigned char *buf, uint64_t length);

// messaging functions
int receiveMessageTCP(int sockfd, Message *returnPtr);
int receiveMessageUDP(int sockfd, Message *returnPtr);
int sendMessageTCP(int sockfd, const Message completeMsg);
int sendBroadcastUDP(int sockfd, const Message completeMsg);

#endif