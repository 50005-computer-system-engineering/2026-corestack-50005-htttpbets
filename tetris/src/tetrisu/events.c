#include <stddef.h>
#include <string.h>
#include <arpa/inet.h>
#include "events.h"
#include "lib/libtetrisbrain/state.h"
#include "lib/libtetrisbrain/killfeed.h"
#include "lib/libbattleroyale/client.h"

// Instantiate libhtttp
extern BRClient *network_client;

// Network Routing
void on_attack_generated(void *args)
{
    // Arm the payload
    AttackPayload *attack = (AttackPayload *)args;

    // Update kill feed (locally)
    addKillFeed(attack->source_player, attack->target_player, attack->lines);

    // Network routing (endian-safe)
    if (network_client != NULL)
    {
        // Pack network struct and convert into standard network format
        AttackPayload net_payload;
        net_payload.source_player = htonl(attack->source_player);
        net_payload.target_player = htonl(attack->target_player);
        net_payload.lines = htonl(attack->lines);

        // Package into the buffer
        unsigned char buffer[512] = {0};
        // Copy into the buffer
        memcpy(buffer, &net_payload, sizeof(AttackPayload));

        // Send message
        brclient_send_msg(network_client, buffer);
    }
}