#include "piece.h"
#include "state.h"
#include "board.h"

const int tetrominoes[7][16] = {
    // 7 pieces, 4X4 bounding box = 16 individual cells
    {0, 0, 0, 0, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0}, // I
    {0, 0, 0, 0, 0, 1, 1, 0, 0, 1, 1, 0, 0, 0, 0, 0}, // O
    {0, 0, 0, 0, 0, 1, 0, 0, 1, 1, 1, 0, 0, 0, 0, 0}, // T
    {0, 0, 0, 0, 0, 0, 1, 1, 0, 1, 1, 0, 0, 0, 0, 0}, // S
    {0, 0, 0, 0, 1, 1, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0}, // Z
    {0, 0, 0, 0, 0, 0, 0, 1, 0, 1, 1, 1, 0, 0, 0, 0}, // J
    {0, 0, 0, 0, 1, 0, 0, 0, 1, 1, 1, 0, 0, 0, 0, 0}  // L
};

// Convert 2D (x,y) coordinates into 1D index for array
int getRotationIndex(int x, int y, Rotation rot)
{
    switch (rot)
    {
    // Rotating the 4x4 grid
    case ROT_0:
        return x + (y * 4);
    case ROT_1:
        return 12 + y - (x * 4);
    case ROT_2:
        return 15 - (y * 4) - x;
    case ROT_3:
        return 3 - y + (x * 4);
    default:
        return 0;
    }
}

// Wall kick helper function
bool testRotate(GameState *state, int nextRot)
{
    // { X offset, Y offset }
    int kicks[5][2] = {{0, 0}, {-1, 0}, {1, 0}, {0, -1}, {0, -2}};

    for (int i = 0; i < 5; i++)
    {
        // Add offset to current coordinates
        int nx = state->current.x + kicks[i][0];
        int ny = state->current.y + kicks[i][1];

        if (isValidPos(state, state->current.type, nextRot, nx, ny))
        {
            // Apply the new coordinates and rotation
            state->current.x = nx;
            state->current.y = ny;
            state->current.rot = nextRot;

            // For t-spin detection
            state->last_action_rotation = true;
            return true;
        }
    }
    return false; // Kicking failed, piece cannot rotate
}

// Rotate clockwise logic
void rotateCurrentPiece(GameState *state)
{
    // Calculate what the next rotation state would be (0 -> 1 -> 2 -> 3 -> 0)
    Rotation nextRot = (Rotation)((state->current.rot + 1) % 4);
    testRotate(state, nextRot);
}

// Rotate clockwise logic
void rotateCounterClockwise(GameState *state)
{
    // Calculate what the next rotation state would be (3 -> 0 -> 1 -> 2 )
    int nextRot = (state->current.rot + 3) % 4;
    testRotate(state, nextRot);
}