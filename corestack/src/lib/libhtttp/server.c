#include "lib/libhtttp/server.h"
#include "common.h"

typedef struct {
    Endpoint *self;
    Endpoint *clientList;
    // ClientLL *clients;   // TODO client linked list
} Server;

// private functions
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

int acceptOnTCP(Connection *socks)
{
    printf("acceptOnServer: server now accepting a TCP client...\n");
    int newClientFd = accept(socks->tcp, NULL, NULL);
    if (newClientFd < 0)
    {
        perror("acceptOnTCP accept");
        return -1;
    }
    printf("acceptOnTCP: server accepted new TCP client\n");
    return newClientFd;
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
    if (newEndpoint(&newServer->self) < 0)
    {
        printf("createServer: endpoint creation failed\n");
        goto fail;
    }
    printf("libhtttp/server createServer: new server created with sockets created\n");
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
        printf("libhtttp/server openLobby: serverPtr has no server allocated\n");
        return -1;
    }
    if (lobbySize < 1)
    {
        printf("libhtttp/server openLobby: lobbySize is required to be at least 1\n");
        return -1;
    }

    // allocate space for client array
    Server *thisServer = serverPtr;
    thisServer->clientList = malloc(sizeof(Record) * lobbySize);
    if (thisServer->clientList == NULL)
    {
        perror("libhtttp/server malloc");
        return -1;
    }

    // start listening until lobbySize of clients connect to server
    if (listenOnTCP(thisServer->socks))
    {
        printf("libhtttp/server openLobby: failed to bind port for listening\n");
        return -1;
    }

    // start accepting clients for TCP connections
    uint8_t slot = 1;   // ID 0 is reserved for server
    while (slot <= lobbySize)
    {
        // blocks until client connects
        int clientFd = acceptOnTCP(thisServer->socks);
        if (clientFd < 0)
        {
            printf("libhtttp/server openLobby: accept failed to find client, skipping loop iteration...\n");
            continue;
        }
        printf("libhtttp/server openLobby: accept succses, adding new client to slot %u/%u...\n", slot, lobbySize);
        
        // adding valid client record
        Record *currentSlot = &thisServer->clientList[slot - 1];
        currentSlot->id = slot;
        currentSlot->socks = malloc(sizeof(Connection));
        if (currentSlot->socks == NULL)
        {
            perror("server malloc");
            // TODO handling of this specific error
            return -1;
        }
        currentSlot->socks->tcp = clientFd;
        // if (registerNewClient(currentSlot) < 0)
        // {
        //     printf("libhtttp/server openLobby: failed to register new client\n");
        //     // TODO cleanup current slot
        //     continue;
        // }

        slot++;
    } // TODO implement halting lobby joining

    printf("libhtttp/server openLobby: Lobby opening complete");
    return 0;
}

int closeLobby(LibhtttpServer *serverPtr)
{
    // TODO Implement close
    return 0;
}
