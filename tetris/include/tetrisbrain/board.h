#ifndef TETRISBRAIN_BOARD_H
#define TETRISBRAIN_BOARD_H

#include <stdint.h>
#include <stdbool.h>
#include "state.h"

/* ----- BOARD LOGISTICS ----- */
#define BOARD_WIDTH 10
#define BOARD_HEIGHT 20

// Check for collisions
bool isValidPos(GameState *state, PieceType type, Rotation rot, int posX, int posY);

// Locking the piece after it finalizes its position
void lockPiece(GameState *state);

// Tetris
int clearLines(GameState *state);

/* ----- BOARD ----- */
typedef struct
{
    uint8_t cells[BOARD_HEIGHT][BOARD_WIDTH];
} Board;

#endif