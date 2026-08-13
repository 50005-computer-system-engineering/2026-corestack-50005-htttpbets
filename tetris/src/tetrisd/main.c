#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdarg.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <ifaddrs.h>
#include <net/if.h>
#include "lib/libbattleroyale/server.h"
#include "lib/libtetrisprotocol/protocol.h"
#include "lib/libtetrisbrain/garbage.h"
#include "lib/libtetrislog/logclient.h"
#include "game.h"

// MAX_LOBBY_SIZE defined in protocol.h
#define MIN_LOBBY_SIZE 1

// Instantiate logger
static LogClient logClient;

// Non-blocking check for a pressed ENTER key -> signifiy transition to GAME state
static bool enterPressed(void)
{
    fd_set fds;
    struct timeval tv = {0, 0};
    FD_ZERO(&fds);
    FD_SET(STDIN_FILENO, &fds);
    if (select(STDIN_FILENO + 1, &fds, NULL, NULL, &tv) > 0)
    {
        int c = getchar();
        return c == '\n';
    }
    return false;
}

static void logMessage(LogLevel level, const char *fmt, ...)
{
    char message[LOG_MSG_LENGTH];
    va_list args;
    va_start(args, fmt);
    vsnprintf(message, sizeof(message), fmt, args);
    va_end(args);

    printf("%s\n", message);                   // Unchanged terminal output
    logClientPush(&logClient, level, message); // Forwarded, non-blocking, may be dropped
}

int main(void)
{
    // Line-buffered so logs survive being piped to a file
    setvbuf(stdout, NULL, _IOLBF, 0);

    // Setup Logger
    if (!logClientInit(&logClient, LOG_DEFAULT_IPC_PATH, "tetrisd"))
    {
        printf("[tetrisd] Warning: could not set up logging client, continuing without it.\n");
    }

    // Initialize server connection
    BRServer *server = NULL;

    // Helper: select first non-loopback IPv4 address
    char host_ip[INET_ADDRSTRLEN] = {0}; // Init to store potential IPV4 addr -> for display only!!
    struct ifaddrs *ifaddr, *ifa;
    if (getifaddrs(&ifaddr) == 0) // Queries and returns a linked list of all the network interfaces currently active
    {
        for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next) // Start at the first network interface, keep looping until reach end of chain
        {
            if (ifa->ifa_addr == NULL) // Active network interface but no IP addr
            {
                continue;
            }
            if (ifa->ifa_addr->sa_family != AF_INET) // Not IPV4 format
            {
                continue;
            }
            if (ifa->ifa_flags & IFF_LOOPBACK) // Skip loopback interface
            {
                continue;
            }

            struct sockaddr_in *sa = (struct sockaddr_in *)ifa->ifa_addr;            // Cast to sockaddr_in (IPV4) to process
            if (inet_ntop(AF_INET, &sa->sin_addr, host_ip, INET_ADDRSTRLEN) != NULL) // Convert binary address into readable texts
            {
                break; // First candidate found
            }
        }
        freeifaddrs(ifaddr); // To prevent memory leak
    }

    int init_res = brserver_init(&server); // Always bind to every interface, so both LAN And clients are reachable
    // Tell the host which IPV4 addr to hand out to other players
    if (host_ip[0] != '\0') // Valid IP addr found
    {
        logMessage(LOG_LEVEL_INFO, "[tetrisd] Players on this network should enter: %s", host_ip);
    }
    else
    {
        // No IPV4 addr found, only local play is possible
        logMessage(LOG_LEVEL_WARN, "[tetrisd] No non-loopback IPv4 found, local play only (127.0.0.1).");
    }
    printf("[tetrisd] Players on this machine can use the default 127.0.0.1\n");

    if (init_res < 0) // Failed
    {
        logMessage(LOG_LEVEL_ERROR, "[tetrisd] Could not create server!");
        return -1;
    }
    logMessage(LOG_LEVEL_INFO, "[tetrisd] Server created successfully! Opening lobby...");

    // LOBBY state and assign player IDs
    if (brserver_open(server) < 0) // Blocking server call, waits until lobby is filled
    {
        logMessage(LOG_LEVEL_ERROR, "[tetrisd] Could not create lobby!");
        return -1;
    }

    // Pre-allocation for clients
    uint32_t lobbySize = 0;
    uint32_t clientIds[MAX_LOBBY_SIZE] = {0};

    // Wait dynamically for lobby to fill
    printf("[tetrisd] Lobby open! Need at least %d player(s).\n", MIN_LOBBY_SIZE);
    printf("[tetrisd] Press ENTER at any time once ready to start the game.\n");
    uint32_t lastReported = UINT32_MAX;
    while (1)
    {
        brserver_client_info(server, &lobbySize, clientIds);
        // Only announce on actual change
        if (lobbySize != lastReported)
        {
            logMessage(LOG_LEVEL_INFO, "[tetrisd] %u player(s) connected...", lobbySize);
            lastReported = lobbySize;
        }

        if (lobbySize >= MIN_LOBBY_SIZE && enterPressed())
        {
            logMessage(LOG_LEVEL_INFO, "[tetrisd] Starting game with %u players!", lobbySize);
            break;
        }
        if (lobbySize >= MAX_LOBBY_SIZE)
        {
            logMessage(LOG_LEVEL_INFO, "[tetrisd] Lobby full (%u), starting automatically.", lobbySize);
            break;
        }
        usleep(100000); // Poll every 100ms
    }

    // Once lobby is full
    {
        char roster_line[LOG_MSG_LENGTH];
        int offset = snprintf(roster_line, sizeof(roster_line), "[tetrisd] Lobby filled! %u player(s) connected:", lobbySize);
        for (uint32_t i = 0; i < lobbySize && offset < (int)sizeof(roster_line); i++)
        {
            offset += snprintf(roster_line + offset, sizeof(roster_line) - offset, " P%u", clientIds[i]);
        }
        logMessage(LOG_LEVEL_INFO, "%s", roster_line);
    }

    // Signal listening for events -> switch to GAME state to start
    if (brserver_start(server) < 0)
    {
        logMessage(LOG_LEVEL_ERROR, "[tetrisd] Failed to start game!");
        return -1;
    }
    logMessage(LOG_LEVEL_INFO, "[tetrisd] Server is now in GAME state. Awaiting events...");

    // Tell every client the players in the lobby
    {
        // Populate roster via RosterPayload
        RosterPayload roster = {0};
        roster.count = lobbySize; // Total number of players
        for (uint32_t i = 0; i < lobbySize && i < MAX_LOBBY_SIZE; i++)
        {
            roster.ids[i] = clientIds[i]; // Add dynamically to array
        }
        unsigned char roster_buffer[512] = {0}; // Empty buffer
        packRoster(roster_buffer, &roster);     // Handles tag and byte order

        // Send setup data (explicitly uses TCP here)
        if (brserver_send_to_all(server, roster_buffer) < 0)
        {
            logMessage(LOG_LEVEL_WARN, "[tetrisd] Warning: failed to broadcast player roster.");
        }
        else
        {
            logMessage(LOG_LEVEL_INFO, "[tetrisd] Roster broadcast to all %u players.", lobbySize);
        }
    }

    // Build the authoritative match, one real GameState per connected player
    // From here the server owns the boards, clients only render copies of them
    GameSession session;
    initSession(&session, clientIds, lobbySize);
    logMessage(LOG_LEVEL_INFO, "[tetrisd] Authoritative session started for %d player(s).", session.count);

    // Array buffer for message queue
    unsigned char buffer[512] = {0};

    // A battle royale ends when only one player is left standing. A solo match
    // (single player testing on localhost) instead runs until that player tops out
    int min_alive = (session.count > 1) ? 2 : 1;

    // Authoritative tick loop; non-blocking
    while (countAlivePlayers(&session) >= min_alive)
    {
        /* --- Drain whatever the clients sent since the last tick --- */
        // Bounded so a flood of packets can never starve the simulation below
        for (int drained = 0; drained < MAX_MSGS_PER_TICK; drained++)
        {
            if (brserver_get_app_msg(buffer) != 1)
            {
                break; // Queue is empty
            }

            uint32_t tag = readPacketTag(buffer);

            if (tag == PACKET_INPUT)
            {
                InputPayload input;
                unpackInput(buffer, &input);

                // Perform the requested action on that player's real board
                PlayerSlot *slot = findPlayer(&session, input.player_id);
                applyAction(&session, slot, (PlayerAction)input.action);
            }
            // Any other tag is ignored
        }

        /* --- Advance every board by one tick --- */
        tickSession(&session);
        logClientDrain(&logClient, LOG_CLIENT_DRAIN_BATCH);
        // Elimination Check
        for (int i = 0; i < session.count; i++)
        {
            PlayerSlot *slot = &session.players[i];
            if (slot->state.game_over && slot->active) // just topped out this tick
            {
                logMessage(LOG_LEVEL_INFO, "[tetrisd] P%u has been eliminated.", slot->player_id);
            }
        }

        
        /* --- Route any garbage the tick produced --- */
        for (int i = 0; i < session.count; i++)
        {
            PlayerSlot *attacker = &session.players[i];

            if (attacker->state.outgoing_garbage == 0)
            {
                continue;
            }

            uint32_t lines = attacker->state.outgoing_garbage;
            attacker->state.outgoing_garbage = 0; // Consumed

            // Resolve the victim using the server's roster
            uint32_t victim_id = resolveTargetID(&attacker->state, &session.roster);
            PlayerSlot *victim = findPlayer(&session, victim_id);

            if (victim == NULL || victim == attacker || victim->state.game_over)
            {
                continue; // No valid target, damage is dropped
            }

            queueGarbage(&victim->state, (int)lines);
            victim->state.last_attacker_id = attacker->player_id;
            victim->dirty = true; // Their board changed, push it

            logMessage(LOG_LEVEL_INFO, " <!> EVENT ROUTED: P%u attacked P%u with %u lines!", attacker->player_id, victim_id, lines);

            // Broadcast the attack itself so every client's kill feed updates
            AttackPayload feed = {
                .source_player = attacker->player_id,
                .target_player = victim_id,
                .lines = lines};

            unsigned char feed_buffer[512] = {0};
            packAttack(feed_buffer, &feed);
            brserver_send_broadcast(server, feed_buffer);
        }

        /* --- Push state to anyone whose board changed --- */
        // Send-on-change keeps traffic low, the keepalive repairs dropped UDP packets
        for (int i = 0; i < session.count; i++)
        {
            PlayerSlot *slot = &session.players[i];

            if (!slot->dirty && slot->idle_ticks < KEEPALIVE_TICKS)
            {
                slot->idle_ticks++;
                continue; // Nothing new to say about this player yet
            }

            StatePayload snapshot;
            buildStatePayload(&slot->state, &snapshot);

            unsigned char state_buffer[512] = {0};
            packState(state_buffer, &snapshot);
            brserver_send_broadcast(server, state_buffer);

            slot->dirty = false;
            slot->idle_ticks = 0;
        }

        usleep(TICK_MICROSECONDS); // Advance at a fixed rate
    }

    /* --- MATCH OVER --- */
    // Announce the survivor, if there is one
    uint32_t winner_id = 0;
    for (int i = 0; i < session.count; i++) // Loop through all players
    {
        if (session.players[i].active && !session.players[i].state.game_over) // Player must be active and have not topped out
        {
            winner_id = session.players[i].player_id;
            break;
        }
    }

    if (winner_id != 0) // Found a winner
    {
        printf("\n[tetrisd] MATCH OVER! Winner: P%u\n", winner_id);
    }
    else
    {
        // No winner found
        printf("\n[tetrisd] MATCH OVER! No survivors.\n");
    }

    // Mark everyone finished and push one final state each so every client can exit its loop cleanly
    for (int i = 0; i < session.count; i++)
    {
        PlayerSlot *slot = &session.players[i];

        if (!slot->active)
        {
            continue; // Already gone, nothing listening
        }

        slot->state.game_over = true;
        slot->state.winner_id = winner_id;

        StatePayload snapshot;
        buildStatePayload(&slot->state, &snapshot);

        unsigned char state_buffer[512] = {0};
        packState(state_buffer, &snapshot);
        brserver_send_broadcast(server, state_buffer);
    }

    logMessage(LOG_LEVEL_INFO, "[tetrisd] Shutting down. %u log record(s) dropped this run.", logClientGetDroppedCount(&logClient));
    logClientDrain(&logClient, LOG_CLIENT_DRAIN_BATCH); // Send that very last line too
    logClientClose(&logClient);

    // Give the final broadcast a moment to land before tearing down the sockets
    usleep(500000); // 500ms

    // Clean up
    brserver_end(server);
    return 0;
}