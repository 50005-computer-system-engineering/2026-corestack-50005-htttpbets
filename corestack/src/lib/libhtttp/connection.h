#ifndef LIBHTTTP_CONNECTION_H
#define LIBHTTTP_CONNECTION_H

#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>

// known ports
#define PORT_TCP 6700
#define PORT_UDP 6767

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

// connection functions
int createSockets(Connection *socks);
int closeSockets(Connection *socks);
int listenOnServer(Connection *socks);
int acceptOnServer(Connection *socks);
int openLobbyOnServer(Connection *socks, Record *clientList, int lobbySize);
int connectToServer(Connection *socks, char *serverIp);

#endif