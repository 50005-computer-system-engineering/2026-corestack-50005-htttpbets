#ifndef LIBHTTTP_COMMON_H
#define LIBHTTTP_COMMON_H

#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <string.h>

#include "liblog.h"

// known ports
#define PORT_TCP 6700
#define PORT_UDP_DIRECT 6701
#define PORT_UDP_BROAD 6702

// important values
#define NONCE_LEN 8

// common structs throughout
typedef struct {
    int tcp;            // important singlecast
    int udpDirect;      // unimportant singlecast
    int udpBroad;       // broadcasts
} Sockets;

typedef struct {
    uint32_t id;                        // Own ID
    unsigned char token[NONCE_LEN];     // token used for communication during session
    Sockets *socks;                  // track sockets
} Endpoint;

// functions for sockets
int checkSockets(Sockets socks);
int createSockets(Sockets **socks);
int closeSockets(Sockets *socks);
int createEndpoint(Endpoint **endpt);
int closeEnpoint(Endpoint **endpt);

#endif