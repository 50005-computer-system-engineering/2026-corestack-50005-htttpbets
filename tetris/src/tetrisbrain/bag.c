#include "tetrisbrain/bag.h"
#include <stdlib.h>

// Helper method to shuffle arrays
void shuffleArray(int *array, int size)
{
    for (int i = size - 1; i > 0; i--)
    {
        int j = rand() % (i + 1);
        int temp = array[i];
        array[i] = array[j];
        array[j] = temp;
    }
}

// Shifts the upcoming bag forward and generates a new one
void refillBag(GameState *state)
{
    for (int i = 0; i < 7; i++)
    {
        state->bag[i] = state->bag[i + 7]; // Copy last 7 pieces and add to the first 7 slots
    }

    int new_bag[7] = {1, 2, 3, 4, 5, 6, 7};
    shuffleArray(new_bag, 7);
    for (int i = 0; i < 7; i++)
    {
        state->bag[i + 7] = new_bag[i]; // Generate new 7 pieces and put to the back 7 slots
    }

    state->bag_index = 0; // Reset bag index
}

// Spawns a new piece
void spawnNewPiece(GameState *state)
{
    // Draw next piece from the bag
    state->current.type = state->bag[state->bag_index];
    state->bag_index++;

    // Only shuffle when bag is empty
    if (state->bag_index >= 7)
    {
        refillBag(state);
    }

    state->current.rot = ROT_0;               // Default rotation
    state->current.x = (BOARD_WIDTH / 2) - 2; // Centered horizontally
    state->current.y = -2;                    // Top of the board (nudged it above to allow for top out collision checks)
    state->has_held = false;                  // Ensure player is not holding any piece at start
}