#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>
#include <arpa/inet.h>
#include "lib/libbattleroyale/server.h"
#include "tetrisu/events.h"

#define LOBBY_SIZE 2

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
    uint32_t clientIds[16] = {0};

    // Wait dynamically for lobby to fill
    printf("[tetrisd] Lobby created successfully! %u player(s) connected:\n", lobbySize);
    printf("[tetrisd] Waiting for %u connections...\n", LOBBY_SIZE);
    while (lobbySize < LOBBY_SIZE)
    {
        brserver_client_info(server, &lobbySize, clientIds);
        usleep(100000); // Poll every 100ms
    }

    // Print all layers for tracking purposes -> TO REMOVE AFTERWARDS
    for (uint32_t i = 0; i < lobbySize; i++)
    {
        printf(" P%u", clientIds[i]);
    }
    printf("\n");

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

    // Validate all players
    bool *isValidPlayer = calloc(maxId + 1, sizeof(bool)); // To check if target player ID is connected
    if (isValidPlayer == NULL)                             // Failed
    {
        printf("[tetrisd] Failed to validate player!\n");
        return -1;
    }
    for (uint32_t i = 0; i < lobbySize; i++)
    {
        isValidPlayer[clientIds[i]] = true; // Mark connected player IDs as valid
    }

    // Array buffer for libbattleroyale message queue
    unsigned char buffer[512] = {0};

    // Continuous listening loop; blocking
    while (1)
    {
        // Read message from message queue
        if (brserver_get_app_msg(buffer) == 1)
        {
            // Cast the raw byte buffer back into our struct
            AttackPayload incoming_attack;
            memcpy(&incoming_attack, buffer, sizeof(AttackPayload));

            // Convert the network bytes back to readable integers
            uint32_t real_source = ntohl(incoming_attack.source_player);
            uint32_t real_target = ntohl(incoming_attack.target_player);
            uint32_t real_lines = ntohl(incoming_attack.lines);

            // Logging
            printf(" <!> EVENT ROUTED: (In-Game P%u) attacked P%u with %u lines!\n", real_source, real_target, real_lines);

            // Validates that target player ID exists and actively connected
            if (real_target <= maxId && isValidPlayer[real_target])
            {
                // Prepare out buffer
                unsigned char out_buffer[512] = {0};

                // Prepare payload to send to other client
                AttackPayload broadcast_payload;
                broadcast_payload.source_player = htonl(real_source);
                broadcast_payload.target_player = htonl(real_target);
                broadcast_payload.lines = htonl(real_lines);

                // Add into buffer
                memcpy(out_buffer, &broadcast_payload, sizeof(AttackPayload));

                // Send the payload via UDP
                brserver_send_broadcast(server, out_buffer);

                // Logging
                printf("-> [Server] Broadcasted %u garbage lines to Target P%u\n\n", real_lines, real_target);
            }
            else // Target disconnected or DNE; logs warning and discards
            {
                printf("-> [Server] WARNING: Target P%u not found. Discarding attack.\n\n", real_target);
            }
        }
    }

    // Clean up
    brserver_end(server);
    free(isValidPlayer);
    printf("[tetrisd] Client disconnected or error occurred. Shutting down.\n");
    return 0;
}