#include "connection.h"

// Helper function for BOTH to check if file descriptor in Connection struct has been setup properly 
int checkSockets(Connection socks)
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

// Function if for BOTH to close sockets
int closeSockets(Connection *socks)
{
    if (checkSockets(*socks) == 0)
    {
        printf("closeSockets: sockets not created, no need to close\n");
        return -1;
    }
    close(socks->tcp);
    socks->tcp = -1;
    #ifdef UDP_INCLUDED
    close(socks->udp);
    socks->udp = -1;
    #endif
    printf("closeSockets: closed sockets\n");
    return 0;
}

// Function is for BOTH to setup socket file descriptors in Connection Struct
int createSockets(Connection **socks)
{
    // Create tcp socket descriptor 
    Connection *newSocks = malloc(sizeof(Connection)); 
    newSocks->tcp = socket(AF_INET, SOCK_STREAM, 0);
    if (newSocks->tcp < 0)
    {
        perror("createSockets socket");
        return -1;
    }

    #ifdef UDP_INCLUDED
    // create udp socket descriptor
    newSocks->udp = socket(AF_INET, SOCK_DGRAM, 0);
    if (newSocks->udp < 0)
    {
        perror("socket");
        return -1;
    }
    #endif
    printf("createSockets: socket file descriptors created\n");
    *socks = newSocks;
    return 0;
}

// Function is for SERVER to setup server for incoming client connections 
int listenOnTCP(Connection *socks)
{
    // Check if sockets have valid fd 
    if (checkSockets(*socks) < 0)
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
    if (bind(socks->tcp, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) < 0)
    {
        perror("listenOnServer bind");
        return -1;
    }

    // Listen on sockets for TCP connections 
    if (listen(socks->tcp, 100))
    {
        perror("listenOnServer listen");
        return -1;
    }
    printf("listenOnServer: server now listening for TCP connections on port %d\n", PORT_TCP);
    return 0;
}

// Function is for SERVER, accepts 1 client and returns them  
int acceptOnTCP(Connection *socks)
{
    printf("acceptOnServer: server now accepting a TCP client...\n");
    int newClientFd = accept(socks->tcp, NULL, NULL);
    if (newClientFd < 0)
    {
        perror("acceptOnServer accept");
        return -1;
    }
    printf("acceptOnServer: server accepted new TCP client\n");
    return newClientFd;
}

// CLIENT FUNCTIONS - LOBBY
int connectOnTCP(Connection *socks, char *serverIp)
{
    printf("connectToServer: Attempting connection to server at %s:%d...\n", serverIp, PORT_TCP);
    // sockaddr_in of server to connect to
    struct sockaddr_in serverAddr = {
        .sin_family = AF_INET,
        .sin_port = htons(PORT_TCP),
    };
    if (inet_pton(AF_INET, serverIp, &serverAddr.sin_addr) < 0) 
    {
        perror("connectToServer inet_pton");
        return -1;
    }
    int serverFd = connect(socks->tcp, (struct sockaddr *)&serverAddr, sizeof(serverAddr));
    if (serverFd < 0) 
    {
        perror("connectToServer connect");
        return -1;
    }
    printf("connectToServer: connection to server success\n");
    return serverFd;
}
