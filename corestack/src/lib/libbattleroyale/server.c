#include <sys/poll.h>

#include "lib/libbattleroyale/server.h"
#include "common.h"
#include "clientll.h"

typedef struct {
    Endpoint* self;
    ClientLinkedList* clients;
} Server;

static MessageQueue server_messages;
static pthread_mutex_t server_messages_lock;
static int pc_flags[2] = {0, 0};

// private functions
int free_server(Server** server_ptr)
{
    Server* freeing_server = *server_ptr;
    if (free_list(&freeing_server->clients)) {
        LOG_E("[freeServer()] could not free client list");
        return -1;
    }
    if (free_endpoint(&freeing_server->self) < 0) {
        LOG_E("[freeServer()] could not free self");
        return -1;
    }
    free(freeing_server);
    freeing_server = NULL;
    LOG_I("[freeServer()] freed server");
    return 0;
}

int listen_on_tcp(Server* server_ptr)
{
    // Check if sockets have valid fd
    if (check_sockets(*(server_ptr->self->socks)) < 0) {
        LOG_E("[listenOnTCP()] sockets in Sockets invalid");
        return -1;
    }

    // Bind sockets to port
    struct sockaddr_in server_addr = {
        .sin_family = AF_INET,
        .sin_port = htons(PORT_TCP),
        .sin_addr.s_addr = INADDR_ANY};
    if (bind(server_ptr->self->socks->tcp, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("[listenOnTCP()] bind");
        return -1;
    }

    // Listen on sockets for TCP connections
    if (listen(server_ptr->self->socks->tcp, 100)) {
        perror("[listenOnTCP()] listen");
        return -1;
    }
    LOG_I("[listenOnTCP()] server now listening for TCP connections on port %d", PORT_TCP);
    return 0;
}

int accept_on_tcp(Server* server_ptr)
{
    LOG_I("[acceptOnTCP()] server now accepting a TCP client...");
    int new_client_fd = accept(server_ptr->self->socks->tcp, NULL, NULL);
    if (new_client_fd < 0) {
        perror("acceptOnTCP accept");
        return -1;
    }
    LOG_I("[acceptOnTCP()] server accepted new TCP client");
    return new_client_fd;
}

int prepare_unicast_udp(Server* server_ptr)
{
    LOG_I("[prepareUnicastUDP()] preparing UDP unicast port");

    // Bind sockets to port
    struct sockaddr_in server_addr = {
        .sin_family = AF_INET,
        .sin_port = htons(PORT_UDP_UNI),
        .sin_addr.s_addr = INADDR_ANY // any interface address
    };
    if (bind(server_ptr->self->socks->udp_uni, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("[prepareUnicastUDP()] bind");
        return -1;
    }
    LOG_D("[prepareUnicastUDP()] UDP socket bound to port %d", PORT_UDP_UNI);

    LOG_I("[prepareUnicastUDP()] UDP unicast port ready to receive");

    return 0;
}

int prepare_broadcast_udp(Server* server_ptr)
{
    LOG_I("[prepareBroadcastUDP()] preparing UDP broadcast port");

    // setting socket options
    int broadcast_enabled = 1;
    if (setsockopt(server_ptr->self->socks->udp_broad, SOL_SOCKET, SO_BROADCAST, &broadcast_enabled, sizeof(broadcast_enabled)) < 0) {
        perror("[prepareBroadcastUDP()] setsockopt");
        return -1;
    }
    LOG_D("[prepareBroadcastUDP()] socket options set");

    LOG_I("[prepareBroadcastUDP()] UDP broadcast port ready for transmission");

    return 0;
}

int send_broadcast_tcp(Server* server_ptr, const Message COMPLETE_MSG)
{
    // iterate through and send
    LOG_I("[broadcastOnTCP()] sending messages to %u clients", server_ptr->clients->count);
    ClientNode* current = server_ptr->clients->head;
    for (int i = 0; i < server_ptr->clients->count; i++) {
        if (current != NULL) {
            int fd = current->client.socks->tcp;
            send_message_tcp(fd, COMPLETE_MSG);
            current = current->next;
        } else {
            return -1;
        }
    }
    LOG_I("[broadcastOnTCP()] sent message to all %u clients", server_ptr->clients->count);
    return 0;
}

// private state-based functions
typedef void (*StateLoops)(Server* server_ptr);

/*
IDLE state where it makes thread busy wait for admin administered state change
Does not take any HTTTP packets in this state
*/
void server_idle_state(Server* server_ptr) // budy waits for next action
{
    LOG_I("[serverIdleState()] SERVER entering IDLE state, awaiting instructions...");
    while (server_ptr->self->state == IDLE) {
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
void server_lobby_state(Server* server_ptr) // loop where server accepts clients
{
    LOG_I("[serverLobbyState()] SERVER entering LOBBY state, accepting clients...");

    // tracking the id assigned
    int prev_assigned_id = 0;

    // preparing TCP socket for polling
    struct pollfd accept_port;
    accept_port.fd = server_ptr->self->socks->tcp; // socket to start connection with the poll
    accept_port.events = POLLIN;
    accept_port.revents = 0;

    // while in this state, keep accepting clients
    while (server_ptr->self->state == LOBBY) {
        // poll for a client
        if (poll(&accept_port, 1, 50) <= 0) // timeout configured to prevent perma-blocking
        {
            continue; // skip if nothing
        }

        // accept a client if the socket has any POLLIN activity
        LOG_D("[serverLobbyState()] polled activity, accpeting a client");
        int client_fd = accept_on_tcp(server_ptr);
        if (client_fd < 0) {
            LOG_E("[serverLobbyState()] accept failed to find client, skipping loop iteration...");
            continue;
        }
        LOG_D("[serverLobbyState()] accept succses, assigning player id %u to new connection", ++prev_assigned_id);

        // creating client endpoint of newly connected client
        Endpoint new_client = {
            .id = prev_assigned_id,
            .socks = malloc(sizeof(Sockets))};
        new_client.socks->tcp = client_fd;

        LOG_D("[serverLobbyState()] sending client %u their ID", new_client.id);

        unsigned char* buffer = NULL;

        buffer = malloc(sizeof(uint32_t));
        if (buffer == NULL) {
            perror("[serverLobbyState()] malloc");
            goto cleanup;
        }
        uint32_t id_bytes = htonl(new_client.id);
        if (send_bytes(new_client.socks->tcp, (unsigned char*)&id_bytes, sizeof(uint32_t)) < 0) {
            LOG_E("[serverLobbyState()] failed to send source ID");
            goto cleanup;
        }

        // add completed new client to the clients list
        if (add_to_list(server_ptr->clients, &new_client)) {
            LOG_E("[serverLobbyState()] failed to add client to new list");
            goto cleanup;
        }
        LOG_D("[serverLobbyState()] finished sending the new client their ID");

        continue;

    cleanup:
        free(new_client.socks);
        new_client.socks = NULL;
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
void server_game_state(Server* server_ptr) // loop where listens for unicast from clients
{
    LOG_I("[serverGameState()] SERVER entering GAME state, prerparing to listen for messages...");

    // check if there are any players
    if (server_ptr->clients->count <= 0) {
        LOG_E("[serverGameState()] SERVER has no clients connected, reverting to lobby state...");
        server_ptr->self->state = LOBBY;
        return;
    }

    // send TCP broadcast for clients to change state
    LOG_D("[serverGameState()] SERVER instructing clients to transition into GAME state");
    if (send_broadcast_tcp(server_ptr, (Message){.source_id = server_ptr->self->id, .msg_type = MSG_START, .msg_content = ""}) < 0) {
        LOG_E("[serverGameState()] SERVER failed to broadcast new state to all clients");
        return;
    }

    // setup pollfd array for clients being listened to
    struct pollfd* listen_fd_tcp = malloc(sizeof(struct pollfd) * server_ptr->clients->count);
    struct pollfd* current_fd = listen_fd_tcp;
    // loop through server's clients
    for (ClientNode* current_client = server_ptr->clients->head; current_client != NULL; current_client = current_client->next) {
        current_fd->fd = current_client->client.socks->tcp;
        current_fd->events = POLLIN;
        current_fd->revents = 0;
        current_fd++;
    }
    struct pollfd listen_fd_udp;
    listen_fd_udp.fd = server_ptr->self->socks->udp_uni;
    listen_fd_udp.events = POLLIN;
    listen_fd_udp.revents = 0;

    // while the GAME state is active, monitor for messages to each port
    while (server_ptr->self->state == GAME) {
        // poll for any tcp activity
        int socket_activity = poll(listen_fd_tcp, server_ptr->clients->count, 50);
        if (socket_activity <= 0) // timeout configured to prevent perma-blocking
        {
            continue; // skip if nothing
        }

        // listen to each message and handle
        Message msg;
        for (uint32_t i = 0; i < server_ptr->clients->count && socket_activity > 0; i++) // Changed condition from || to && to prevent array out-of-bounds access
        {
            if (listen_fd_tcp[i].revents == POLLIN) {
                int fd = listen_fd_tcp[i].fd;
                socket_activity--;
                if (receive_message_tcp(fd, &msg) < 0) {
                    LOG_E("[serverGameState()] could not read message from TCP socket fd %d", fd);
                    listen_fd_tcp[i].fd = -1; // Set disconnected socket to -1 to prevent triggering infinite EOF loop
                    continue;
                }
                LOG_D("[serverGameState()] received message:\n\tsource: %u\n\ttype (integerified): %d\n\tcontent: %s", msg.source_id, msg.msg_type, msg.msg_content);

                // enqueue a message if its an application layer message
                if (msg.msg_type == MSG_APP) // Shifted this inside the successful TCP read block to prevent queuing stale / uninitialized messages
                {
                    LOG_D("[serverGameState()] Message received for application");
                    if (pc_flags[0]) {
                        pc_flags[1] = 1;
                        LOG_D("[serverGameState()] listener yielding CPU control for user polling queue");
                        sched_yield();
                    }
                    pthread_mutex_lock(&server_messages_lock);
                    Message_enqueue(&server_messages, msg);
                    pthread_mutex_unlock(&server_messages_lock);
                    pc_flags[1] = 0;
                }
            }
        }

        if ((poll(&listen_fd_udp, 1, 50) > 0) && (listen_fd_udp.revents & POLLIN)) // also poll for a UDP message
        {
            int fd = listen_fd_udp.fd;
            if (receive_message_udp(fd, &msg) < 0) {
                LOG_E("[serverGameState()] could not read message from UDP unicast socket fd %d", fd);
                continue;
            }
            LOG_D("[serverGameState()] received message:\n\tsource: %u\n\ttype (integerified): %d\n\tcontent: %s", msg.source_id, msg.msg_type, msg.msg_content);

            // enqueue a message if its an application layer message
            if (msg.msg_type == MSG_APP) // Shifted this inside the successful UDP read block to prevent queuing stale / uninitialized messages
            {
                LOG_D("[serverGameState()] Message received for application");
                if (pc_flags[0]) {
                    pc_flags[1] = 1;
                    LOG_D("[serverGameState()] listener yielding CPU control for user polling queue");
                    sched_yield();
                }
                pthread_mutex_lock(&server_messages_lock);
                Message_enqueue(&server_messages, msg);
                pthread_mutex_unlock(&server_messages_lock);
                pc_flags[1] = 0;
            }
        }
        // TODO other type message handling
    }
    free(listen_fd_tcp); // free to prevent memory leaks
}

void server_end_state(Server* server_ptr)
{
    LOG_I("[serverEndState()] SERVER entering END state, closing connection with all clients");

    if (send_broadcast_tcp(server_ptr, (Message){.source_id = server_ptr->self->id, .msg_type = MSG_END, .msg_content = ""}) < 0) {
        LOG_E("[serverEndState()] could not send instruction to proceed to end state");
    }

    LOG_D("[serverEndState()] SERVER sent command to END game to all CLIENT");

    if (free_server(&server_ptr) < 0) {
        LOG_E("[serverEndState()] could not free server in memory");
    }

    LOG_I("[serverEndState()] SERVER finished cleaning up");
}

// setup for private background thread function
StateLoops server_state_loops[] = {
    [IDLE] = server_idle_state,
    [LOBBY] = server_lobby_state,
    [GAME] = server_game_state,
    [END] = server_end_state};

void* server_thread_func(void* server)
{
    Server* this_server = (Server*)server;

    while (this_server->self->state != END) {
        server_state_loops[this_server->self->state](this_server);
    }

    if (this_server->self->state == END) {
        server_state_loops[this_server->self->state](this_server);
        pthread_exit(NULL);
    }
}

// public functions
/*
Creates SERVER in memory and a background thread for listener
SERVER will start in IDLE state and await a state change triggered by user program
*/
int brserver_init(BRServer** server_ptr)
{
    // allocate memory for it
    Server* new_server = calloc(1, sizeof(Server));
    if (new_server == NULL) {
        perror("server malloc");
        return -1;
    }

    LOG_I("[brserver_init()] creating HTTTP server...");

    // create the endpoint
    if (create_endpoint(&new_server->self) < 0) {
        LOG_E("[brserver_init()] endpoint creation failed");
        goto fail;
    }
    if (initalise_list(&new_server->clients) < 0) {
        LOG_E("[brserver_init()] client list creation failed");
        goto fail;
    }
    LOG_I("[brserver_init()] new server created with sockets created");

    // prepare ports for usage
    if (listen_on_tcp(new_server) < 0) {
        LOG_E("[brserver_open()] failed to bind port for listening");
        goto fail;
    }
    LOG_D("[brserver_init()] bound port for listening");
    if (prepare_broadcast_udp(new_server) < 0 || prepare_unicast_udp(new_server) < 0) {
        LOG_E("[brserver_open()] failed to prepare UDP port for listening");
        goto fail;
    }
    LOG_D("[brserver_init()] UDP ports bound and ready for messaging");

    *server_ptr = new_server;

    // prepare message queue
    Message_init(&server_messages);
    pthread_mutex_init(&server_messages_lock, NULL);

    // spawn backrgound thread
    pthread_t thread_id;
    if (pthread_create(&thread_id, NULL, server_thread_func, (void*)new_server) != 0) {
        perror("Failed to create thread");
        return 1;
    }

    LOG_I("[brserver_init()] HTTTP server created and saved to pointer, background thread also started");

    return 0;

fail:
    LOG_I("[brserver_init()] failed to create new HTTTP server");
    free(new_server);
    new_server = NULL;
    return -1;
}

/*
Changes a SERVER into LOBBY state
Background thread will see the state change and adjust behaviour accordingly
*/
int brserver_open(BRServer* server_ptr)
{
    LOG_I("[brserver_open()] setting SERVER to LOBBY state...");

    // check parameters
    if (server_ptr == NULL) {
        LOG_E("[brserver_open()] serverPtr has no server allocated");
        return -1;
    }

    Server* this_server = server_ptr;
    if (this_server->self->state == LOBBY) {
        LOG_E("[brserver_open()] server is already in LOBBY state");
        return 0; // allow to continue as though no issue (intended effect already in place)
    }

    this_server->self->state = LOBBY;

    LOG_I("[brserver_open()] SERVER is set to LOBBY state");

    return 0;
}

/*
Changes SERVER into GAME state (prerequisite state: LOBBY)
SERVER will update the clients and start listening for messages (handled by background thread)
*/
int brserver_start(BRServer* server_ptr)
{
    LOG_I("[brserver_start()] transitioning SERVER from GAME to LOBBY state...");

    // check parameters
    if (server_ptr == NULL) {
        LOG_E("[brserver_start()] serverPtr has no server allocated");
        return -1;
    }

    Server* this_server = server_ptr;
    if (this_server->self->state != LOBBY) {
        LOG_E("[brserver_start()] server is not currently in LOBBY state");
        return -1; // disallow, not in prerequisite state
    }

    this_server->self->state = GAME;

    LOG_I("[brserver_start()] SERVER is set to GAME state");

    return 0;
}

/*
Changes SERVER into END state (prerequisite state: GAME)
SERVER will cleanup and close connections with all clients
*/
int brserver_end(BRServer* server_ptr)
{
    LOG_I("[brserver_end()] transitioning SERVER from GAME to LOBBY state...");

    // check parameters
    if (server_ptr == NULL) {
        LOG_E("[brserver_end()] serverPtr has no server allocated");
        return -1;
    }

    Server* this_server = server_ptr;
    if (this_server->self->state != GAME) {
        LOG_E("[brserver_end()] server is not currently in GAME state");
        return -1; // disallow, not in prerequisite state
    }

    this_server->self->state = END;

    LOG_I("[brserver_start()] SERVER is set to END state");

    return 0;
}

/*
Function returns the number of clients and the valid clientIds
*/
int brserver_client_info(BRServer* server_ptr, uint32_t* n_clients, uint32_t* client_ids)
{
    LOG_I("[brserver_client_info()] retrieving client info...");

    // check parameters
    if (server_ptr == NULL) {
        LOG_E("[brserver_client_info()] serverPtr has no server allocated");
        return -1;
    }
    Server* this_server = server_ptr;

    // iterate through client linked list for clientIds
    *n_clients = this_server->clients->count;
    // uint32_t *idArray = malloc(sizeof(uint32_t)*(*nClients));
    ClientNode* client = this_server->clients->head;
    LOG_D("[brserver_client_info()] getting ids of %u clients connected to SERVER", *n_clients);
    for (uint32_t i = 0; i < *n_clients; i++) {
        if (client != NULL) // Write directly into the caller's pre-allocated clientIds array
        {
            LOG_D("\t[brserver_client_info()] SERVER has client with id %u", client->client.id);
            // idArray[i] = client->client.id;
            client_ids[i] = client->client.id;
            client = client->next;
        }
    }

    // write to return pointer
    // clientIds = idArray; -> leaked memory and failed to caller's reference because passed by value

    LOG_I("[brserver_client_info()] client information written to pointers");
    return 0;
}

int brserver_send_to_target(BRServer* server_ptr, uint32_t target_id, unsigned char content[1024]) // use defined value instead of explicit number
{
    LOG_I("[brserver_send_to_target()] sending broadcast to cliets...");

    // cast to private server struct
    Server* this_server = server_ptr;

    // prepare the complete message
    Message complete_msg = {
        .source_id = this_server->self->id,
        .msg_type = MSG_APP,
    };
    // snprintf(completeMsg.msgContent, MSG_CONTENT_LENGTH, content);
    memcpy(complete_msg.msg_content, content, MSG_CONTENT_LENGTH); // Prevent null-byte truncation of binary data

    // get client
    Endpoint target_client;
    if (get_from_list(this_server->clients, &target_client, target_id) < 0) {
        LOG_E("[brserver_send_to_target()] failed to get target client %u from the list", target_id);
        return -1;
    }

    LOG_D("[brserver_send_to_target()] sending message to client %u:\n\tcontent: %s", target_id, complete_msg.msg_content);

    // send the message
    if (send_message_tcp(target_client.socks->tcp, complete_msg)) {
        LOG_E("[brserver_send_to_target()] failed to send message");
        return -1;
    }

    LOG_I("[brserver_send_to_target()] message has been sent");

    return 0;
}

int brserver_send_broadcast(BRServer* server_ptr, unsigned char content[1024])
{
    Server* this_server = server_ptr;

    // prepare the complete message
    Message complete_msg = {
        .source_id = this_server->self->id,
        .msg_type = MSG_APP,
    };
    // snprintf(completeMsg.msgContent, MSG_CONTENT_LENGTH, content);
    memcpy(complete_msg.msg_content, content, MSG_CONTENT_LENGTH); // Prevent null-byte truncation of binary data

    if (send_broadcast_udp(this_server->self->socks->udp_broad, complete_msg)) {
        LOG_E("[brserver_send_broadcast()] failed to send broadcast");
        return -1;
    }

    LOG_I("[brserver_send_broadcast()] message has been sent");
}

int brserver_send_to_all(BRServer* server_ptr, unsigned char content[1024])
{
    Server* this_server = server_ptr;

    // prepare the complete message
    Message complete_msg = {
        .source_id = this_server->self->id,
        .msg_type = MSG_APP,
    };
    // snprintf(completeMsg.msgContent, MSG_CONTENT_LENGTH, content);
    memcpy(complete_msg.msg_content, content, MSG_CONTENT_LENGTH); // Prevent null-byte truncation of binary data

    if (send_broadcast_tcp(server_ptr, complete_msg)) {
        LOG_E("[brserver_send_to_all()] failed to send broadcast");
        return -1;
    }

    LOG_I("[brserver_send_to_all()] message has been sent");
}

/*
function allows developers to get a message from the message queue
returns 0 if no message, returns 1 if there is
*/
int brserver_get_app_msg(unsigned char return_msg[1024])
{
    if (!Message_empty(&server_messages)) {
        if (pc_flags[1]) {
            pc_flags[0] = 1;
            LOG_D("[brserver_get_app_msg()] yielding CPU control for listener thread to write to queue");
            sched_yield();
        }
        pthread_mutex_lock(&server_messages_lock);
        memcpy(return_msg, Message_peek(&server_messages)->msg_content, MSG_CONTENT_LENGTH);
        Message_dequeue(&server_messages);
        pthread_mutex_unlock(&server_messages_lock);
        pc_flags[0] = 0;
        LOG_D("[brserver_get_app_msg()] client message has been returned to unsigned char array");
        return 1;
    } else {
        LOG_D("[brserver_get_app_msg()] queue is empty");
        return 0;
    }
}
