#ifndef EVENTS_H
#define EVENTS_H

#include <stdbool.h>
typedef enum {
    // Input
    EVENT_INPUT_MOVE_PERFORMING,
    EVENT_INPUT_MOVE_RELEASED,
    EVENT_INPUT_BOMB_PRESSED,
    EVENT_INPUT_EXIT_PRESSED,

    EVENT_INPUT_SPRINT_CHANGED,

    // Events
    EVENT_BOMB_EXPLODED,

    // Final Count
    EVENT_COUNT
} Event;

// Event Arguments
#pragma region Input Arguments
typedef struct {
    int player_id; // TODO
} InputEventArgs;

typedef struct {
    int player_id; // TODO
    int x, y; // Direction
} MoveEventArgs;

typedef struct {
    int player_id; // TODO
    bool toggled;
} SprintEventArgs;
#pragma endregion Input Arguments

typedef struct {
    int x;
    int y;
} TileEventArgs;
#endif
