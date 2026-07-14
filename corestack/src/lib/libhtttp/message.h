#ifndef LIBHTTTP_MESSAGE_H
#define LIBHTTTP_MESSAGE_H

#include <arpa/inet.h>
#include <unistd.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>

// message codes
#define MSG_REG 0       // registration related
#define MSG_DEREG 1     // deregistering client
#define MSG_ADMIN 2     // kicking, banning, start game, etc. (libhtttp actions)
#define MSG_APP 3       // for higher level protocol used built on HTTTP (Tetris/Bomberman)

// message structs
typedef struct {
    uint8_t sourceId;       // member determines session token to be used (0 refers to server)
    uint8_t type;           // member would determine how the receiver handles the content
    uint32_t length;        // member specifies number of bytes of content to be received
    unsigned char *content; // raw bytes to be handled by functions or higher level protocol
} Message;

// messaging functions
int receiveMessage(int sockfd, Message **returnPtr);
int sendMessage(int sockfd, Message completeMsg);

#endif 