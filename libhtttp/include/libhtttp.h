#ifndef LIBHTTTP_H
#define LIBHTTTP_H

#include <arpa/inet.h>
#include <endian.h>
#include <unistd.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>

// known ports
#define PORT_TCP 6700
#define PORT_UDP 6767
// message codes
#define MSG_REG 0       // registration related
#define MSG_DEREG 1     // deregistering client
#define MSG_ADMIN 2     // kicking, banning, start game, etc. (libhtttp actions)
#define MSG_APP 3       // for higher level protocol used built on HTTTP (Tetris/Bomberman)

// misc
#define MAX_CONNECTIONS 100

// connection structs 
typedef struct {
    int tcp;     // member (tcp socket fd)
    #ifdef UDP_INCLUDED
    int udp;     // member (udp socket fd), only if present
    #endif
} Connection;
typedef struct {
    uint8_t id;             // on server it is record of client ID, on client it is record of own ID
    int token;              // will replace with proper token with encryption later on
    Connection connection;  // member is 
} Record;   // struct used to track connections by BOTH
typedef struct {
    Connection socks;           // member tracks own TCP and UDP sockets
    uint8_t numClients;         // member tracks number of clients
    Record *clientList;   // member is list of clients according to id
} Server;
// message structs
typedef struct {
    uint8_t sourceId;       // member determines session token to be used (0 refers to server)
    uint8_t type;           // member would determine how the receiver handles the content
    uint32_t length;        // member specifies number of bytes of content to be received
    unsigned char *content; // raw bytes to be handled by functions or higher level protocol
} Message;

// connection functions
int createSockets(Connection *socks);
int closeSockets(Connection *socks);
int listenOnServer(Connection *socks);
int acceptOnServer(Connection *socks);
int openLobbyOnServer(Connection *socks, Record *clientList, int lobbySize);
int connectToServer(Connection *socks, char *serverIp);

// // messaging functions
// int startTcpListener(Connection *socks);
// int buildHtttpMessage(Connection *socks);
// int sendHtttpMesssage(Connection *socks);

#endif