#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <stdlib.h>
#include "tetrisbrain.h"

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

// Function call to start the game
void startGame(GameState *state)
{
    // reset game state
    memset(state, 0, sizeof(GameState));

    // reset game variables
    state->score = 0;
    state->game_over = 0;
    state->lines_cleared = 0;
    state->level = 1;

    // Start spawning first piece
    spawnNewPiece(state);
}

// Spawns a new piece
void spawnNewPiece(GameState *state)
{
    // Pick a random piece
    state->current.type = (PieceType)((rand() % 7) + 1);

    state->current.rot = ROT_0;               // Default rotation
    state->current.x = (BOARD_WIDTH / 2) - 2; // Centered horizontally
    state->current.y = -2;                    // Top of the board (nudged it above to allow for top out collision checks)
}

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

// Check for collisions
bool isValidPos(GameState *state, PieceType type, Rotation rot, int posX, int posY)
{
    int shapeIndex = type - 1; // Mapping directly to tetrominoes array

    // Keeping in bounds of 4x4
    for (int px = 0; px < 4; px++)
    {
        for (int py = 0; py < 4; py++)
        {
            int cellIndex = getRotationIndex(px, py, rot); // Fetch exact 1D index for current rotation
            // If this part of the 4x4 box is empty, skip it
            if (tetrominoes[shapeIndex][cellIndex] == 0)
                continue;

            // Global Positions
            int boardX = posX + px;
            int boardY = posY + py;

            // Out of bounds checks
            if (boardX < 0 || boardX >= BOARD_WIDTH || boardY >= BOARD_HEIGHT)
            {
                return false;
            }
            // Block Collision Check
            if (boardY >= 0)
            {
                if (state->board.cells[boardY][boardX] != 0)
                {
                    return false;
                }
            }
        }
    }
    // Move is valid
    return true;
}

// Rotation logic
void rotateCurrentPiece(GameState *state)
{
    // Calculate what the next rotation state would be (0 -> 1 -> 2 -> 3 -> 0)
    Rotation nextRot = (Rotation)((state->current.rot + 1) % 4);

    // Check if after rotation, is the new position valid
    if (isValidPos(state, state->current.type, nextRot, state->current.x, state->current.y))
    {
        state->current.rot = nextRot; // Successful rotation
    }
}

// Locking the piece after it finalizes its position
void lockPiece(GameState *state)
{
    int shapeIndex = state->current.type - 1; // Mapping directly to tetrominoes array

    // Keeping in bounds of 4x4
    for (int px = 0; px < 4; px++)
    {
        for (int py = 0; py < 4; py++)
        {
            int cellIndex = getRotationIndex(px, py, state->current.rot); // Which rotation?

            // Check if we are looking at a solid chunk of the piece / not empty space!!
            if (tetrominoes[shapeIndex][cellIndex] != 0)
            {
                // Find where the block lives on the board
                int boardX = state->current.x + px;
                int boardY = state->current.y + py;

                // Save final position onto the board
                if (boardY >= 0 && boardY < BOARD_HEIGHT)
                {
                    state->board.cells[boardY][boardX] = state->current.type;
                }
            }
        }
    }
}

// Tetris !
int clearLines(GameState *state)
{
    // Collective counter for later use
    int linesCleared = 0;

    // Once line is cleared, drop above row
    for (int y = BOARD_HEIGHT - 1; y >= 0; y--)
    {
        // Checking if row is complete
        bool isFull = true;
        for (int x = 0; x < BOARD_WIDTH; x++)
        {
            if (state->board.cells[y][x] == 0)
            {
                isFull = false;
                break;
            }
        }

        if (isFull)
        {
            linesCleared++;

            // Shift everything above this row down by 1
            for (int yy = y; yy > 0; yy--)
            {
                for (int xx = 0; xx < BOARD_WIDTH; xx++)
                {
                    state->board.cells[yy][xx] = state->board.cells[yy - 1][xx];
                }
            }

            // Clear the very top row to 0
            for (int xx = 0; xx < BOARD_WIDTH; xx++)
            {
                state->board.cells[0][xx] = 0;
            }

            // Check again if newly formed line is full
            y++;
        }
    }

    // Update internal score/lines
    if (linesCleared > 0)
    {
        state->lines_cleared += linesCleared;
        state->score += linesCleared * 100; // Basic placeholder scoring
    }

    return linesCleared;
}

// Repeat function to advance the game
int tickGame(GameState *state)
{
    // Attempt to let gravity pull the piece down
    if (isValidPos(state, state->current.type, state->current.rot, state->current.x, state->current.y + 1))
    {
        state->current.y++; // If possible, fall for one row
        return 0;           // 0 lines cleared
    }

    // Game Over Check 1
    if (!isValidPos(state, state->current.type, state->current.rot, state->current.x, state->current.y + 1))
    {
        // Lock Out
        if (state->current.y < 0)
        {
            state->game_over = true;
            return 0;
        }
    }

    // Otherwise collision detected, so cannot move
    lockPiece(state);                // Store in game state
    int cleared = clearLines(state); // Check if any rows need to be cleared

    // Spawn a new piece at the top
    spawnNewPiece(state);

    // Game Over Check
    if (!isValidPos(state, state->current.type, state->current.rot, state->current.x, state->current.y))
    {
        // Block Out
        state->game_over = true;
    }

    return cleared;
}