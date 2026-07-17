#include "garbage.h"
#include <stdlib.h>

// Inject garbage lines at the bottom of the board
void addGarbage(GameState *state, int lines)
{
    if (lines <= 0)
    {
        return;
    }

    // Out of bounds check
    if (lines > BOARD_HEIGHT)
    {
        lines = BOARD_HEIGHT;
    }

    // Bump all existing blocks up by the number of garbage lines
    for (int y = 0; y < BOARD_HEIGHT - lines; y++)
    {
        for (int x = 0; x < BOARD_WIDTH; x++)
        {
            state->board.cells[y][x] = state->board.cells[y + lines][x];
        }
    }

    // Add the new garbage lines at the bottom
    for (int y = BOARD_HEIGHT - lines; y < BOARD_HEIGHT; y++)
    {
        // Random x coord to spawn hole for the player
        int hole = rand() % BOARD_WIDTH;

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