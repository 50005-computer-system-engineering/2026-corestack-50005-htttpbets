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
        printf("[server listenOnTCP()] sockets in Sockets invalid\n");
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
    printf("[server listenOnTCP()]: server now listening for TCP connections on port %d\n", PORT_TCP);
    return 0;
}

int acceptOnTCP(Server *serverPtr)
{
    printf("[server acceptOnTCP()] server now accepting a TCP client...\n");
    int newClientFd = accept(serverPtr->self->socks->tcp, NULL, NULL);
    if (newClientFd < 0)
    {
        perror("acceptOnTCP accept");
        return -1;
    }
    printf("[server acceptOnTCP()] server accepted new TCP client\n");
    return newClientFd;
}

int getFdSetTCP(Server *serverPtr, struct pollfd **clientFds)
{
    printf("[server getFdSetTCP()] getting a pollfd struct from server\n");

    // check if empty
    if (serverPtr->clients == NULL || serverPtr->clients->head == NULL)
    {
        printf("[server getFdSetTCP()] no clients connected to server\n");
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
    for (ClientNode *currentClient = serverPtr->clients->head; currentClient->next != NULL; currentClient = currentClient->next)
    {
        currentFd->fd = currentClient->client.socks->tcp;
        currentFd->events = POLLIN;
        currentFd->revents = 0;
        currentFd++;
    }

    *clientFds = fds;

    printf("[server getFdSetTCP()] added all clients to set\n");
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
        printf("[server createServer()] endpoint creation failed\n");
        goto fail;
    }
    newServer->clients = NULL;
    printf("[server createServer()] new server created with sockets created\n");
    *serverPtr = newServer;
    return 0;

    fail:
    free(newServer);
    newServer = NULL;
    return -1;
}

int openLobby(LibhtttpServer *serverPtr, uint8_t lobbySize)
{
    // check parameters
    if (serverPtr == NULL)
    {
        printf("[server openLobby()] serverPtr has no server allocated\n");
        return -1;
    }
    if (lobbySize < 1)
    {
        printf("[server openLobby()] lobbySize is required to be at least 1\n");
        return -1;
    }

    // allocate space for client array
    printf("[server openLobby()] opening server lobby for clients...\n");
    Server *thisServer = serverPtr;
    if (thisServer->clients != NULL)
    {
        // free and make a new array
        if (freeList(&thisServer->clients) < 0)
        {
            printf("[server openLobby()] could not free list for new lobby\n");
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
    printf("[server openLobby()] memory allocated for client linked list\n");

    // start listening until lobbySize of clients connect to server
    if (listenOnTCP(thisServer) < 0)
    {
        printf("[server openLobby()] failed to bind port for listening\n");
        return -1;
    }
    printf("[server openLobby()] bound port for listening\n");

    // start accepting clients for TCP connections
    uint8_t slot = 1;   // ID 0 is reserved for server
    while (thisServer->clients->count < lobbySize && slot != UINT32_MAX)
    {
        // blocks until client connects
        int clientFd = acceptOnTCP(thisServer);
        if (clientFd < 0)
        {
            printf("[server openLobby()] accept failed to find client, skipping loop iteration...\n");
            continue;
        }
        printf("[server openLobby()] accept succses, adding new client to slot %u/%u...\n", slot, lobbySize);
        
        // creating client endpoint of newly connected client
        Endpoint newClient = {
            .id = slot,
            .token = "0000000",
            .socks = malloc(sizeof(Sockets))
        };
        newClient.socks->tcp = clientFd;
        if (registerNewClient(&newClient) < 0)
        {
            printf("[server openLobby()] failed to register new client\n");
            goto cleanup;
        }
        
        // add completed new client to the clients list
        if (addToList(thisServer->clients, &newClient))
        {
            printf("[server openLobby()] failed to add client to new list\n");
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

    printf("[server openLobby()] Lobby opening complete\n");
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
        printf("[server listenForClientMsg()] could not make a list of client socket fds\n");
        return -1;
    }
    int readyToRead = poll(clientFds, thisServer->clients->count, 3 * 60 * 1000);
    if (readyToRead <= 0)
    {
        printf("[server listenForClientMsg()] no message from any client within timeout period\n");
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
        printf("[server listenForClientMsg()] failed to receive message\n");
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

    printf("[server listenForClientMsg()] received message\n");
    return 0;
}
