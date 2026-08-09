#ifndef TETRISU_EVENTS_H
#define TETRISU_EVENTS_H

#include <stdint.h>

// Shared lobby cap
#define MAX_LOBBY_SIZE 16 // TO BE KEPT IN SYNC

// Network byte order
typedef enum
{
    PACKET_ATTACK = 0, // To signal AttackPayload
    PACKET_ROSTER = 1  // To signal RosterPayload
} PacketType;

// Event Dictionary
enum GameEvents
{
    EVENT_ATTACK_GENERATED,
    EVENT_COUNT
};

// Data Package
typedef struct
{
    uint32_t source_player; // Player that sent it
    uint32_t target_player; // Player that recieves it
    uint32_t lines;         // How much damage
} AttackPayload;

// Roster Data Package
// Broadcast from server -> all clients once the game starts
// Each client knows who else is in the lobby as well
typedef struct
{
    uint32_t count;               // Determined lobby size
    uint32_t ids[MAX_LOBBY_SIZE]; // Connected players
} RosterPayload;

// Network Routing
void on_attack_generated(void *args);

#endif