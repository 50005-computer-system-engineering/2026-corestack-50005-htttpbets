/* FOR EVENT BUS; PAYLOADS AND EVENTS */

#ifndef TETRISU_EVENTS_H
#define TETRISU_EVENTS_H

// Event Dictionary
enum GameEvents
{
    EVENT_ATTACK_GENERATED,
    EVENT_COUNT
};

// Data Package
typedef struct
{
    int source_player; // Player that sent it
    int target_player; // Player that recieves it
    int lines;         // How much damage
} AttackPayload;

// Test Network Routing
void on_attack_generated(void *args);

#endif