#include "board_control.h"

#pragma region Piece Control
const int tetrominoes[7][16] = {
    // Read-only 2D array => 7 pieces, 4x4 grid flattened into 1D list of numbers (16)
    // 1: solid piece, 0: empty space
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
    // Determine rotation state
    switch (rot)
    {
    case ROT_0: // 0 degrees
        return x + (y * 4);
    case ROT_1: // 90 degrees CW
        return 12 + y - (x * 4);
    case ROT_2: // 180 degrees
        return 15 - (y * 4) - x;
    case ROT_3: // 270 degrees CCW
        return 3 - y + (x * 4);
    default:
        return 0;
    }
}

// Helper to identify rotation transition index (0 to 7)
static int getTransitionIndex(Rotation currentRot, Rotation nextRot)
{
    if (currentRot == ROT_0 && nextRot == ROT_1)
    {
        return 0; // 0 -> 90  (CW)
    }
    if (currentRot == ROT_1 && nextRot == ROT_0)
    {
        return 1; // 90 -> 0  (CCW)
    }
    if (currentRot == ROT_1 && nextRot == ROT_2)
    {
        return 2; // 90 -> 180 (CW)
    }
    if (currentRot == ROT_2 && nextRot == ROT_1)
    {
        return 3; // 180 -> 90 (CCW)
    }
    if (currentRot == ROT_2 && nextRot == ROT_3)
    {
        return 4; // 180 -> 270 (CW)
    }
    if (currentRot == ROT_3 && nextRot == ROT_2)
    {
        return 5; // 270 -> 180 (CCW)
    }
    if (currentRot == ROT_3 && nextRot == ROT_0)
    {
        return 6; // 270 -> 0   (CW)
    }
    if (currentRot == ROT_0 && nextRot == ROT_3)
    {
        return 7; // 0 -> 270   (CCW)
    }
    return 0;
}

// Wall kick helper function
bool testRotate(GameState *state, int nextRot)
{
    // JLSTZ Kick Offsets (8 transitions x 5 tests x 2 coords {X, Y})
    static const int kicks_jlstz[8][5][2] = {
        {{0, 0}, {-1, 0}, {-1, -1}, {0, 2}, {-1, 2}},
        {{0, 0}, {1, 0}, {1, 1}, {0, -2}, {1, -2}},
        {{0, 0}, {1, 0}, {1, 1}, {0, -2}, {1, -2}},
        {{0, 0}, {-1, 0}, {-1, -1}, {0, 2}, {-1, 2}},
        {{0, 0}, {1, 0}, {1, -1}, {0, 2}, {1, 2}},
        {{0, 0}, {-1, 0}, {-1, 1}, {0, -2}, {-1, -2}},
        {{0, 0}, {-1, 0}, {-1, 1}, {0, -2}, {-1, -2}},
        {{0, 0}, {1, 0}, {1, -1}, {0, 2}, {1, 2}}};

    // I-Piece Kick Offsets
    static const int kicks_i[8][5][2] = {
        {{0, 0}, {-2, 0}, {1, 0}, {-2, 1}, {1, -2}},
        {{0, 0}, {2, 0}, {-1, 0}, {2, -1}, {-1, 2}},
        {{0, 0}, {-1, 0}, {2, 0}, {-1, -2}, {2, 1}},
        {{0, 0}, {1, 0}, {-2, 0}, {1, 2}, {-2, -1}},
        {{0, 0}, {2, 0}, {-1, 0}, {2, -1}, {-1, 2}},
        {{0, 0}, {-2, 0}, {1, 0}, {-2, 1}, {1, -2}},
        {{0, 0}, {1, 0}, {-2, 0}, {1, 2}, {-2, -1}},
        {{0, 0}, {-1, 0}, {2, 0}, {-1, -2}, {2, 1}}};

    int transIndex = getTransitionIndex(state->current.rot, nextRot);
    PieceType type = state->current.type;

    for (int i = 0; i < 5; i++)
    {
        int offsetX, offsetY;
        // Select correct table
        if (type == 1) // I-Piece
        {
            offsetX = kicks_i[transIndex][i][0];
            offsetY = kicks_i[transIndex][i][1];
        }
        else if (type == 2) // O-Piece
        {
            offsetX = 0;
            offsetY = 0;
        }
        else // J, L, S, Z, T Pieces
        {
            offsetX = kicks_jlstz[transIndex][i][0];
            offsetY = kicks_jlstz[transIndex][i][1];
        }

        int nx = state->current.x + offsetX;
        int ny = state->current.y + offsetY;

        if (isValidPos(state, type, nextRot, nx, ny))
        {
            state->current.x = nx;
            state->current.y = ny;
            state->current.rot = nextRot;
            state->last_action_rotation = true;
            return true;
        }
    }
    return false; // All 5 SRS kick tests failed
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
#pragma endregion

#pragma region Board Control
// Check for collisions
bool isValidPos(GameState *state, PieceType type, Rotation rot, int posX, int posY)
{
    int shapeIndex = type - 1; // Mapping directly to tetrominoes array

    // Keeping in bounds of 4x4
    for (int px = 0; px < 4; px++)
    {
        for (int py = 0; py < 4; py++)
        {
            int cellIndex = getRotationIndex(px, py, rot); // Get exact 1D index for current rotation
            if (tetrominoes[shapeIndex][cellIndex] == 0)   // If this part of the 4x4 box is empty, do nothing
                continue;

            // Convert local 4x4 grid to global x / y positions
            int boardX = posX + px;
            int boardY = posY + py;

            // Out of bounds checks
            if (boardX < 0 || boardX >= BOARD_WIDTH || boardY >= BOARD_HEIGHT)
            {
                return false;
            }
            // Block Collision Check
            if (boardY >= 0 && state->board.cells[boardY][boardX] != 0)
            {
                return false;
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

                // Save final position onto the board (within bounds)
                if (boardY >= 0 && boardY < BOARD_HEIGHT)
                {
                    if (boardX >= 0 && boardX < BOARD_WIDTH)
                    {
                        // Lock piece permanently onto the board grid
                        state->board.cells[boardY][boardX] = state->current.type;
                    }
                }
            }
        }
    }
}

// Clear lines
int clearLines(GameState *state)
{
    // Collective counter for later use
    int linesCleared = 0;

    // Once line is cleared, drop above row
    for (int y = BOARD_HEIGHT - 1; y >= 0; y--) // Check from bottom row up to row 0
    {
        // Checking if row is complete
        bool isFull = true;
        for (int x = 0; x < BOARD_WIDTH; x++)
        {
            // Empty space detected
            if (state->board.cells[y][x] == 0)
            {
                isFull = false;
                break;
            }
        }
        // No empty space detected
        if (isFull)
        {
            linesCleared++; // Increment counter

            // Shift everything above this row down by 1
            for (int yy = y; yy > 0; yy--) // Loop through all rows from cleared row up to row 1
            {
                for (int xx = 0; xx < BOARD_WIDTH; xx++) // Loop across x axis
                {
                    // Shift content of one row above into current row
                    state->board.cells[yy][xx] = state->board.cells[yy - 1][xx];
                }
            }

            // Clear top row to 0
            for (int xx = 0; xx < BOARD_WIDTH; xx++) // Loop across x axis
            {
                state->board.cells[0][xx] = 0; // Clear entire row
            }

            // Check again if newly formed line is full
            y++; // Offset to check same row again
        }
    }

    // Update internal score/lines
    if (linesCleared > 0)
    {
        state->lines_cleared += linesCleared;
        state->level = (state->lines_cleared / 10) + 1; // Basic level progression
        state->score += linesCleared * 100;             // Basic placeholder scoring
    }
    // Update cleared count
    return linesCleared;
}
#pragma endregion