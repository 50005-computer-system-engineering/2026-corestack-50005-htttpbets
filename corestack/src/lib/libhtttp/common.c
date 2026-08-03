#include "common.h"

// checks file descriptors in Sockets struct 
int checkSockets(Sockets socks)
{
    int tcpActive = socks.tcp > 0 ? 1 : 0;
    if (tcpActive == 0)
    {
        LOG_E("[checkSockets()] missing TCP socket");
        return 0;
    }
    int udpUniActive = socks.udpUni > 0 ? 1 : 0;
    if (udpUniActive == 0)
    {
        LOG_E("[checkSockets()] missing UDP direct socket");
        return 0;
    }
    int udpBroadActive = socks.udpBroad > 0 ? 1 : 0;
    if (udpBroadActive == 0)
    {
        LOG_E("[checkSockets()] missing UDP broadcast socket");
        return 0;
    }
    return 1;
}

// closes both socket fd
int closeSockets(Sockets *socks)
{
    LOG_I("[closeSockets()] closing sockets...");
    if (checkSockets(*socks) == 0)
    {
        LOG_E("[closeSockets()] sockets not created, no need to close");
        return -1;
    }
    close(socks->tcp);
    socks->tcp = -1;
    close(socks->udpUni);
    socks->udpUni = -1;
    close(socks->udpBroad);
    socks->udpBroad = -1;
    LOG_I("[closeSockets()] closed sockets successfully");
    return 0;
}

// sets up socket fd for both
int createSockets(Sockets **socks)
{
    LOG_I("[createSockets()] creating sockets...");
    // Create tcp socket descriptor 
    Sockets *newSocks = malloc(sizeof(Sockets)); 
    newSocks->tcp = socket(AF_INET, SOCK_STREAM, 0);
    if (newSocks->tcp < 0)
    {
        perror("[common createSockets()] socket");
        goto fail;
    }

    // create udp direct socket descriptor
    newSocks->udpUni = socket(AF_INET, SOCK_DGRAM, 0);
    if (newSocks->udpUni < 0)
    {
        perror("[common createSockets()] socket");
        close(newSocks->tcp);
        goto fail;
    }

    // create udp broadcast socket descriptor
    newSocks->udpBroad = socket(AF_INET, SOCK_DGRAM, 0);
    if (newSocks->udpBroad < 0)
    {
        perror("[common createSockets()] socket");
        close(newSocks->tcp);
        close(newSocks->udpUni);
        goto fail;
    }

    LOG_I("[createSockets()] socket file descriptors created");
    *socks = newSocks;
    return 0;
    fail:
    free(newSocks);
    newSocks = NULL;
    return -1;
}

// creates a new endpoint with sockets set up
int createEndpoint(Endpoint **endpt)
{
    LOG_I("[createEndpoint()] creating Endpoint...");
    Endpoint *newEndpt = malloc(sizeof(Endpoint));
    if (newEndpt == NULL)
    {
        perror("[common createEndpoint()] malloc");
        return -1;
    }

    // assign some default values
    newEndpt->id = 0;
    // memcpy(newEndpt->token, (unsigned char *)"0000000", NONCE_LEN);

    newEndpt->state = IDLE;

    // create new sockets
    if (createSockets(&newEndpt->socks) < 0)
    {
        LOG_E("[createEndpoint()] could not create sockets for endpoint");
        goto fail;
    }

    // create a message queue
    newEndpt->messages = MessageQueue {
        .front = NULL,
        .back = NULL
    };

    *endpt = newEndpt;
    return 0;

    LOG_I("[createEndpoint()] endpoint created successfully");
    fail:
    free(newEndpt);
    return -1;
}

// smoothly close an endpoint
int closeEnpoint(Endpoint **endpt)
{
    Endpoint *closingEndpt = *endpt;
    // close sockets
    if (closeSockets(closingEndpt->socks) < 0)
    {
        LOG_E("[closeEndpoint()] could not close sockets");
        return -1;
    } 
    free(closingEndpt->socks);
    closingEndpt->socks = NULL;

    // free struct memeory
    free(closingEndpt);
    closingEndpt = NULL;
    
    LOG_I("[closeEndpoint()] endpoint closed successfully");
    
    return 0;
}
