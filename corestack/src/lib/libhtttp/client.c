#include "lib/libhtttp/client.h"
#include "common.h"
#include "message.h"

// private functions
int connectOnTCP(Sockets *socks, char *serverIp)
{
    LOG_I("[connectToServer()] Attempting connection to server at %s:%d...", serverIp, PORT_TCP);
    // sockaddr_in of server to connect to
    struct sockaddr_in serverAddr = {
        .sin_family = AF_INET,
        .sin_port = htons(PORT_TCP),
    };
    if (inet_pton(AF_INET, serverIp, &serverAddr.sin_addr) < 0) 
    {
        perror("[client connectToServer()] inet_pton");
        return -1;
    }
    int serverFd = connect(socks->tcp, (struct sockaddr *)&serverAddr, sizeof(serverAddr));
    if (serverFd < 0) 
    {
        perror("[client connectToServer()] connect");
        return -1;
    }
    LOG_I("[client connectToServer()] connection to server success");
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
        LOG_E("client: could not create endpoint struct for client");
        return -1;
    }

    LOG_I("[createClient()] new client created");
    *clientPtr = newClient;
    return 0;
}

// connects to a libhtttp server
int joinLobby(LibhtttpClient *clientPtr, char *ipAddress)
{
    Endpoint *thisClient = clientPtr;

    LOG_I("[client joinLobby()] attempting connection to lobby located at IP %s", ipAddress);
    if (createSockets(&thisClient->socks) < 0)
    {
        LOG_E("libhtttp/client createClient: socket creation failed");
        return -1;
    }

    if (connectOnTCP(thisClient->socks, ipAddress) < 0)
    {
        LOG_E("libhtttp/client joinLobby: failed to connect to server at IP");
        return -1;
    }

    if (registerWithServer(thisClient) < 0)
    {
        LOG_E("[joinLobby()] failed to register new client");
        return -1;
    }
    LOG_I("[joinLobby()] lobby joining complete");
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
        LOG_E("[sendAsClient()] sending has failed");
        goto fail;
    }

    LOG_I("[sendAsClient()] message has been sent");

    return 0;

    fail:
    free(msg.content);
    return -1;
}

// pending functions - message sending, receiving, lobby leaving