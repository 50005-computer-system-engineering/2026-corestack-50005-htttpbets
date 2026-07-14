#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <stdlib.h>
#include <time.h>
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

    // Generate starting bag of pieces
    srand(time(NULL)); // Seed for randomization

    // reset game variables
    state->held_type = 0;
    state->t_spins = 0;
    state->last_action_rotation = false;
    state->score = 0;
    state->game_over = 0;
    state->lines_cleared = 0;
    state->level = 1;

    // Start spawning first piece
    shuffleBag(state); // Shuffle first bag
    spawnNewPiece(state);
}

// Bagging system - fill the bag with the pieces and shuffle them randomly
void shuffleBag(GameState *state)
{
    // Fill up the bag sequentially
    for (int i = 0; i < 7; i++)
    {
        state->bag[i] = i + 1;
    }
    // Fisher-Yates shuffle -> ensures that our next bag is truly random with the same 7 pieces
    for (int i = 6; i > 0; i--)
    {
        int j = rand() % (i + 1); // Pick random index from 0 to i
        // Swap the pieces
        int temp = state->bag[i];
        state->bag[i] = state->bag[j];
        state->bag[j] = temp;
    }
    // Reset the draw index
    state->bag_index = 0;
}

// Spawns a new piece
void spawnNewPiece(GameState *state)
{
    // Only shuffle when bag is empty
    if (state->bag_index >= 7)
    {
        shuffleBag(state);
    }

    // Draw next piece from the bag
    state->current.type = state->bag[state->bag_index];
    state->bag_index++;

    state->current.rot = ROT_0;               // Default rotation
    state->current.x = (BOARD_WIDTH / 2) - 2; // Centered horizontally
    state->current.y = -2;                    // Top of the board (nudged it above to allow for top out collision checks)
    state->has_held = false;                  // Ensure player is not holding any piece at start
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

// Check for t-spin; return 1 if valid, 0 if not
int checkTSpin(GameState *state)
{
    // 1 & 2: Must be t-piece and last action must be rotation
    if (state->current.type != 3 || !state->last_action_rotation)
    {
        return 0;
    }

    // 3: 3 corner test
    int cx = state->current.x + 1;
    int cy = state->current.y + 1;
    int blocked_corners = 0;

    // Check for overhead
    int corner_offsets[4][2] = {{-1, -1}, {1, -1}, {-1, 1}, {1, 1}};

    for (int i = 0; i < 4; i++)
    {
        int checkX = cx + corner_offsets[i][0];
        int checkY = cy + corner_offsets[i][1];
        if (checkX < 0 || checkX >= BOARD_WIDTH || checkY >= BOARD_HEIGHT || checkY < 0)
        {
            blocked_corners++;
        }
        else if (checkY >= 0 && state->board.cells[checkY][checkX] != 0)
        {
            blocked_corners++;
        }
    }

    if (blocked_corners >= 3)
    {
        return 1; // T-spin confirmed
    }

    return 0; // Not t-spin
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

    // Check for t-spins
    if (checkTSpin(state))
    {
        state->t_spins++;
    }

    // Otherwise collision detected, so cannot move
    lockPiece(state); // Store in game state
    state->pieces_placed++;
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

// Hold piece
void holdPiece(GameState *state)
{
    // If a piece is already held, do nothing
    if (state->has_held)
    {
        return;
    }

    // If not holding anything, store the piece and spawn a new one
    if (state->held_type == 0)
    {
        state->held_type = state->current.type;
        spawnNewPiece(state);
    }
    else // Swap pieces
    {
        int temp = state->current.type;
        state->current.type = state->held_type;
        state->held_type = temp;

        // Reset piece to spawn at top of board
        state->current.y = -2;
        state->current.x = BOARD_WIDTH / 2 - 2;
        state->current.rot = 0;
    }

    // Set flag to true to prevent holding new pieces for this turn
    state->has_held = true;
}

// Inject garbage lines at the bottom of the board
void addGarbage(GameState *state, int lines)
{
    if (lines <= 0)
    {
        return;
    }

    // Bump all existing blocks up by the number of garbage lines
    for (int y = 0; y < BOARD_HEIGHT - lines; y++)
    {
        for (int x = 0; x < BOARD_WIDTH; x++)
        {
            state->board.cells[y][x] = state->board.cells[y + lines][x];
        }
    }

    // Random x coord to spawn hole for the player
    int hole = rand() % BOARD_WIDTH;

    // Add the new garbage lines at the bottom
    for (int y = BOARD_HEIGHT - lines; y < BOARD_HEIGHT; y++)
    {

        for (int x = 0; x < BOARD_WIDTH; x++)
        {
            if (x == hole)
            {
                // The gap
                state->board.cells[y][x] = 0;
            }
            else
            {
                // Used 8 as a separate ID since 0-7 already in use
                state->board.cells[y][x] = 8;
            }
        }
    }

    // Push falling piece up
    state->current.y -= lines;

    // Game over check
    if (!isValidPos(state, state->current.type, state->current.rot, state->current.x, state->current.y))
    {
        state->game_over = true;
    }
}

// Calculate garbage based on lines cleared (following TETR.IO guideline rules)
int calculateGarbage(GameState *state, int lines_cleared, bool is_t_spin)
{
    (void)state;
    if (lines_cleared == 0)
    {
        return 0;
    }

    int garbage = 0;

    if (is_t_spin)
    {
        // T spins deal more garbage (2x multiplier)
        if (lines_cleared > 0)
        {
            garbage = lines_cleared * 2;
        }
    }
    else
    {
        // Normal clears, regular multiplier
        if (lines_cleared == 2)
        {
            garbage = 1;
        }
        else if (lines_cleared == 3)
        {
            garbage = 3;
        }
        else if (lines_cleared == 4)
        {
            garbage = 4;
        }
    }

    return garbage;
}