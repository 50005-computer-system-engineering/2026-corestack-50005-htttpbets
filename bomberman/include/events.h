#ifndef EVENTS_H
#define EVENTS_H

typedef enum {
    // Input
    EVENT_INPUT_MOVE_PERFORMING,
    EVENT_INPUT_MOVE_RELEASED,
    EVENT_INPUT_BOMB_PRESSED,
    EVENT_INPUT_EXIT_PRESSED,

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
#pragma endregion Input Arguments
#endif
