#include "connection.h"

// Helper function for BOTH to check if file descriptor in Connection struct has been setup properly 
int checkSocks(Connection socks)
{
    int tcpActive = socks.tcp > 0 ? 1 : 0;
    if (tcpActive == 0)
    {
        printf("checkSockets: missing TCP socket\n");
        return 0;
    }
    #ifdef UDP_INCLUDED
    int udpActive = socks.udp > 0 ? 1 : 0;
    if (udpActive == 0)
    {
        printf("checkSockets: missing UDP socket\n");
        return 0;
    }
    #endif
    return 1;
}

// Function is for BOTH to setup socket file descriptors in Connection Struct
int createSockets(Connection *socks)
{
    // Create tcp socket descriptor 
    socks->tcp = socket(AF_INET, SOCK_STREAM, 0);
    if (socks->tcp < 0)
    {
        perror("createSockets socket");
        return -1;
    }

    #ifdef UDP_INCLUDED
    // create udp socket descriptor
    socks.udp = socket(AF_INET, SOCK_DGRAM, 0);
    if (socks.udp < 0)
    {
        perror("socket");
        return -1;
    }
    #endif
    printf("createSockets: socket file descriptors created");
    return 0;
}

// Function is for SERVER to setup server for incoming client connections 
int listenOnServer(Connection socks)
{
    // Check if sockets have valid fd 
    if (checkSocks(socks) < 0)
    {
        printf("listenOnSever: sockets in Connection invalid\n");
        return -1;
    }

    // Bind sockets to port 
    struct sockaddr_in serverAddr = {
        .sin_family = AF_INET,
        .sin_port = htons(PORT_TCP),
        .sin_addr.s_addr = INADDR_ANY
    };
    if (bind(socks.tcp, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) < 0)
    {
        perror("listenOnServer bind");
        return -1;
    }

    // Listen on sockets for TCP connections 
    if (listen(socks.tcp, 100))
    {
        perror("listenOnServer listen");
        return -1;
    }
    printf("listenOnServer: server now listening for TCP connections on port %d\n", PORT_TCP);
    return 1;
}

// Function is for SERVER, accepts 1 client and returns them  
int acceptOnServer(Connection socks)
{
    int newClientFd = accept(socks.tcp, NULL, NULL);
    if (newClientFd < 0)
    {
        perror("acceptOnServer accept");
        return -1;
    }
    return newClientFd;
}