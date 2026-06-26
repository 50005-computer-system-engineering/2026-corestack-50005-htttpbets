#ifndef CONNECTION_H
#define CONNECTION_H

#include <arpa/inet.h>
#include <unistd.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <stdio.h>

// known ports
#define PORT_TCP 6700
#define PORT_UDP 6767

// misc
#define MAX_CONNECTIONS 100

// structs 
typedef struct {
    int tcp;     // member (tcp socket fd)
    #ifdef UDP_INCLUDED
    int udp;     // member (udp socket fd), only if present
    #endif
} Connection;

int createSockets(Connection *socks);
int closeSockets(Connection *socks);
int listenOnServer(Connection *socks);
int acceptOnServer(Connection *socks);
int connectToServer(Connection *socks, char *serverIp);

#endif