#include "board.h"
#include "state.h"
#include "piece.h"

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