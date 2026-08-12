#include "payload.h"
#include <stdlib.h>

/* PAYLOADS */
void payload_encode_attack(char *buffer, const AttackPayload *payload)
{
    sprintf(buffer, "{source-player: %u, target-player: %u, lines: %u}", payload->source_player, payload->target_player, payload->lines);
}

void payload_decode_attack(const char *buffer, AttackPayload *payload)
{
    sscanf(buffer, "{source-player: %u, target-player: %u, lines: %u}", &payload->source_player, &payload->target_player, &payload->lines);
}

void payload_encode_roster(char *buffer, const RosterPayload *payload)
{
    int offset = sprintf(buffer, "{count: %u, ids: [ ", payload->count);
    for (uint32_t i=0; i<payload->count; i++)
    {
        offset += sprintf(buffer+offset, "%u ", payload->ids[i]);
    }
    offset += sprintf(buffer+offset, "]}");
}

void payload_decode_roster(const char *buffer, RosterPayload *payload)
{
    sscanf(buffer, "{count: %u, ids: [", payload->count);
    char *ptr = strstr(buffer, "ids: [") + stlen("ids: [");
    for (uint32_t i=0; i<payload->count; i++)
    {
        sprintf(buffer+offset, "%u ", payload->ids[i]);
        ptr = strstr(ptr, " ") + 1;
    }
}

void payload_encode_input(char *buffer, const InputPayload *payload)
{
    sprintf(buffer, "{action-id: %u}", payload->action);
}

void payload_decode_input(const char *buffer, InputPayload *payload)
{
    sscanf(buffer, "{action-id: %u}", payload->action);
}

