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

// Populated from server's PACKET_ROSTER broadcast
extern Roster lobby;

// Process all pending terminal inputs
void processInputs(GameState *state)
{
    // Read user inputs
    while (kbhit())
    {
        // Call getchar() wrapper
        int key = getchar();

        // Player keybinds
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
            cycleManualTarget(state, &lobby);
        }
        else if (key == ' ') // Spacebar (hard drop)
        {
            hardDrop(state);
            // Calculate Garbage and send
            tickGame(state);
            if (state->outgoing_garbage > 0)
            {
                // Find target player
                uint32_t target_victim = resolveTargetID(state, &lobby);
                if (target_victim != 0 && target_victim != state->player_id) // Only send when target victim is known
                {
                    // Pack the payload
                    AttackPayload payload =
                        {
                            .source_player = state->player_id,
                            .target_player = target_victim,
                            .lines = state->outgoing_garbage};
                    // Trigger Event Bus
                    event_bus_trigger(EVENT_ATTACK_GENERATED, &payload);
                }
                state->outgoing_garbage = 0; // Reset after sending
            }
            // Reset env variables
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