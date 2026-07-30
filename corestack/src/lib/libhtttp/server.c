#include <sys/poll.h>

#include "lib/libhtttp/server.h"
#include "common.h"
#include "message.h"
#include "clientll.h"

typedef struct {
    Endpoint *self;
    ClientLinkedList *clients;
    EndpointState state;
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

// private state-based functions
/* 
IDLE state where it makes thread busy wait for admin administered state change 
Does not take any HTTTP packets in this state
*/
void idleStateLoop(Server *serverPtr)    // budy waits for next action
{
    LOG_I("[idleStateLoop()] SERVER entering IDLE state, awaiting instructions...");
    while (serverPtr->self->state == IDLE)
    {
        // busy wait
        continue;
    }
    LOG_I("[idleStateLoop()] state change detected, SERVER exiting IDLE state");
    return;
}

/*
LOBBY state where clients are allowed to send JOIN, LEAVE HTTTP packets
During this state, TCP connections from client are accepted (and server assigns player ID)
*/ 
void lobbyStateLoop(Server *serverPtr)   // loop where server accepts clients
{
    LOG_I("[lobbyStateLoop()] SERVER entering LOBBY state, accepting clients...");
    
    // prepare client list (upon entering lobby state)
    LOG_D("[lobbyStateLoop()] allocating space for client list on SERVER");
    if (serverPtr->clients != NULL)
    {
        // free and make a new array
        if (freeList(&serverPtr->clients) < 0)
        {
            LOG_E("[lobbyStateLoop()] could not free list for new lobby");
            return;
        }
        serverPtr->clients = NULL;
    }
    serverPtr->clients = malloc(sizeof(ClientLinkedList));
    if (serverPtr->clients == NULL)
    {
        perror("[lobbyStateLoop()] malloc");
        return;
    }
    LOG_D("[lobbyStateLoop()] memory allocated for client linked list");

    // tracking the id assigned
    int prevAssignedId = 0; 

    // preparing TCP socket for polling
    struct pollfd acceptPort;
    acceptPort.fd = serverPtr->self->socks->tcp;   // socket to start connection with the poll
    acceptPort.events = POLLIN;
    acceptPort.revents = 0;

    // while in this state, keep accepting clients
    while (serverPtr->self->state == LOBBY)
    {
        // poll for a client
        if (poll(&acceptPort, 1, 50) <= 0)  // timeout configured to prevent perma-blocking
        {
            continue; // skip if nothing
        }

        // accept a client if the socket has any POLLIN activity
        LOG_D("[lobbyStateLoop()] polled activity, accpeting a client");
        int clientFd = acceptOnTCP(serverPtr);
        if (clientFd < 0)
        {
            LOG_E("[lobbyStateLoop()] accept failed to find client, skipping loop iteration...");
            continue;
        }
        LOG_D("[lobbyStateLoop()] accept succses, assigning player id %u to new connection", ++prevAssignedId);
        
        // creating client endpoint of newly connected client
        Endpoint newClient = {
            .id = prevAssignedId,
            .socks = malloc(sizeof(Sockets))
        };
        newClient.socks->tcp = clientFd;
        if (registerNewClient(&newClient) < 0)
        {
            LOG_E("[lobbyStateLoop()] failed to register new client");
            goto cleanup;
        }
        
        // add completed new client to the clients list
        if (addToList(serverPtr->clients, &newClient))
        {
            LOG_E("[lobbyStateLoop()] failed to add client to new list");
            goto cleanup;
        }
        LOG_D("[lobbyStateLoop()] finished sending the new client their ID");

        continue;
        
        cleanup:
        free(newClient.socks);
        newClient.socks = NULL;
        LOG_E("[lobbyStateLoop()] failed to make new client, dropping it");

        continue;
    }

    LOG_I("[lobbyStateLoop()] state change detected, SERVER exiting LOBBY state");
    return;
}

/*
GAME state where it it listens for ACTION packets and sends STATE packets
At start of state state, all clients are sent an START packet to change their state into a game state
During this state, clients may still choose to LIST
To end the state, END packet is sent to change its state again
*/
void gameStateLoop(Server *serverPtr)    // loop where listens for unicast from clients
{
    LOG_I("[gameStateLoop()] SERVER entering GAME state, prerparing to listen for messages...");

    // check if there are any players
    if (serverPtr->clients->count <= 0)
    {
        LOG_E("[gameStateLoop()] SERVER has no clients connected, reverting to lobby state...");
        serverPtr->self->state = LOBBY;
        return;
    }

    // setup pollfd array for clients being listened to
    struct pollfd *listenFdTCP = malloc(sizeof(struct pollfd) * serverPtr->clients->count);
    struct pollfd *currentFd = listenFdTCP;
    // loop through server's clients
    for (ClientNode *currentClient = serverPtr->clients->head; currentClient != NULL; currentClient = currentClient->next)
    {
        currentFd->fd = currentClient->client.socks->tcp;
        currentFd->events = POLLIN;
        currentFd->revents = 0;
        currentFd++;
    }
    struct pollfd listenFdUDP;
    listenFdUDP.fd = serverPtr->self->socks->udpUni;
    listenFdUDP.events = POLLIN;
    listenFdUDP.revents = 0;

    // while the GAME state is active, monitor for messages to each port
    while (serverPtr->self->state != GAME)
    {
        // poll for any tcp activity
        int socketActivity = poll(&listenFdTCP, serverPtr->clients->count, 50);
        if (socketActivity <= 0)  // timeout configured to prevent perma-blocking
        {
            continue; // skip if nothing
        }
        
        // listen to each message and handle
        // TODO message buffer for high traffic situation
        // FOR NOW handles 1 message at a time for testing and basic functionality
        Message *msg = NULL;
        uint32_t sourceId = 0;
        for (uint32_t i=0; i < serverPtr->clients->count || socketActivity > 0; i++)
        {
            if (clientFds[i].revents == POLLIN)
            {
                int fd = clientFds[i].fd;
                readyToRead--;
                if (receiveMessage(fd, &msg) < 0)
                {
                    LOG_E("[gameStateLoop()] could not read message from TCP socket fd %d", fd);
                    continue;
                }
                LOG_D("[gameStateLoop()] received message:\n\tsource: %u\n\tlength: %u\n\tcontent: %s", msg->sourceId, msg->length, msg->content);
            }
        }

    }
}

void endStateCleanup(Server *serverPtr)
{
    
}

// public functions
int createServer(LibhtttpServer **serverPtr)
{
    // allocate memory for it
    Server *newServer = malloc(sizeof(Server));
    if (newServer == NULL)
    {
        perror("server malloc");
        return -1;
    }

    LOG_I("[createServer()] creating HTTTP server...");

    // create the endpoint
    if (createEndpoint(&newServer->self) < 0)
    {
        LOG_E("[createServer()] endpoint creation failed");
        goto fail;
    }
    newServer->clients = NULL;
    LOG_I("[createServer()] new server created with sockets created");

    // prepare ports for usage
    if (listenOnTCP(newServer) < 0)
    {
        LOG_E("[openLobby()] failed to bind port for listening");
        goto fail;
    }
    LOG_D("[createServer()] bound port for listening");
    if (prepareBroadcastUDP(newServer) < 0 || prepareUnicastUDP(newServer) < 0)
    {
        LOG_E("[openLobby()] failed to prepare UDP port for listening");
        goto fail;
    }
    LOG_D("[createServer()] UDP ports bound and ready for messaging");

    *serverPtr = newServer;

    LOG_I("[createServer()] HTTTP created and saved to pointer...");

    return 0;

    fail:
    LOG_I("[createServer()] failed to create new HTTTP server");
    free(newServer);
    newServer = NULL;
    return -1;
}

int pauseServer(LibhtttpServer *serverPtr)
{
    LOG_I("[openLobby()] setting SERVER to IDLE state...");

    // check parameters
    if (serverPtr == NULL)
    {
        LOG_E("[openLobby()] serverPtr has no server allocated");
        return -1;
    }

    Server *thisServer = serverPtr;
    if (thisServer->self->state == IDLE)
    {
        LOG_E("[openLobby()] server is already in IDLE state");
        return 0; // allow to continue as though no issue (intended effect already in place)
    }

    thisServer->self->state = IDLE;

    LOG_I("[openLobby()] SERVER is set to LOBBY state");

    return 0;
}

int openLobby(LibhtttpServer *serverPtr)
{
    LOG_I("[openLobby()] setting SERVER to LOBBY state...");

    // check parameters
    if (serverPtr == NULL)
    {
        LOG_E("[openLobby()] serverPtr has no server allocated");
        return -1;
    }

    Server *thisServer = serverPtr;
    if (thisServer->self->state == LOBBY)
    {
        LOG_E("[openLobby()] server is already in LOBBY state");
        return 0; // allow to continue as though no issue (intended effect already in place)
    }

    thisServer->self->state = LOBBY;

    LOG_I("[openLobby()] SERVER is set to LOBBY state");

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

