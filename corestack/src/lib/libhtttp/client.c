#include <sys/poll.h>

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
void idleStateLoop(Endpoint *client)
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
void lobbyStateLoop(Endpoint *client)
{
    LOG_I("[lobbyStateLoop()] CLIENT entering IDLE state, awaiting state change...");
    while (client->state == LOBBY)
    {
        // Any HTTTP message in LOBBY state will be send through TCP
        Message *msg;
        if (receiveMessage(client->socks->tcp, &msg) < 0)
        {
            LOG_E("[lobbyStateLoop()] CLIENT failed to receive TCP message");
            continue;
        }
        // TODO handle message
    }
    LOG_I("[lobbyStateLoop()] state change detected, CLIENT exiting LOBBY state");
}

/*
CLIENT in GAME state where it listens for broadcasts, TCP, and UDP messages
In this state listen and handle messages accordingly (mainly about passing the messages to upper layer protocol)
*/
void gameStateLoop(Endpoint *client)
{
    LOG_I("[gameStateLoop()] CLIENT entering GAME state, awaiting state change...");

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
            LOG_D("[gameStateLoop()] %d active sockets on CLIENT", socketActivity);
            for (int i=0; i<3 && socketActivity>0; i++)
            {
                Message *msg;
                if (listenFd[i].revents & POLLIN)
                {
                    if (receiveBroadcast(listenFd[i].fd, &msg) < 0)
                    {
                        LOG_E("[gameStateLoop()] failed to receive message from %d", listenFd[i].fd);
                        continue;
                    }

                    if (listenFd[i].revents != 0)
                    {
                        socketActivity--;
                    }

                    LOG_D("[gameStateLoop()] received message:\n\tsource: %u\n\tlength: %u\n\tcontent: %s", msg->sourceId, msg->length, msg->content);
                }
            }
        }
        // handle message

    }
    LOG_I("[gameStateLoop()] state change detected, CLIENT exiting GAME state");
}

void endStateCleanup(Endpoint *client)
{
    LOG_I("[endStateCleanup()] SERVER entering END state, closing connection with all clients");

    LOG_I("[endStateCleanup()] SERVER finished cleaning up");
}

// setup for private background thread function
StateLoops stateLoops[] = {
    [IDLE] = idleStateLoop,
    [LOBBY] = lobbyStateLoop,
    [GAME] = gameStateLoop,
    [END] = endStateCleanup
};

void* threadFunc(void *client)
{
    Endpoint *thisClient = (Endpoint *) client;

    while (thisClient->state != END)
    {
        stateLoops[thisClient->state](thisClient);
    }

    if (thisClient ->state == END)
    {
        stateLoops[thisClient->state](thisClient);
    }
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
        LOG_E("[joinLobby()] failed to register with server");
        return -1;
    }

    if (prepareUDP(thisClient->socks, ipAddress) < 0)
    {
        LOG_E("[joinLobby()] could not prepare UDP port to receive broadcasts");
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
        perror("[sendAsClient()] malloc");
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

int receiveBroadcastAsClient(LibhtttpClient *clientPtr, unsigned char **returnBuffer)
{
    Endpoint *thisClient = clientPtr;

    // block until broadcast received
    Message *returnMsg;  
    if (receiveBroadcast(thisClient->socks->udpBroad, &returnMsg) < 0)
    {
        LOG_E("[receiveBroadcastAsClient()] Failed to receive broadcast");
        return -1;
    }
    
    // copy message content to return buffer
    memcpy(*returnBuffer, returnMsg->content, (size_t)returnMsg->length);

    // free function's pointers
    free(returnMsg);
    returnMsg = NULL;

    return 0;
}

// pending functions - message sending, receiving, lobby leaving