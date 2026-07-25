#include "common.h"

// checks file descriptors in Sockets struct 
int checkSockets(Sockets socks)
{
    int tcpActive = socks.tcp > 0 ? 1 : 0;
    if (tcpActive == 0)
    {
        printf("[common checkSockets()] missing TCP socket\n");
        return 0;
    }
    int udpDirectActive = socks.udpDirect > 0 ? 1 : 0;
    if (udpDirectActive == 0)
    {
        printf("[checkSockets()] missing UDP direct socket\n");
        return 0;
    }
    int udpBroadActive = socks.udpBroad > 0 ? 1 : 0;
    if (udpBroadActive == 0)
    {
        printf("[checkSockets()] missing UDP broadcast socket\n");
        return 0;
    }
    return 1;
}

// closes both socket fd
int closeSockets(Sockets *socks)
{
    if (checkSockets(*socks) == 0)
    {
        printf("[common closeSockets()] sockets not created, no need to close\n");
        return -1;
    }
    close(socks->tcp);
    socks->tcp = -1;
    close(socks->udpDirect);
    socks->udpDirect = -1;
    close(socks->udpBroad);
    socks->udpBroad = -1;
    printf("[common closeSockets()] closed sockets\n");
    return 0;
}

// sets up socket fd for both
int createSockets(Sockets **socks)
{
    // Create tcp socket descriptor 
    Sockets *newSocks = malloc(sizeof(Sockets)); 
    newSocks->tcp = socket(AF_INET, SOCK_STREAM, 0);
    if (newSocks->tcp < 0)
    {
        perror("[common createSockets()] socket");
        goto fail;
    }

    // create udp direct socket descriptor
    newSocks->udpDirect = socket(AF_INET, SOCK_DGRAM, 0);
    if (newSocks->udpDirect < 0)
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
        close(newSocks->udpDirect);
        goto fail;
    }

    printf("[common createSockets()] socket file descriptors created\n");
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
    Endpoint *newEndpt = malloc(sizeof(Endpoint));
    if (newEndpt == NULL)
    {
        perror("[common createEndpoint()] malloc");
        return -1;
    }

    // assign some default values
    newEndpt->id = 0;
    memcpy(newEndpt->token, (unsigned char *)"0000000", NONCE_LEN);

    // create new sockets
    if (createSockets(&newEndpt->socks) < 0)
    {
        printf("createEndpoint: could not create sockets for endpoint\n");
        goto fail;
    }

    *endpt = newEndpt;
    return 0;

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
        printf("closeEndpoint: could not close sockets\n");
        return -1;
    } 
    free(closingEndpt->socks);
    closingEndpt->socks = NULL;

    // free struct memeory
    free(closingEndpt);
    closingEndpt = NULL;
    
    return 0;
}
