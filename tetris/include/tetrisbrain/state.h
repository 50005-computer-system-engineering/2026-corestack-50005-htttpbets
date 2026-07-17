#ifndef TETRISBRAIN_STATE_H
#define TETRISBRAIN_STATE_H

#include "board.h"
#include "piece.h"

/* ----- GAME STATE ----- */
typedef struct
{
    Board board;
    Piece current;
    Piece next;
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

// Function call to start the game
void startGame(GameState *state);

#endif