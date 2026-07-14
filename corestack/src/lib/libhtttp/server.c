#include "lib/libhtttp/server.h"
#include "connection.h"

int createServer(LibhtttpServer **serverPtr)
{
    Server *newServer = malloc(sizeof(Server));
    if (newServer == NULL)
    {
        perror("server malloc");
        return -1;
    }
    if (createSockets(&newServer->socks) < 0)
    {
        printf("libhtttp/server createServer: socket creation failed\n");
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
    if (serverPtr != NULL)
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
        Record *current_slot = &thisServer->clientList[slot - 1];
        current_slot->id = slot;
        current_slot->token = 0; // TODO nonce generation
        current_slot->socks = malloc(sizeof(Connection));
        if (current_slot->socks == NULL)
        {
            perror("server malloc");
            // TODO handling of this specific error
            return -1;
        }
        current_slot->socks->tcp = clientFd;
        // TODO registration (sending them their ID and token)

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
