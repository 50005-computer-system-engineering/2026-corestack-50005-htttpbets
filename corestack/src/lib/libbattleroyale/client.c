#include <sys/poll.h>

#include "lib/libbattleroyale/client.h"
#include "common.h"

static MessageQueue clientMessages;
static pthread_mutex_t clientMessagesLock;

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

int prepareUDP(Sockets *socks, char *serverIp)
{
    LOG_I("[prepareUnicastUDP()] preparing UDP unicast port");

    // set socket options
    int opt = 1;
    setsockopt(socks->udpBroad, SOL_SOCKET, SO_REUSEPORT, &opt, sizeof(opt));

    // Bind sockets to port 
    struct sockaddr_in serverAddr = {
        .sin_family = AF_INET,
        .sin_port = htons(PORT_UDP_BROAD),
        .sin_addr.s_addr = INADDR_ANY
    };
    if (bind(socks->udpBroad, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) < 0)
    {
        perror("[prepareUnicastUDP()] bind");
        return -1;
    }
    LOG_D("[prepareUnicastUDP()] UDP socket bound to port %d", PORT_UDP_UNI);

    LOG_I("[prepareUnicastUDP()] UDP unicast port ready");

    return 0;
}

// private state functions
typedef void (*StateLoops)(Endpoint *client);

/*
CLIENT in IDLE state where it is waiting for player to make action
In this state, do nothing
*/
// TODO possibly add server discovery
void clientIdleState(Endpoint *client)
{
    LOG_I("[idleStateLoop()] CLIENT entering IDLE state, awaiting state change...");
    while (client->state == IDLE)
    {
        // busy wait
        continue;
    }
    LOG_I("[idleStateLoop()] state change detected, CLIENT exiting IDLE state");
    return;
}

/*
CLIENT in LOBBY state where is awaits the server message to START
In this state, listen and make state change only upon START
*/
void clientLobbyState(Endpoint *client)
{
    LOG_I("[clientLobbyState()] CLIENT entering LOBBY state, awaiting state change...");
    while (client->state == LOBBY)
    {
        // Any HTTTP message in LOBBY state will be send through TCP
        Message msg;
        if (receiveMessageTCP(client->socks->tcp, &msg) < 0)
        {
            LOG_E("[clientLobbyState()] CLIENT failed to receive TCP message");
            continue;
        }
        if (msg.msgType == MSG_START)
        {
            LOG_D("[clientLobbyState()] CLIENT received message to START");
            client->state = GAME;
        }
    }
    LOG_I("[clientLobbyState()] state change detected, CLIENT exiting LOBBY state");
}

/*
CLIENT in GAME state where it listens for broadcasts, TCP, and UDP messages
In this state listen and handle messages accordingly (mainly about passing the messages to upper layer protocol)
*/
void clientGameState(Endpoint *client)
{
    LOG_I("[clientGameState()] CLIENT entering GAME state, awaiting state change...");

    // Any HTTTP message in LOBBY state will be sent through TCP, UDP unicast, or UDP broadcast
    // create pollfd struct
    struct pollfd *listenFd = malloc(sizeof(struct pollfd) * 3);
    listenFd[0] = (struct pollfd) {
        .fd = client->socks->tcp,
        .events = POLLIN,
        .revents = 0
    };
    listenFd[1] = (struct pollfd) {
        .fd = client->socks->udpUni,
        .events = POLLIN,
        .revents = 0
    };
    listenFd[2] = (struct pollfd) {
        .fd = client->socks->udpBroad,
        .events = POLLIN,
        .revents = 0
    };
    
    while (client->state == GAME)
    {
        int socketActivity = poll(listenFd, 3, 50);
        if (socketActivity > 0)
        {
            LOG_D("[clientGameState()] %d active sockets on CLIENT", socketActivity);
            for (int i=0; i<3 && socketActivity>0; i++)
            {
                if (listenFd[i].revents & POLLIN)
                {
                    Message msg;

                    // receiving TCP message
                    if (i == 0)
                    {
                        if (receiveMessageTCP(listenFd[i].fd, &msg) < 0)
                        {
                            LOG_E("[clientGameState()] failed to receive TCP message from %d", listenFd[i].fd);
                            continue;
                        }
                    }
                    // receiving UDP messages
                    else 
                    {
                        if (receiveMessageUDP(listenFd[i].fd, &msg) < 0)
                        {
                            LOG_E("[clientGameState()] failed to receive TCP message from %d", listenFd[i].fd);
                            continue;
                        }
                    }

                    if (listenFd[i].revents != 0)
                    {
                        socketActivity--;
                    }

                    LOG_D("[clientGameState()] received message:\n\tsource: %u\n\ttype (integerified): %d\n\tcontent: %s", msg.sourceId, msg.msgType, msg.msgContent);
                    if (msg.msgType == MSG_APP)
                    {
                        LOG_D("[clientGameState()] Message received for application");
                        pthread_mutex_lock(&clientMessagesLock);
                        Message_enqueue(&clientMessages, msg);
                        pthread_mutex_unlock(&clientMessagesLock);
                    }
                    if (msg.msgType == MSG_END)
                    {
                        LOG_D("[clientGameState()] Message received to change to END state");
                        client->state = END;
                    }
                }
            }
        }
    }
    LOG_I("[clientGameState()] state change detected, CLIENT exiting GAME state");
}

void clientEndState(Endpoint *client)
{
    LOG_I("[endStateCleanup()] CLIENT entering END state, closing connection with all clients");
    if (freeEndpoint(&client) < 0)
    {
        LOG_E("[clientEndState()] could not free client in memory");
    }
    LOG_I("[endStateCleanup()] CLIENT finished cleaning up");
}

// setup for private background thread function
StateLoops clientStateLoops[] = {
    [IDLE] = clientIdleState,
    [LOBBY] = clientLobbyState,
    [GAME] = clientGameState,
    [END] = clientEndState
};

void* clientThreadFunc(void *client)
{
    Endpoint *thisClient = (Endpoint *) client;

    while (thisClient->state != END)
    {
        clientStateLoops[thisClient->state](thisClient);
    }

    if (thisClient ->state == END)
    {
        clientStateLoops[thisClient->state](thisClient);
    }
}

// public functions
// allows developers to create a libhtttp client in application
int brclient_init(BRClient **clientPtr)
{
    Endpoint *newClient = NULL;

    // create endpoint
    if (createEndpoint(&newClient) < 0)
    {
        LOG_E("[brclient_init()] could not create endpoint struct for client");
        return -1;
    }

    // open sockets
    if (createSockets(&newClient->socks) < 0)
    {
        LOG_E("[brclient_init()] socket creation failed");
        return -1;
    }

    // default values
    newClient->id = 0;
    newClient->state = 0;

    *clientPtr = newClient;
    
    // prepare queue
    Message_init(&clientMessages);
    pthread_mutex_init(&clientMessagesLock, NULL);

    // spawn backrgound thread
    pthread_t threadId;
    if (pthread_create(&threadId, NULL, clientThreadFunc, (void*)newClient) != 0) 
    {
        perror("Failed to create thread");
        return 1;
    }

    LOG_I("[brclient_init()] new client created, and spawned background thread");

    return 0;
}

// connects to a libhtttp server
int brclient_join(BRClient *clientPtr, char *ipAddress)
{
    Endpoint *thisClient = clientPtr;

    LOG_I("[brclient_join()] attempting connection to lobby located at IP %s", ipAddress);

    // TALKING TO SERVER
    // connect on TCP first
    if (connectOnTCP(thisClient->socks, ipAddress) < 0)
    {
        LOG_E("[startClientHandshake()] failed to connect to server at IP");
        return -1;
    }

    // receive user id and save to Endpoint
    unsigned char *buffer = NULL;
    buffer = malloc(sizeof(uint32_t));
    if (buffer == NULL)
    {
        perror("[startClientHandshake()] malloc");
        return -1;
    }
    if (readBytes(thisClient->socks->tcp, &buffer, sizeof(uint32_t)) < 0)
    {
        LOG_E("[startClientHandshake()] failed to read sourceId");
        return -1;
    } 
    uint32_t sourceBytes;
    memcpy(&sourceBytes, buffer, sizeof(sourceBytes));
    thisClient->id = ntohl(sourceBytes);
    free(buffer);
    buffer = NULL;

    // INDEPENDENT OF SERVER
    // prepare UDP ports for future use upon connection
    if (prepareUDP(thisClient->socks, ipAddress) < 0)
    {
        LOG_E("[brclient_join()] could not prepare UDP port to receive broadcasts");
        return -1;
    }

    // joining lobby successful, enter lobby state
    thisClient->state = LOBBY;

    LOG_I("[brclient_join()] lobby joining complete");
    return 0;
}

// message functions
int brclient_send_msg(BRClient *clientPtr, unsigned char content[512])
{
    Endpoint *thisClient = clientPtr;

    // build message
    Message msg = {
        .sourceId = thisClient->id,
        .msgType = MSG_APP,
    };
    snprintf(msg.msgContent, MSG_CONTENT_LENGTH, content);

    // send via socket
    if (sendMessageTCP(thisClient->socks->tcp, msg) < 0)
    {
        LOG_E("[brclient_send_msg()] sending has failed");
        goto fail;
    }

    LOG_I("[brclient_send_msg()] message has been sent");

    return 0;

    fail:
    return -1;
}

/*
function allows developers to get a message from the message queue
returns 0 if no message, returns 1 if there is
*/
int brclient_get_app_msg(unsigned char returnMsg[512])
{
    if (pthread_mutex_trylock(&clientMessagesLock) == 0)
    {
        if (Message_empty(&clientMessages))
        {
            LOG_D("[brclient_get_app_msg()] no messages to process");
            return 0;
        }
        snprintf(returnMsg, MSG_CONTENT_LENGTH, Message_peek(&clientMessages)->msgContent);
        Message_dequeue(&clientMessages);
        pthread_mutex_unlock(&clientMessagesLock);
        LOG_D("[brclient_get_app_msg()] client message has been returned unsigned char array");
        return 1;
    }
    else 
    {
        LOG_D("[brclient_get_app_msg()] queue is busy being locked");
        return 0;
    }
}
