#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include "lib/libhtttp/server.h"
#include "tetrisu/events.h"

int main(void)
{
    // Initialize server connection
    LibhtttpServer *server = NULL;
    if (createServer(&server) < 0) // Failed
    {
        printf("[tetrisd] Could not create server!\n");
        return -1;
    }
    printf("[tetrisd] Server created successfully!\n");

    // Create lobby (1 client for stopgap test)
    if (openLobby(server, 1) < 0)
    {
        printf("[tetrisd] Could not create lobby!\n");
        return -1;
    }
    printf("[tetrisd] Lobby created successfully!\n");

    // Signal listening for events
    printf("[tetrisd] Client connected! Awaiting events...\n");

    // Empty buffer to store
    unsigned char *buffer = NULL;

    // Continuous listening loop
    while (listenForClientMsg(server, &buffer) == 0)
    {
        // Cast the raw byte buffer back into our struct
        AttackPayload *incoming_attack = (AttackPayload *)buffer;

        // Convert the network bytes back to readable integers
        uint32_t real_source = ntohl(incoming_attack->source_player);
        uint32_t real_target = ntohl(incoming_attack->target_player);
        uint32_t real_lines = ntohl(incoming_attack->lines);

        // Logging
        printf(" <!> EVENT ROUTED: Player %u sent %u lines to Player %u!\n", real_source, real_lines, real_target);
        
        // Free and clear buffer
        free(buffer);
        buffer = NULL;
    }

    printf("[tetrisd] Client disconnected or error occurred. Shutting down.\n");

    return 0;
}