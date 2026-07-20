/* FOR EVENT BUS; PAYLOADS AND EVENTS */

#ifndef TETRISU_EVENTS_H
#define TETRISU_EVENTS_H

#include <stdint.h>

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

// Test Network Routing
void on_attack_generated(void *args);

#endif