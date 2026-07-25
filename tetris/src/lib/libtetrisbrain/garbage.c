#include "garbage.h"
#include <stdlib.h>

// Inject garbage lines at the bottom of the board
void addGarbage(GameState *state, int lines)
{
    // If 0 or -ve garbage passed, do nothing
    if (lines <= 0)
    {
        return;
    }

    // Bounds check in case garbage sent > board height
    if (lines > BOARD_HEIGHT)
    {
        lines = BOARD_HEIGHT;
    }

    // Bump all existing blocks up by the number of garbage lines
    for (int y = 0; y < BOARD_HEIGHT - lines; y++) // Loop from top of the board down to highest row
    {
        for (int x = 0; x < BOARD_WIDTH; x++) // Loop through x axis
        {
            state->board.cells[y][x] = state->board.cells[y + lines][x]; // Shift whole row up
        }
    }

    // Add the new garbage lines at the bottom
    for (int y = BOARD_HEIGHT - lines; y < BOARD_HEIGHT; y++) // Loop through newly created empty space at bottom of board
    {
        // Random x coord to spawn a hole
        int hole = rand() % BOARD_WIDTH;

        for (int x = 0; x < BOARD_WIDTH; x++) // Loop across x axis
        {
            if (x == hole)
            {
                // Gap
                state->board.cells[y][x] = 0;
            }
            else
            {
                // Fill with solid tile; used 8 as a separate ID since 0-7 already in use
                state->board.cells[y][x] = 8;
            }
        }
    }
}

// Calculate garbage based on lines cleared (following TETR.IO guideline rules)
int calculateGarbage(GameState *state, int lines_cleared, bool is_t_spin)
{
    // Fake gamestate
    (void)state;
    // No lines cleared = 0 damage
    if (lines_cleared == 0)
    {
        return 0;
    }

    // Counter
    int garbage = 0;

    // T spins deal more garbage (2x multiplier)
    if (is_t_spin)
    {
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