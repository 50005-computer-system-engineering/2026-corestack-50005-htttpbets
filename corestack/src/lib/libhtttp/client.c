#include "lib/libhtttp/client.h"
#include "common.h"
#include "message.h"

// private functions
int connectOnTCP(Sockets *socks, char *serverIp)
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

// public functions
// allows developers to create a libhtttp client in application
int createClient(LibhtttpClient **clientPtr)
{
    Endpoint *newClient = NULL;

    // create endpoint
    if (createEndpoint(&newClient) < 0)
    {
        printf("client: could not create endpoint struct for client\n");
        return -1;
    }

    printf("libhtttp/client createClient: new client created\n");
    *clientPtr = newClient;
    return 0;
}

// connects to a libhtttp server
int joinLobby(LibhtttpClient *clientPtr, char *ipAddress)
{
    Endpoint *thisClient = clientPtr;

    if (createSockets(&thisClient->socks) < 0)
    {
        printf("libhtttp/client createClient: socket creation failed\n");
        return -1;
    }

    if (connectOnTCP(thisClient->socks, ipAddress) < 0)
    {
        printf("libhtttp/client joinLobby: failed to connect to server at IP\n");
        return -1;
    }

    // if (registerWithServer(thisClient))
    // {
    //     printf("libhtttp/client joinLobby: failed to register\n");
    //     return -1;
    // }
    return 0;
}

// message functions
int sendAsClient(LibhtttpClient *clientPtr, uint32_t length, unsigned char *content)
{
    Endpoint *thisClient = clientPtr;

    // build message
    Message msg = {
        .sourceId = thisClient->id,
        .length = length,
        .content = malloc(length)
    };
    if (msg.content == NULL)
    {
        perror("sendAsClient malloc");
        return -1;
    }
    
    // add content to message
    memcpy(msg.content, content, length);

    // send via socket
    if (sendMessage(thisClient->socks->tcp, msg) < 0)
    {
        printf("sendAsClient: sending has failed\n");
        goto fail;
    }

    printf("sendAsClient: message has been sent\n");

    return 0;

    fail:
    free(msg.content);
    return -1;
}

// pending functions - message sending, receiving, lobby leaving