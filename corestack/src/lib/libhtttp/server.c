#include <sys/poll.h>

#include "lib/libhtttp/server.h"
#include "common.h"
#include "message.h"
#include "clientll.h"

typedef struct {
    Endpoint *self;
    ClientLinkedList *clients;
} Server;

// private functions
int listenOnTCP(Server *serverPtr)
{
    // Check if sockets have valid fd 
    if (checkSockets(*(serverPtr->self->socks)) < 0)
    {
        LOG_E("[listenOnTCP()] sockets in Sockets invalid");
        return -1;
    }

    // Bind sockets to port 
    struct sockaddr_in serverAddr = {
        .sin_family = AF_INET,
        .sin_port = htons(PORT_TCP),
        .sin_addr.s_addr = INADDR_ANY
    };
    if (bind(serverPtr->self->socks->tcp, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) < 0)
    {
        perror("listenOnTCP bind");
        return -1;
    }

    // Listen on sockets for TCP connections 
    if (listen(serverPtr->self->socks->tcp, 100))
    {
        perror("listenOnTCP listen");
        return -1;
    }
    LOG_I("[listenOnTCP()]: server now listening for TCP connections on port %d", PORT_TCP);
    return 0;
}

int acceptOnTCP(Server *serverPtr)
{
    LOG_I("[acceptOnTCP()] server now accepting a TCP client...");
    int newClientFd = accept(serverPtr->self->socks->tcp, NULL, NULL);
    if (newClientFd < 0)
    {
        perror("acceptOnTCP accept");
        return -1;
    }
    LOG_I("[acceptOnTCP()] server accepted new TCP client");
    return newClientFd;
}

int prepareUnicastUDP(Server *serverPtr)
{
    LOG_I("[prepareUnicastUDP()] preparing UDP unicast port");

    // Bind sockets to port 
    struct sockaddr_in serverAddr = {
        .sin_family = AF_INET,
        .sin_port = htons(PORT_UDP_UNI),
        .sin_addr.s_addr = INADDR_ANY // any interface address
    };
    if (bind(serverPtr->self->socks->udpUni, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) < 0)
    {
        perror("[prepareUnicastUDP()] bind");
        return -1;
    }
    LOG_D("[prepareUnicastUDP()] UDP socket bound to port %d", PORT_UDP_UNI);

    LOG_I("[prepareUnicastUDP()] UDP unicast port ready to receive");

    return 0;
}

int prepareBroadcastUDP(Server *serverPtr)
{
    LOG_I("[prepareBroadcastUDP()] preparing UDP broadcast port");

    // setting socket options
    int broadcastEnabled = 1;
    if (setsockopt(serverPtr->self->socks->udpBroad, SOL_SOCKET, SO_BROADCAST, &broadcastEnabled, sizeof(broadcastEnabled)) < 0)
    {
        perror("[prepareBroadcastUDP()] setsockopt");
        return -1;
    }
    LOG_D("[prepareBroadcastUDP()] socket options set");

    LOG_I("[prepareBroadcastUDP()] UDP broadcast port ready for transmission");

    return 0;
}

int getFdSetTCP(Server *serverPtr, struct pollfd **clientFds)
{
    LOG_I("[getFdSetTCP()] getting a pollfd struct from server");

    // check if empty
    if (serverPtr->clients == NULL || serverPtr->clients->head == NULL)
    {
        LOG_E("[getFdSetTCP()] no clients connected to server");
        return -1;
    }

    // initialise struct
    struct pollfd *fds = malloc(sizeof(struct pollfd) * serverPtr->clients->count);
    if (fds == NULL)
    {
        perror("server malloc");
        return -1;
    }

    // add all client fd to polled fds
    struct pollfd *currentFd = fds;
    for (ClientNode *currentClient = serverPtr->clients->head; currentClient != NULL; currentClient = currentClient->next)
    {
        currentFd->fd = currentClient->client.socks->tcp;
        currentFd->events = POLLIN;
        currentFd->revents = 0;
        currentFd++;
    }

    *clientFds = fds;

    LOG_I("[getFdSetTCP()] added all clients to set");
    return 0;
}

// public functions
int createServer(LibhtttpServer **serverPtr)
{
    Server *newServer = malloc(sizeof(Server));
    if (newServer == NULL)
    {
        perror("server malloc");
        return -1;
    }
    if (createEndpoint(&newServer->self) < 0)
    {
        LOG_E("[createServer()] endpoint creation failed");
        goto fail;
    }
    newServer->clients = NULL;
    LOG_I("[createServer()] new server created with sockets created");

    // start listening until lobbySize of clients connect to server
    if (listenOnTCP(newServer) < 0)
    {
        LOG_E("[openLobby()] failed to bind port for listening");
        goto fail;
    }
    LOG_D("[createServer()] bound port for listening");

    // setup UDP ports
    if (prepareBroadcastUDP(newServer) < 0 || prepareUnicastUDP(newServer) < 0)
    {
        LOG_E("[openLobby()] failed to prepare UDP port for listening");
        goto fail;
    }
    LOG_D("[createServer()] UDP ports bound and ready for messaging");

    *serverPtr = newServer;
    return 0;

    fail:
    free(newServer);
    newServer = NULL;
    return -1;
}

int openLobby(LibhtttpServer *serverPtr, uint32_t *lobbySize, uint32_t **clientIds)
{
    LOG_I("[openLobby()] opening server lobby for clients...");

    // check parameters
    if (serverPtr == NULL)
    {
        LOG_E("[openLobby()] serverPtr has no server allocated");
        return -1;
    }
    if (*lobbySize < 1)
    {
        LOG_E("[openLobby()] lobbySize is required to be at least 1");
        return -1;
    }

    // allocate space for client array
    Server *thisServer = serverPtr;
    if (thisServer->clients != NULL)
    {
        // free and make a new array
        if (freeList(&thisServer->clients) < 0)
        {
            LOG_E("[openLobby()] could not free list for new lobby");
            return -1;
        }
        thisServer->clients = NULL;
    }
    thisServer->clients = malloc(sizeof(ClientLinkedList));
    if (thisServer->clients == NULL)
    {
        perror("server malloc");
        return -1;
    }
    LOG_D("[openLobby()] memory allocated for client linked list");

    // start accepting clients for TCP connections
    uint8_t slot = 1;   // ID 0 is reserved for server
    while (thisServer->clients->count < *lobbySize && slot != UINT32_MAX)
    {
        // blocks until client connects
        int clientFd = acceptOnTCP(thisServer);
        if (clientFd < 0)
        {
            LOG_E("[openLobby()] accept failed to find client, skipping loop iteration...");
            continue;
        }
        LOG_D("[openLobby()] accept succses, adding new client to slot %u/%u...", slot, *lobbySize);
        
        // creating client endpoint of newly connected client
        Endpoint newClient = {
            .id = slot,
            .token = "0000000",
            .socks = malloc(sizeof(Sockets))
        };
        newClient.socks->tcp = clientFd;
        if (registerNewClient(&newClient) < 0)
        {
            LOG_E("[openLobby()] failed to register new client");
            goto cleanup;
        }
        
        // add completed new client to the clients list
        if (addToList(thisServer->clients, &newClient))
        {
            LOG_E("[openLobby()] failed to add client to new list");
            goto cleanup;
        }

        slot++;
        continue;
        
        cleanup:
        free(newClient.socks);
        newClient.socks = NULL;
        // stack variable, cleared automatically I think
        continue;
    } // TODO implement halting lobby joining

    // return a list of players
    *lobbySize = thisServer->clients->count;
    if (getIdArray(thisServer->clients, clientIds) < 0)
    {
        LOG_E("[openLobby()] failed to write list of client ids");
        return -1;
    }
    LOG_D("[openLobby()] successfully returning array of %u player IDs", *lobbySize);

    LOG_I("[openLobby()] Lobby opening complete");
    return 0;
}

int closeLobby(LibhtttpServer *serverPtr)
{
    // TODO Implement close
    return 0;
}

int listenForClientMsg(LibhtttpServer *serverPtr, uint32_t *sourceId, unsigned char **returnBuffer)
{
    Server *thisServer = serverPtr;
    Message *returnMsg = NULL;

    // listening for multiple clients
    struct pollfd *clientFds;
    if (getFdSetTCP(thisServer, &clientFds) < 0)
    {
        LOG_E("[listenForClientMsg()] could not make a list of client socket fds");
        return -1;
    }
    int readyToRead = poll(clientFds, thisServer->clients->count, 3 * 60 * 1000);
    if (readyToRead <= 0)
    {
        LOG_E("[listenForClientMsg()] no message from any client within timeout period");
        return -1;
    }
    
    int temporarySoltuion = 0;  // TODO handle multiple clients
    for (int i=0; i < thisServer->clients->count || readyToRead != 0; i++)
    {
        if (clientFds[i].revents == POLLIN)
        {
            temporarySoltuion = clientFds[i].fd;
            readyToRead--;
            break;  // TODO handle multiple clients
        }
    }
    free(clientFds);

    // receive and return
    if (receiveMessage(temporarySoltuion, &returnMsg) < 0)
    {
        LOG_E("[listenForClientMsg()] failed to receive message");
        return -1;
    }

    *returnBuffer = malloc(returnMsg->length);
    if (*returnBuffer == NULL)
    {
        perror("malloc");
        // Free memory allocated for message
        // TODO: Move me to a function/goto so you don't copy and paste this?
        free(returnMsg->content);
        free(returnMsg);
        return -1;
    }

    *sourceId = returnMsg->sourceId;
    memcpy(*returnBuffer, returnMsg->content, returnMsg->length);

    // Free memory allocated for message
    free(returnMsg->content);
    free(returnMsg);

    LOG_I("[listenForClientMsg()] received message");
    return 0;
}

int sendBroadcastToClients(LibhtttpServer *serverPtr, uint32_t length, unsigned char *content)
{
    LOG_I("[sendBroadcastToClients()] sending broadcast to cliets...");

    // cast to private server struct
    Server *thisServer = serverPtr;

    // prepare the complete message
    Message completeMsg = {
        .sourceId = thisServer->self->id,
        .length = length,
        .content = content
    };
    LOG_D("[sendBroadcastToClients()] sending broadcast with header:\n\tsource: %u\n\tlength: %u", completeMsg.sourceId, completeMsg.length);

    // send as broadcast
    if (sendBroadcast(thisServer->self->socks->udpBroad, completeMsg))
    {
        LOG_E("[sendBroadcastToClients()] failed to send broadcast");
        return -1;
    }

    LOG_I("[sendBroadcastToClients()] broadcast has been sent");

    return 0;
}
