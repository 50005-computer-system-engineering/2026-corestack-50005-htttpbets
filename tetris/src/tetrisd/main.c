#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/select.h>
#include "lib/libbattleroyale/server.h"
#include "tetrisu/events.h"

// MAX_LOBBY_SIZE defined in events.h
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
        roster.count = htonl(lobbySize); // Total number of players
        for (uint32_t i = 0; i < lobbySize && i < MAX_LOBBY_SIZE; i++)
        {
            roster.ids[i] = htonl(clientIds[i]); // Add dynamically to array
        }
        unsigned char roster_buffer[512] = {0}; // Empty buffer
        uint32_t tag = htonl(PACKET_ROSTER);    // Identify action once recieved

        // Copy RosterPayload and prep to send via broadcast
        memcpy(roster_buffer, &tag, sizeof(tag));
        memcpy(roster_buffer + sizeof(tag), &roster, sizeof(RosterPayload));

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

    // Per-player garbage accumulator based on player IDs
    // lookup table to mark which IDs are connected
    // player - 0 is for the server, not a valid target
    uint32_t maxId = 0;
    for (uint32_t i = 0; i < lobbySize; i++)
    {
        if (clientIds[i] > maxId)
        {
            maxId = clientIds[i];
        }
    }

    // Validate all players -> calloc sets all elements to false (0) by default
    bool *isValidPlayer = calloc(maxId + 1, sizeof(bool)); // To check if target player ID is connected
    if (isValidPlayer == NULL)                             // Failed
    {
        printf("[tetrisd] Failed to validate player!\n");
        return -1;
    }
    for (uint32_t i = 0; i < lobbySize; i++)
    {
        isValidPlayer[clientIds[i]] = true; // Mark connected player IDs as valid to ensure we only route attacks to real players
    }

    // Array buffer for message queue
    unsigned char buffer[512] = {0};

    // Continuous listening loop; non-blocking
    while (1)
    {
        // Read message from message queue
        if (brserver_get_app_msg(buffer) == 1)
        {
            // Extract 4-byte tag to identify action
            uint32_t tag;
            memcpy(&tag, buffer, sizeof(tag));
            tag = ntohl(tag);

            if (tag != PACKET_ATTACK)
            {
                continue; // Ignore (should not happen)
            }

            // Cast the raw byte buffer back into our struct
            AttackPayload incoming;
            memcpy(&incoming, buffer + sizeof(tag), sizeof(AttackPayload));

            // Convert the network bytes back to readable integers
            uint32_t real_source = ntohl(incoming.source_player);
            uint32_t real_target = ntohl(incoming.target_player);
            uint32_t real_lines = ntohl(incoming.lines);

            // Logging
            printf(" <!> EVENT ROUTED: (In-Game P%u) attacked P%u with %u lines!\n", real_source, real_target, real_lines);

            // Validates that target player ID exists and actively connected
            if (real_target <= maxId && isValidPlayer[real_target])
            {
                // Prepare out buffer
                unsigned char out_buffer[512] = {0};
                memcpy(out_buffer, buffer, sizeof(tag) + sizeof(AttackPayload)); // Inclusive of the tag

                // Send the payload via UDP broadcast to all clients
                // Clients will be responsible for parsing the payload and checking if they are the target
                brserver_send_broadcast(server, out_buffer);

                // Logging
                printf("-> [Server] Broadcasted %u garbage lines to Target P%u\n\n", real_lines, real_target);
            }
        }
        usleep(100000); // Poll every 100ms
    }

    // Clean up
    brserver_end(server);
    free(isValidPlayer);
    printf("[tetrisd] Client disconnected or error occurred. Shutting down.\n");
    return 0;
}