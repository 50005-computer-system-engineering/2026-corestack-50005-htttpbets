#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/select.h>
#include "lib/libbattleroyale/server.h"
#include "lib/libtetrisprotocol/protocol.h"
#include "lib/libtetrisbrain/garbage.h"
#include "game.h"

// MAX_LOBBY_SIZE defined in protocol.h
#define MIN_LOBBY_SIZE 2

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

int main(void)
{
    // Line-buffered so logs survive being piped to a file
    setvbuf(stdout, NULL, _IOLBF, 0);

    // Initialize server connection
    BRServer *server = NULL;
    if (brserver_init(&server) < 0) // Failed
    {
        printf("[tetrisd] Could not create server!\n");
        return -1;
    }
    printf("[tetrisd] Server created successfully! Opening lobby...\n");

    // LOBBY state and assign player IDs
    if (brserver_open(server) < 0) // Blocking server call, waits until lobby is filled
    {
        printf("[tetrisd] Could not create lobby!\n");
        return -1;
    }

    // Pre-allocation for clients
    uint32_t lobbySize = 0;
    uint32_t clientIds[MAX_LOBBY_SIZE] = {0};

    // Wait dynamically for lobby to fill
    printf("[tetrisd] Lobby open! Need at least %d player(s).\n", MIN_LOBBY_SIZE);
    printf("[tetrisd] Press ENTER at any time once ready to start the game.\n");
    while (1)
    {
        brserver_client_info(server, &lobbySize, clientIds);
        printf("\r[tetrisd] %u player(s) connected...\n", lobbySize);
        fflush(stdout);

        if (lobbySize >= MIN_LOBBY_SIZE && enterPressed())
        {
            printf("\n[tetrisd] Starting game with %u players!\n", lobbySize);
            break;
        }
        if (lobbySize >= MAX_LOBBY_SIZE)
        {
            printf("\n[tetrisd] Lobby full (%u), starting automatically.\n", lobbySize);
            break;
        }
        usleep(100000); // Poll every 100ms
    }

    // Once lobby is full
    printf("[tetrisd] Lobby filled! %u player(s) connected:", lobbySize);
    for (uint32_t i = 0; i < lobbySize; i++)
    {
        printf(" P%u", clientIds[i]);
    }
    printf("\n");

    // Signal listening for events -> switch to GAME state to start
    if (brserver_start(server) < 0)
    {
        printf("[tetrisd] Failed to start game!\n");
        return -1;
    }
    printf("[tetrisd] Server is now in GAME state. Awaiting events...\n");

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

        // TODO: Send setup data (explicitly uses TCP here) -> supposed to but causing deadlock so set to UDP
        if (brserver_send_broadcast(server, roster_buffer) < 0)
        {
            printf("[tetrisd] Warning: failed to broadcast player roster.\n");
        }
        else
        {
            printf("[tetrisd] Roster broadcast to all %u players.\n", lobbySize);
        }
    }

    // Build the authoritative match, one real GameState per connected player
    // From here the server owns the boards, clients only render copies of them
    GameSession session;
    initSession(&session, clientIds, lobbySize);
    printf("[tetrisd] Authoritative session started for %d player(s).\n", session.count);

    // Array buffer for message queue
    unsigned char buffer[512] = {0};

    // Authoritative tick loop; non-blocking
    while (1)
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

            addGarbage(&victim->state, (int)lines);
            victim->state.last_attacker_id = attacker->player_id;
            victim->dirty = true; // Their board changed, push it

            printf(" <!> EVENT ROUTED: P%u attacked P%u with %u lines!\n", attacker->player_id, victim_id, lines);

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

    // Clean up
    brserver_end(server);
    printf("[tetrisd] Client disconnected or error occurred. Shutting down.\n");
    return 0;
}