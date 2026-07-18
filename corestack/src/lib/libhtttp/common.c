#include "common.h"

// checks file descriptors in Sockets struct 
int checkSockets(Sockets socks)
{
    int tcpActive = socks.tcp > 0 ? 1 : 0;
    if (tcpActive == 0)
    {
        printf("checkSockets: missing TCP socket\n");
        return 0;
    }
    int udpActive = socks.udp > 0 ? 1 : 0;
    if (udpActive == 0)
    {
        printf("checkSockets: missing UDP socket\n");
        return 0;
    }
    return 1;
}

// closes both socket fd
int closeSockets(Sockets *socks)
{
    if (checkSockets(*socks) == 0)
    {
        printf("closeSockets: sockets not created, no need to close\n");
        return -1;
    }
    close(socks->tcp);
    socks->tcp = -1;
    close(socks->udp);
    socks->udp = -1;
    printf("closeSockets: closed sockets\n");
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
        perror("createSockets socket");
        goto fail;
    }

    // create udp socket descriptor
    newSocks->udp = socket(AF_INET, SOCK_DGRAM, 0);
    if (newSocks->udp < 0)
    {
        perror("socket");
        goto fail;
    }

    printf("createSockets: socket file descriptors created\n");
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
        perror("createEndpoint malloc");
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