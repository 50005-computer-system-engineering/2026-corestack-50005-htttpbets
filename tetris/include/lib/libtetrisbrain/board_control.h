#ifndef TETRISBRAIN_BOARD_CONTROL_H
#define TETRISBRAIN_BOARD_CONTROL_H

#include "state.h"
#include <stdbool.h>

/* ----- PIECE LOGISTICS ----- */
// Convert 2D (x,y) coordinates into 1D index for array
int getRotationIndex(int x, int y, Rotation rot);

// Wall kick helper function
bool testRotate(GameState *state, int nextRot);

// Rotate clockwise logic
void rotateCurrentPiece(GameState *state);

// Rotate counter clockwise logic
void rotateCounterClockwise(GameState *state);

/* ----- BOARD LOGISTICS ----- */
// Check for collisions
bool isValidPos(GameState *state, PieceType type, Rotation rot, int posX, int posY);

// Locking the piece after it finalizes its position
void lockPiece(GameState *state);

// Tetris
int clearLines(GameState *state);

#endif