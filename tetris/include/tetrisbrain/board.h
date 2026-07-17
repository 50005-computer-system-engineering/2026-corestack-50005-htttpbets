#ifndef TETRISBRAIN_BOARD_H
#define TETRISBRAIN_BOARD_H

#include <stdint.h>
#include <stdbool.h>
#include "state.h"
#include "piece.h"

/* ----- BOARD LOGISTICS ----- */
#define BOARD_WIDTH 10
#define BOARD_HEIGHT 20

// Forward declaration to avoid circular includes
struct GameState;

// Check for collisions
bool isValidPos(struct GameState *state, PieceType type, Rotation rot, int posX, int posY);

// Locking the piece after it finalizes its position
void lockPiece(struct GameState *state);

// Tetris
int clearLines(struct GameState *state);

/* ----- BOARD ----- */
typedef struct
{
    uint8_t cells[BOARD_HEIGHT][BOARD_WIDTH];
} Board;

#endif