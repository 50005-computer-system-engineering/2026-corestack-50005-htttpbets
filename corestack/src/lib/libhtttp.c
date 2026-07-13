#include "lib/libhtttp.h"

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
    socks->udp = socket(AF_INET, SOCK_DGRAM, 0);
    if (socks->udp < 0)
    {
        perror("socket");
        return -1;
    }
    #endif
    printf("createSockets: socket file descriptors created\n");
    return 0;
}

// SERVER FUNCTIONS - LOBBY
// Function is for SERVER to setup server for incoming client connections 
int listenOnServer(Connection *socks)
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
int acceptOnServer(Connection *socks)
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

// Function is for SERVER and creates a Client struct
Record buildRecord(int tcpSockFd, uint8_t id)
{
    Record newRecord;
    newRecord.id = id;
    newRecord.token = 0;   // TODO unique token generation 
    Connection newRecordSocks = {
        .tcp = tcpSockFd
    };
    newRecord.connection = newRecordSocks;
    return newRecord;
}

// Function is exposed to SERVER to accept specified lobbySize of clients
int openLobbyOnServer(Connection *socks, Record *clientArray, int lobbySize)
{
    // input checks
    if (lobbySize < 1)
    {
        printf("openLobbyOnServer: invalid value for lobbySize, must 1 or more\n");
        return -1;
    }
    if (checkSockets(*socks) < 0)
    {
        printf("openLobbyOnServer: socks struct pointer invalid\n");
        return -1;
    }
    if (!clientArray)
    {
        printf("openLobbyOnServer: requires an array with memory allocated\n");
        return -1;
    }

    // loop to add clients
    printf("openLobbyOnServer: server now accepting %d clients into lobby...\n", lobbySize);
    uint8_t slot = 1;   // id 0 reserved for server
    while (slot <= lobbySize)
    {
        int newClientFd = acceptOnServer(socks);
        if (newClientFd < 0)
        {
            printf("openLobbyOnServer: client with invalid socket fd\n");
            continue;   // don't add this client to the clientArray
        }
        clientArray[slot - 1] = buildRecord(newClientFd, slot);
        printf("openLobbyServer: client accepted into client list\n");
        slot++;
    }
    printf("openLobbyOnServer: server done accepting clients\n");
    return 0;
}

// CLIENT FUNCTIONS - LOBBY
int connectToServer(Connection *socks, char *serverIp)
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

// MESSAGE FUNCTIONS
// same functions as in common.c in PA2, not exposed to library users
int readBytes(int sockfd, unsigned char **returnBuf, uint64_t length)
{
    // allocate space for the return buffer
    if (*returnBuf != NULL)
    {
        free(*returnBuf);
        *returnBuf = NULL;
    }
    *returnBuf = malloc(length);
    if (*returnBuf == NULL)
    {
        perror("readBytes malloc");
        return -1;
    }

    // loop to receive length number of bytes and write to buffer
    uint64_t bytesReceived = 0;
    while (bytesReceived < length)
    {
        uint64_t remaining = length - bytesReceived;
        ssize_t n = recv(sockfd, *returnBuf + bytesReceived, (size_t)remaining, 0);
        if (n <= 0)
        {
            perror("readBytes recv");
            free(*returnBuf);
            *returnBuf = NULL;
            return -1;
        }
        bytesReceived += (uint64_t)n;
    }

    return 0;
}

int sendBytes(int sockfd, const unsigned char *buf, uint64_t length)
{
    // loop to read length number of bytes from buf pointer location
    uint64_t totalSent = 0;
    while (totalSent < length)
    {
        uint64_t remaining = length - totalSent;
        ssize_t m = send(sockfd, buf + totalSent, (size_t)remaining, 0);
        totalSent += (uint64_t)m;
    }
    return 0;
}

// functions to send and receive complete messages
int receiveMessage(int sockfd, Message **returnPtr)
{
    Message *returnMsg = malloc(sizeof(Message));
    if (returnMsg == NULL)
    {
        perror("readBytes malloc");
        goto fail;
    }

    // read Message header bytes
    printf("receiveMessage: allocated memory for Message, listening for header...\n");
    unsigned char *buffer = NULL;
    if (readBytes(sockfd, &buffer, sizeof(uint8_t)) < 0)
    {
        printf("readMessage: failed to read sourceId\n");
        goto fail;
    } 
    returnMsg->sourceId = *buffer;
    free(buffer);
    buffer = NULL;

    if (readBytes(sockfd, &buffer, sizeof(uint8_t)) < 0)
    {
        printf("readMessage: failed to read type\n");
        goto fail;
    } 
    returnMsg->type = *buffer;
    free(buffer);
    buffer = NULL;

    if (readBytes(sockfd, &buffer, sizeof(uint32_t)) < 0)
    {
        printf("readMessage: failed to read length\n");
        goto fail;
    } 
    uint32_t lenBytes;
    memcpy(&lenBytes, buffer, sizeof(lenBytes));
    returnMsg->length = ntohl(lenBytes);
    free(buffer);
    buffer = NULL;

    // listen for message considering the header
    printf("readMessage: now listening for message with header:\n\tsourceId: %u\n\ttype: %u\n\tlength: %u\n", returnMsg->sourceId, returnMsg->type, returnMsg->length);
    if (readBytes(sockfd, &buffer, returnMsg->length) < 0)
    {
        printf("readMessage: failed to read length\n");
        goto fail;
    }
    returnMsg->content = malloc(returnMsg->length);
    if (returnMsg->content == NULL)
    {
        perror("receiveMessage malloc");
        goto fail;
    }
    memcpy(returnMsg->content, buffer, returnMsg->length);
    free(buffer);
    buffer = NULL;

    // return this ptr
    *returnPtr = returnMsg;

    return 0;

fail:
    free(buffer);
    if (returnMsg != NULL)
    {
        if (returnMsg->content != NULL)
        {
            free(returnMsg->content);
        }
        free(returnMsg);
    }

    return -1;
}

int sendMessage(int sockfd, const Message completeMsg)
{
    printf("sendMessage: sending message with the header:\n\tsourceId: %u\n\ttype: %u\n\tlength: %u\n", completeMsg.sourceId, completeMsg.type, completeMsg.length);
    sendBytes(sockfd, &completeMsg.sourceId, sizeof(uint8_t));
    sendBytes(sockfd, &completeMsg.type, sizeof(uint8_t));
    uint32_t length = htonl(completeMsg.length);
    sendBytes(sockfd, (const unsigned char *)&length, sizeof(uint32_t));
    sendBytes(sockfd, completeMsg.content, completeMsg.length);
    printf("sendMessage: message sending probably worked\n");
    return 0;
}