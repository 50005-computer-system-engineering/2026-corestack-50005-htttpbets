#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <arpa/inet.h>
#include "lib/libhtttp/server.h"
#include "tetrisu/events.h"

#define LOBBY_SIZE 2

int main(void)
{
    // Initialize server connection
    LibhtttpServer *server = NULL;
    if (createServer(&server) < 0) // Failed
    {
        printf("[tetrisd] Could not create server!\n");
        return -1;
    }
    printf("[tetrisd] Server created successfully! Waiting for connections...\n");

    // Create lobby and assign player IDs
    uint32_t lobbySize = LOBBY_SIZE;
    uint32_t *clientIds = NULL;
    if (openLobby(server, &lobbySize, &clientIds) < 0) // Blocking server call, waits until lobby is filled
    {
        printf("[tetrisd] Could not create lobby!\n");
        return -1;
    }
    printf("[tetrisd] Lobby created successfully! %u player(s) connected:", lobbySize);
    // Print all layers for tracking purposes
    for (uint32_t i = 0; i < lobbySize; i++)
    {
        printf(" P%u", clientIds[i]);
    }
    printf("\n");

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
    uint32_t *heldGarbage = calloc(maxId + 1, sizeof(uint32_t)); // Store pending garbage accumulated for each player ID
    bool *isValidPlayer = calloc(maxId + 1, sizeof(bool));       // To check if target player ID is connected
    if (heldGarbage == NULL || isValidPlayer == NULL)            // Failed
    {
        printf("[tetrisd] Failed to allocate player tracking!\n");
        free(clientIds);
        free(heldGarbage);
        free(isValidPlayer);
        return -1;
    }
    for (uint32_t i = 0; i < lobbySize; i++)
    {
        isValidPlayer[clientIds[i]] = true; // Mark connected player IDs as valid
    }
    free(clientIds); // Clean up once lookup mask is populated

    // Signal listening for events
    printf("[tetrisd] Both clients connected! Awaiting events...\n");

    // Empty buffer to store
    unsigned char *buffer = NULL;
    uint32_t source_id = 0;

    // Continuous listening loop; blocking
    while (listenForClientMsg(server, &source_id, &buffer) == 0)
    {
        // Cast the raw byte buffer back into our struct
        AttackPayload *incoming_attack = (AttackPayload *)buffer;

        // Convert the network bytes back to readable integers
        uint32_t real_source = ntohl(incoming_attack->source_player);
        uint32_t real_target = ntohl(incoming_attack->target_player);
        uint32_t real_lines = ntohl(incoming_attack->lines);

        // Logging
        printf(" <!> EVENT ROUTED: libhtttp Client %u (In-Game P%u) attacked P%u with %u lines!\n", source_id, real_source, real_target, real_lines);

        if (real_target <= maxId && isValidPlayer[real_target]) // Validates that target player ID exists and actively connected
        {
            // Prep payload to send to other client
            AttackPayload broadcast_payload;
            broadcast_payload.source_player = htonl(real_source);
            broadcast_payload.target_player = htonl(real_target);
            broadcast_payload.lines = htonl(real_lines);

            // Send the payload
            sendBroadcastToClients(server, sizeof(AttackPayload), (unsigned char *)&broadcast_payload);
            
            // Logging
            printf("-> [Server] Broadcasted %u garbage lines to Target P%u\n\n", real_lines, real_target);
        }
        else // Target disconnected or DNE; logs warning and discards
        {
            printf("-> [Server] WARNING: Target P%u not found. Discarding attack.\n\n", real_target);
        }

        // Free and clear buffer
        free(buffer);
        buffer = NULL;
    }

    // Clean up
    free(heldGarbage);
    free(isValidPlayer);

    printf("[tetrisd] Client disconnected or error occurred. Shutting down.\n");
    return 0;
}