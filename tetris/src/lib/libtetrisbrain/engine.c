#include "engine.h"
#include "board_control.h"
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

    // Check for {x, y} offsets for the 4 corners (top left, top right, bottom left, bottom right)
    int corner_offsets[4][2] = {{-1, -1}, {1, -1}, {-1, 1}, {1, 1}};
    // Through all 4 corners
    for (int i = 0; i < 4; i++)
    {
        // Checking x and y axis
        int checkX = cx + corner_offsets[i][0];
        int checkY = cy + corner_offsets[i][1];
        // If corner point is out of bounds
        if (checkX < 0 || checkX >= BOARD_WIDTH || checkY >= BOARD_HEIGHT || checkY < 0)
        {
            blocked_corners++;
        }
        // Within bounds but blocked
        else if (checkY >= 0 && state->board.cells[checkY][checkX] != 0)
        {
            blocked_corners++;
        }
    }
    // Minimum 3 corners blocked to qualify for t-spin
    if (blocked_corners >= 3)
    {
        return 1; // T-spin confirmed
    }

    return 0; // Not t-spin
}

// Function to advance the game / frame
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
        state->t_spins++; // Increment counter
    }

    // Otherwise collision detected, so cannot move
    lockPiece(state);                // Bake into the board grid
    state->pieces_placed++;          // Increment coutner
    int cleared = clearLines(state); // Clear full rows and store amount in counter

    // Dealing with Garbage
    int damage = calculateGarbage(state, cleared, tspin); // Number of garbage lines generated in total for this turn
    // Cancel incoming garbage with our attack
    if (damage > 0 && state->pending_garbage > 0) // Generated + Incoming
    {
        if (damage >= state->pending_garbage) // Generated > Incoming
        {
            damage -= state->pending_garbage; // Reduce generated
            state->pending_garbage = 0;       // Reset Incoming
        }
        else
        {
            state->pending_garbage -= damage; // Incoming > Generated
            damage = 0;                       // Reset Generated
        }
    }
    // Send remaining generated damage to opponent
    state->outgoing_garbage = damage;
    // Taking damage
    if (cleared == 0 && state->pending_garbage > 0) // No generated to cancel out incoming
    {
        addGarbage(state, state->pending_garbage); // Push to bottom of the board
        state->pending_garbage = 0;                // Reset buffer
    }

    // Spawn a new piece at the top AFTER garbage has been settled
    spawnNewPiece(state);

    // Game Over Check
    if (!isValidPos(state, state->current.type, state->current.rot, state->current.x, state->current.y))
    {
        // Block Out
        state->game_over = true;
    }
    // Return lines cleared this turn
    return cleared;
}