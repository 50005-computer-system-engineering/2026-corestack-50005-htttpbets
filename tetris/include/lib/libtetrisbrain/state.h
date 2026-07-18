#ifndef TETRISBRAIN_STATE_H
#define TETRISBRAIN_STATE_H

#include "board.h"
#include "piece.h"
#include <stdbool.h>

/* ----- GAME STATE ----- */
typedef enum
{
    TARGET_MANUAL, // Manual cycling
    TARGET_RANDOM, // Random opponent
    TARGET_KO,     // Highest stack
    TARGET_MODE_COUNT
} TargetingMode;

typedef struct
{
    Board board;
    Piece current;
    Piece next;
    TargetingMode target_mode;
    int player_id;
    int target_player_id;
    int held_type;
    bool has_held;
    int bag[14];
    int bag_index;
    int pieces_placed;
    int tetrises;
    int t_spins;
    bool last_action_rotation;
    int pending_garbage;
    int outgoing_garbage;
    int score;
    int lines_cleared;
    int level;
    int game_over;
} GameState;

// Spawn game state(s)
extern GameState gamestate_p1;
extern GameState gamestate_p2;

// Function call to start the game
void startGame(GameState *state);

#endif