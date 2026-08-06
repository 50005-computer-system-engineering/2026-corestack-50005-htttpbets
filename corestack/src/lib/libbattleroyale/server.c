#include <sys/poll.h>

#include "lib/libbattleroyale/server.h"
#include "common.h"
#include "clientll.h"

typedef struct {
    Endpoint *self;
    ClientLinkedList *clients;
} Server;

static MessageQueue serverMessages;
static pthread_mutex_t serverMessagesLock;

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
        perror("[listenOnTCP()] bind");
        return -1;
    }

    // Listen on sockets for TCP connections 
    if (listen(serverPtr->self->socks->tcp, 100))
    {
        perror("[listenOnTCP()] listen");
        return -1;
    }
    LOG_I("[listenOnTCP()] server now listening for TCP connections on port %d", PORT_TCP);
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

int sendBroadcastTCP(Server *serverPtr, const Message completeMsg)
{
    // iterate through and send
    LOG_I("[broadcastOnTCP()] sending messages to %u clients", serverPtr->clients->count);
    ClientNode *current = serverPtr->clients->head;
    for (int i=0; i<serverPtr->clients->count; i++)
    {
        if (current != NULL)
        {
            int fd = current->client.socks->tcp;
            sendMessageTCP(fd, completeMsg);
            current = current->next;
        }
        else
        {
            return -1;
        }
    }
    LOG_I("[broadcastOnTCP()] sent message to all %u clients", serverPtr->clients->count);
    return 0;
}

// private state-based functions
typedef void (*StateLoops)(Server *serverPtr);

/* 
IDLE state where it makes thread busy wait for admin administered state change 
Does not take any HTTTP packets in this state
*/
void serverIdleState(Server *serverPtr)    // budy waits for next action
{
    LOG_I("[serverIdleState()] SERVER entering IDLE state, awaiting instructions...");
    while (serverPtr->self->state == IDLE)
    {
        // busy wait
        continue;
    }
    LOG_I("[serverIdleState()] state change detected, SERVER exiting IDLE state");
    return;
}

/*
LOBBY state where clients are allowed to send JOIN, LEAVE HTTTP packets
During this state, TCP connections from client are accepted (and server assigns player ID)
*/ 
void serverLobbyState(Server *serverPtr)   // loop where server accepts clients
{
    LOG_I("[serverLobbyState()] SERVER entering LOBBY state, accepting clients...");
    
    // prepare client list (upon entering lobby state)
    LOG_D("[serverLobbyState()] allocating space for client list on SERVER");
    if (serverPtr->clients != NULL)
    {
        // free and make a new array
        if (freeList(&serverPtr->clients) < 0)
        {
            LOG_E("[serverLobbyState()] could not free list for new lobby");
            return;
        }
        serverPtr->clients = NULL;
    }
    serverPtr->clients = malloc(sizeof(ClientLinkedList));
    if (serverPtr->clients == NULL)
    {
        perror("[serverLobbyState()] malloc");
        return;
    }
    LOG_D("[serverLobbyState()] memory allocated for client linked list");

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
        LOG_D("[serverLobbyState()] polled activity, accpeting a client");
        int clientFd = acceptOnTCP(serverPtr);
        if (clientFd < 0)
        {
            LOG_E("[serverLobbyState()] accept failed to find client, skipping loop iteration...");
            continue;
        }
        LOG_D("[serverLobbyState()] accept succses, assigning player id %u to new connection", ++prevAssignedId);
        
        // creating client endpoint of newly connected client
        Endpoint newClient = {
            .id = prevAssignedId,
            .socks = malloc(sizeof(Sockets))
        };
        newClient.socks->tcp = clientFd;

        LOG_D("[serverLobbyState()] sending client %u their ID", newClient.id);

        unsigned char *buffer = NULL;
        
        buffer = malloc(sizeof(uint32_t));
        if (buffer == NULL)
        {
            perror("[serverLobbyState()] malloc");
            goto cleanup;
        }
        uint32_t idBytes = htonl(newClient.id);
        if (sendBytes(newClient.socks->tcp, (unsigned char *)&idBytes, sizeof(uint32_t)) < 0)
        {
            LOG_E("[serverLobbyState()] failed to send source ID");
            goto cleanup;
        }
        
        // add completed new client to the clients list
        if (addToList(serverPtr->clients, &newClient))
        {
            LOG_E("[serverLobbyState()] failed to add client to new list");
            goto cleanup;
        }
        LOG_D("[serverLobbyState()] finished sending the new client their ID");

        continue;
        
        cleanup:
        free(newClient.socks);
        newClient.socks = NULL;
        LOG_E("[serverLobbyState()] failed to make new client, dropping it");

        continue;
    }

    LOG_I("[serverLobbyState()] state change detected, SERVER exiting LOBBY state");
    return;
}

/*
GAME state where it it listens for ACTION packets and sends STATE packets
At start of state state, all clients are sent an START packet to change their state into a game state
During this state, clients may still choose to LIST
To end the state, END packet is sent to change its state again
*/
void serverGameState(Server *serverPtr)    // loop where listens for unicast from clients
{
    LOG_I("[serverGameState()] SERVER entering GAME state, prerparing to listen for messages...");

    // check if there are any players
    if (serverPtr->clients->count <= 0)
    {
        LOG_E("[serverGameState()] SERVER has no clients connected, reverting to lobby state...");
        serverPtr->self->state = LOBBY;
        return;
    }

    // send TCP broadcast for clients to change state
    LOG_D("[serverGameState()] SERVER instructing clients to transition into GAME state");
    if (sendBroadcastTCP(serverPtr, (Message) {.sourceId = serverPtr->self->id, .msgType = MSG_START, .msgContent = ""}) < 0)
    {
        LOG_E("[serverGameState()] SERVER failed to broadcast new state to all clients");
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
    while (serverPtr->self->state == GAME)
    {
        // poll for any tcp activity
        int socketActivity = poll(listenFdTCP, serverPtr->clients->count, 50);
        if (socketActivity <= 0)  // timeout configured to prevent perma-blocking
        {
            continue; // skip if nothing
        }
        
        // listen to each message and handle
        Message msg;
        for (uint32_t i=0; i < serverPtr->clients->count || socketActivity > 0; i++)
        {
            if (listenFdTCP[i].revents == POLLIN)
            {
                int fd = listenFdTCP[i].fd;
                socketActivity--;
                if (receiveMessageTCP(fd, &msg) < 0)
                {
                    LOG_E("[serverGameState()] could not read message from TCP socket fd %d", fd);
                    continue;
                }
                LOG_D("[serverGameState()] received message:\n\tsource: %u\n\ttype (integerified): %d\n\tcontent: %s", msg.sourceId, msg.msgType, msg.msgContent);
            }
        }
        
        if ((poll(&listenFdUDP, 1, 50) > 0) && (listenFdUDP.revents & POLLIN))   // also poll for a UDP message
        {
            int fd = listenFdUDP.fd;
            if (receiveMessageUDP(fd, &msg) < 0)
            {
                LOG_E("[serverGameState()] could not read message from UDP unicast socket fd %d", fd);
                continue;
            }
            LOG_D("[serverGameState()] received message:\n\tsource: %u\n\ttype (integerified): %d\n\tcontent: %s", msg.sourceId, msg.msgType, msg.msgContent);
        }
        
        // enqueue a message if its an application layer message
        if (msg.msgType == MSG_APP)
        {
            LOG_D("[serverGameState()] Message received for application");
            pthread_mutex_lock(&serverMessagesLock);
            Message_enqueue(&serverMessages, msg);
            pthread_mutex_unlock(&serverMessagesLock);
        }
        // TODO other type message handling
    }
}

void serverEndState(Server *serverPtr)
{
    LOG_I("[serverEndState()] SERVER entering END state, closing connection with all clients");

    // TODO end all clients

    while (serverPtr->clients->head != NULL)
    {
        ClientNode *target = serverPtr->clients->head;
        LOG_D("[serverEndState()] removing client %u", target->client.id);
        if (closeSockets(serverPtr->self->socks) < 0)
        {
            LOG_E("[serverEndState()] could not close a socket fd");
        }
        if (removeFromList(serverPtr->clients, target->client.id) < 0)
        {
            LOG_E("[serverEndState()] could not remove client %u from list", target->client.id);
        }
    }

    LOG_I("[serverEndState()] SERVER finished cleaning up");
}

// setup for private background thread function
StateLoops serverStateLoops[] = {
    [IDLE] = serverIdleState,
    [LOBBY] = serverLobbyState,
    [GAME] = serverGameState,
    [END] = serverEndState
};

void* serverThreadFunc(void *server)
{
    Server *thisServer = (Server *) server;

    while (thisServer->self->state != END)
    {
        serverStateLoops[thisServer->self->state](thisServer);
    }

    if (thisServer->self->state == END)
    {
        serverStateLoops[thisServer->self->state](thisServer);
    }
}

// public functions
/*
Creates SERVER in memory and a background thread for listener
SERVER will start in IDLE state and await a state change triggered by user program
*/
int createServer(BRServer **serverPtr)
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

    // prepare message queue
    Message_init(&serverMessages);
    pthread_mutex_init(&serverMessagesLock, NULL);

    // spawn backrgound thread
    pthread_t threadId;
    if (pthread_create(&threadId, NULL, serverThreadFunc, (void*)newServer) != 0) 
    {
        perror("Failed to create thread");
        return 1;
    }

    LOG_I("[createServer()] HTTTP server created and saved to pointer, background thread also started");

    return 0;

    fail:
    LOG_I("[createServer()] failed to create new HTTTP server");
    free(newServer);
    newServer = NULL;
    return -1;
}

/*
Changes a SERVER into LOBBY state
Background thread will see the state change and adjust behaviour accordingly
*/
int openLobby(BRServer *serverPtr)
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

/*
Changes SERVER into GAME state (prerequisite state: LOBBY)
SERVER will update the clients and start listening for messages (handled by background thread)
*/
int startGame(BRServer *serverPtr)
{
    LOG_I("[startGame()] transitioning SERVER from GAME to LOBBY state...");

    // check parameters
    if (serverPtr == NULL)
    {
        LOG_E("[startGame()] serverPtr has no server allocated");
        return -1;
    }

    Server *thisServer = serverPtr;
    if (thisServer->self->state != LOBBY)
    {
        LOG_E("[startGame()] server is not currently in LOBBY state");
        return -1; // disallow, not in prerequisite state
    }

    thisServer->self->state = GAME;

    LOG_I("[startGame()] SERVER is set to GAME state");

    return 0;
}

/*
Changes SERVER into END state (prerequisite state: GAME)
SERVER will cleanup and close connections with all clients
*/
int endGame(BRServer *serverPtr)
{
    LOG_I("[endGame()] transitioning SERVER from GAME to LOBBY state...");

    // check parameters
    if (serverPtr == NULL)
    {
        LOG_E("[endGame()] serverPtr has no server allocated");
        return -1;
    }

    Server *thisServer = serverPtr;
    if (thisServer->self->state != GAME)
    {
        LOG_E("[endGame()] server is not currently in GAME state");
        return -1; // disallow, not in prerequisite state
    }

    thisServer->self->state = END;

    LOG_I("[startGame()] SERVER is set to END state");

    return 0;
}

/*
Function returns the number of clients and the valid clientIds
*/
int getClientInfo(BRServer *serverPtr, uint32_t *nClients, uint32_t *clientIds)
{
    LOG_I("[openLobby()] retrieving client info...");

    // check parameters
    if (serverPtr == NULL)
    {
        LOG_E("[openLobby()] serverPtr has no server allocated");
        return -1;
    }
    Server *thisServer = serverPtr;

    // iterate through client linked list for clientIds
    *nClients = thisServer->clients->count;
    uint32_t *idArray = malloc(sizeof(uint32_t)*(*nClients));
    ClientNode *client = thisServer->clients->head;
    LOG_D("[getClientInfo()] getting ids of %u clients connected to SERVER", *nClients);  
    for (uint32_t i=0; i<*nClients; i++)
    {
        LOG_D("\t[getClientInfo()] SERVER has client with id %u", client->client.id) ; 
        idArray[i] = client->client.id;
        client = client->next;
    }

    // write to return pointer
    clientIds = idArray;

    LOG_I("[openLobby()] client information written to pointers");
}

int sendAppMessageToClient(BRServer *serverPtr, uint32_t targetId, unsigned char content[512]) // use defined value instead of explicit number
{
    LOG_I("[sendAppMessageToClient()] sending broadcast to cliets...");

    // cast to private server struct
    Server *thisServer = serverPtr;

    // prepare the complete message
    Message completeMsg = {
        .sourceId = thisServer->self->id,
        .msgType = MSG_APP,
    };
    snprintf(completeMsg.msgContent, MSG_CONTENT_LENGTH, content);

    // get client
    Endpoint targetClient;
    if (getFromList(thisServer->clients, &targetClient, targetId) < 0)
    {
        LOG_E("[sendAppMessageToClient()] failed to get target client %u from the list", targetId);
        return -1;
    }

    LOG_D("[sendAppMessageToClient()] sending message to client %u:\n\tcontent: %s", targetId, completeMsg.msgContent);

    // send the message
    if (sendMessageTCP(targetClient.socks->tcp, completeMsg))
    {
        LOG_E("[sendAppMessageToClient()] failed to send message");
        return -1;
    }

    LOG_I("[sendAppMessageToClient()] message has been sent");

    return 0;
}

int sendBroadcastToClients(BRServer *serverPtr, unsigned char content[512])
{
    Server *thisServer = serverPtr;

    // prepare the complete message
    Message completeMsg = {
        .sourceId = thisServer->self->id,
        .msgType = MSG_APP,
    };
    snprintf(completeMsg.msgContent, MSG_CONTENT_LENGTH, content);

    if (sendBroadcastUDP(thisServer->self->socks->udpBroad, completeMsg))
    {
        LOG_E("[sendBroadcastToClients()] failed to send broadcast");
        return -1;
    }

    LOG_I("[sendBroadcastToClients()] message has been sent");
}

int sendReliableBroadcastToClients(BRServer *serverPtr, unsigned char content[512])
{
    Server *thisServer = serverPtr;

    // prepare the complete message
    Message completeMsg = {
        .sourceId = thisServer->self->id,
        .msgType = MSG_APP,
    };
    snprintf(completeMsg.msgContent, MSG_CONTENT_LENGTH, content);

    if (sendBroadcastTCP(serverPtr, completeMsg))
    {
        LOG_E("[sendBroadcastToClients()] failed to send broadcast");
        return -1;
    }

    LOG_I("[sendBroadcastToClients()] message has been sent");
}

/*
function allows developers to get a message from the message queue
returns 0 if no message, returns 1 if there is
*/
int getServerAppMessage(unsigned char returnMsg[512])
{
    if (pthread_mutex_trylock(&serverMessagesLock) == 0)
    {
        if (Message_empty(&serverMessages))
        {
            LOG_D("[getServerAppMessage()] no messages to process");
            return 0;
        }
        memcpy(returnMsg, Message_peek(&serverMessages)->msgContent, MSG_CONTENT_LENGTH);
        Message_dequeue(&serverMessages);
        pthread_mutex_unlock(&serverMessagesLock);
        LOG_D("[getServerAppMessage()] client message has been returned to pointer");
        return 1;
    }
    else 
    {
        LOG_D("[getServerAppMessage()] queue is busy being locked");
        return 0;
    }
}