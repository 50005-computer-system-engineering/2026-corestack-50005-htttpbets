#include <stddef.h>
#include <arpa/inet.h>
#include "events.h"
#include "lib/libtetrisbrain/state.h"
#include "lib/libtetrisbrain/killfeed.h"
#include "lib/libhtttp/client.h"

extern LibhtttpClient *network_client;

// Test Network Routing
void on_attack_generated(void *args)
{
    // Arm the payload
    AttackPayload *attack = (AttackPayload *)args;

    // Update kill feed (locally)
    addKillFeed(attack->source_player, attack->target_player, attack->lines);

    // Network routing (endian-safe)
    if (network_client != NULL)
    {
        // Convert into standard network format
        AttackPayload net_payload;
        net_payload.source_player = htonl(attack->source_player);
        net_payload.target_player = htonl(attack->target_player);
        net_payload.lines = htonl(attack->lines);

        // Send converted struct
        sendAsClient(network_client, sizeof(AttackPayload), (unsigned char *)&net_payload);
    }
}