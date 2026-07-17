#include "engine.h"
#include "board.h"
#include "bag.h"
#include "garbage.h"

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
    bool tspin = checkTSpin(state);
    if (tspin)
    {
        state->t_spins++;
    }

    // Otherwise collision detected, so cannot move
    lockPiece(state); // Store in game state
    state->pieces_placed++;
    int cleared = clearLines(state); // Check if any rows need to be cleared

    // Dealing with Garbage
    int damage = calculateGarbage(state, cleared, tspin);
    // Defend Phase: Cancel incoming garbage with our attack
    if (damage > 0 && state->pending_garbage > 0)
    {
        if (damage >= state->pending_garbage)
        {
            damage -= state->pending_garbage;
            state->pending_garbage = 0;
        }
        else
        {
            state->pending_garbage -= damage;
            damage = 0;
        }
    }
    // Attack Phase: Send remaining damage to opponent
    state->outgoing_garbage = damage;
    // Recieve Phase: eat that shit up
    if (cleared == 0 && state->pending_garbage > 0)
    {
        addGarbage(state, state->pending_garbage);
        state->pending_garbage = 0;
    }

    // Spawn a new piece at the top AFTER garbage has been settled
    spawnNewPiece(state);

    // Game Over Check
    if (!isValidPos(state, state->current.type, state->current.rot, state->current.x, state->current.y))
    {
        // Block Out
        state->game_over = true;
    }

    return cleared;
}