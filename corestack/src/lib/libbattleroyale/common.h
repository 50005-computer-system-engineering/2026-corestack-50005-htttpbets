#ifndef LIBBR_COMMON_H
#define LIBBR_COMMON_H

#include <arpa/inet.h>
#include <netinet/in.h>
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <string.h>
#include <unistd.h>

#include "utils/logger.h"
#include "utils/queue.h"
#include "message.h"

// known ports
#define PORT_TCP 6700
#define PORT_UDP_UNI 6701
#define PORT_UDP_BROAD 6702

// important values
#define NONCE_LEN 8

// common structs throughout
typedef struct {
    int tcp;            // important singlecast
    int udpUni;      // unimportant singlecast
    int udpBroad;       // broadcasts
} Sockets;

typedef enum {
    IDLE,
    LOBBY,
    GAME,
    END
} EndpointState;

typedef struct {
    uint32_t id;                        // Own ID
    EndpointState state;
    Sockets *socks;                  // track sockets
    // MessageQueue messages;
} Endpoint;

// functions for sockets
int checkSockets(Sockets socks);
int createSockets(Sockets **socks);
int closeSockets(Sockets *socks);
int createEndpoint(Endpoint **endpt);
int closeEnpoint(Endpoint **endpt);

#endif