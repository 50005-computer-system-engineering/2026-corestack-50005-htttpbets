#include <stdio.h>
#include "lib/libeventbus.h"
#include "input.h"
#include "events.h"
#include "config.h"
#include "input_handler.h"
#include "lib/libtetrisbrain/engine.h"
#include "lib/libtetrisbrain/hold.h"
#include "lib/libtetrisbrain/targeting.h"
#include "lib/libtetrisbrain/board_control.h"
#include "lib/libtetrisbrain/state.h"
#include "lib/libtetrisbrain/movement.h"

// Process all pending terminal inputs
void processInputs(GameState *state)
{
    // Read user inputs
    while (kbhit())
    {
        // Call getchar() wrapper
        int key = getchar();

        // Player 2 keybinds
        // Linux arrow keys send 3 characters instantly => escape (27), '[', and a letter
        if (key == 27) // Escape or arrow key?
        {
            if (kbhit() && getchar() == '[') // Bracket right after?
            {
                if (kbhit()) // Decide output based on letter
                {
                    switch (getchar())
                    {
                    case 'A': // Up arrow (rotate clockwise)
                        rotateCurrentPiece(state);
                        state->lock_timer = 0;
                        break;
                    case 'D': // Left arrow
                        if (moveLeft(state))
                        {
                            state->lock_timer = 0;
                        }
                        break;
                    case 'C': // Right arrow
                        if (moveRight(state))
                        {
                            state->lock_timer = 0;
                        }
                        break;
                    case 'B': // Down arrow (soft drop + lock delay)
                        if (softDrop(state))
                        {
                            state->lock_timer = 0;
                        }
                        break;
                    }
                }
            }
        }
        else if (key == 'x' || key == 'X') // Rotate clockwise (alternate key)
        {
            rotateCurrentPiece(state);
            state->lock_timer = 0;
        }
        else if (key == 'z' || key == 'Z') // Rotate counterclockwise
        {
            rotateCounterClockwise(state);
            state->lock_timer = 0;
        }
        else if (key == 't' || key == 'T') // Change target mode
        {
            cycleTargetMode(state);
        }
        else if (key == 'r' || key == 'R') // Change target ID
        {
            // Manually swap target directly
            // TODO: MANUAL PATCH FIX, NEED TO CHANGE !!
            GameState *localLobby[] = {state};
            cycleManualTarget(state, localLobby, 1);
        }
        else if (key == ' ') // Spacebar (hard drop)
        {
            hardDrop(state);
            // Calculate Garbage and send
            tickGame(state);
            if (state->outgoing_garbage > 0)
            {
                // TODO: MANUAL PATCH FIX, NEED TO CHANGE !!
                // Find target player
                GameState *localLobby[] = {state};
                uint32_t current_victim = resolveTargetID(state, localLobby, 1);
                // Pack the payload
                AttackPayload payload =
                    {
                        .source_player = state->player_id,
                        .target_player = current_victim,
                        .lines = state->outgoing_garbage};
                // Trigger Event Bus
                event_bus_trigger(EVENT_ATTACK_GENERATED, &payload);
                state->outgoing_garbage = 0; // Reset after sending
            }
            state->gravity_timer = 0;
            state->lock_timer = 0;
        }
        else if (key == 'h' || key == 'H') // H to hold
        {
            if (!state->has_held)
            {
                holdPiece(state);
                state->gravity_timer = 0;
            }
        }
        else if (key == 'q' || key == 'Q') // Q to quit
        {
            // Set game over state
            state->game_over = true;
            break; // Exit
        }
    }
}