#ifndef TETRISBRAIN_PIECE_H
#define TETRISBRAIN_PIECE_H
#include "state.h"

/* ----- TETRIS PIECES ----- */
typedef enum
{
    PIECE_NONE = 0,
    PIECE_I = 1, // Straight line of 4 blocks
    PIECE_O = 2, // Square block
    PIECE_T = 3, // T-piece
    PIECE_S = 4, // Z shape, offset to the right
    PIECE_Z = 5, // Z shape, offset to the left
    PIECE_J = 6, // Reverse L shape pointing to the left
    PIECE_L = 7  // Reverse L shape pointing to the right
} PieceType;

/* ----- ROTATION (SUPER ROTATION SYSTEM) ----- */
typedef enum
{
    ROT_0 = 0, // SPAWN
    ROT_1 = 1, // RIGHT
    ROT_2 = 2, // 180 DEG
    ROT_3 = 3  // LEFT
} Rotation;

/* ----- ACTIVE PIECE ----- */
typedef struct
{
    PieceType type;
    Rotation rot;
    int x; // Location on the board (x-axis)
    int y; // Location on the board (y-axis)
} Piece;

extern const int tetrominoes[7][16];

// Convert 2D (x,y) coordinates into 1D index for array
int getRotationIndex(int x, int y, Rotation rot);

// Wall kick helper function
bool testRotate(GameState *state, int nextRot);

// Rotate clockwise logic
void rotateCurrentPiece(GameState *state);

// Rotate counter clockwise logic
void rotateCounterClockwise(GameState *state);

#endif