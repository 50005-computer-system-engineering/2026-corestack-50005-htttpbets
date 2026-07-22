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

int listenForClientMsg(LibhtttpServer *serverPtr, unsigned char **returnBuffer)
{
    Server *thisServer = serverPtr;

    Message *returnMsg = NULL;

    if (receiveMessage(thisServer->clients->head->client.socks->tcp, &returnMsg) < 0)
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

    memcpy(*returnBuffer, returnMsg->content, returnMsg->length);

    // Free memory allocated for message
    free(returnMsg->content);
    free(returnMsg);

    printf("[server listenForClientMsg()] received message\n");
    return 0;
}
