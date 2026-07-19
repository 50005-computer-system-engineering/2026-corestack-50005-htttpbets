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
void processInputs(GameState *lobby[], int total_players)
{
    GameState *p1 = lobby[0];
    GameState *p2 = lobby[1];

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
                        rotateCurrentPiece(p2);
                        p2->lock_timer = 0;
                        break;
                    case 'D': // Left arrow
                        if (moveLeft(p2))
                        {
                            p2->lock_timer = 0;
                        }
                        break;
                    case 'C': // Right arrow
                        if (moveRight(p2))
                        {
                            p2->lock_timer = 0;
                        }
                        break;
                    case 'B': // Down arrow (soft drop + lock delay)
                        if (softDrop(p2))
                        {
                            p2->lock_timer = 0;
                        }
                        break;
                        break;
                    }
                }
            }
        }
        else if (key == 'x' || key == 'X') // Rotate clockwise (alternate key)
        {
            rotateCurrentPiece(p2);
            p2->lock_timer = 0;
        }
        else if (key == 'z' || key == 'Z') // Rotate counterclockwise
        {
            rotateCounterClockwise(p2);
            p2->lock_timer = 0;
        }
        else if (key == 't' || key == 'T') // Change target mode
        {
            cycleTargetMode(p2);
        }
        else if (key == 'r' || key == 'R') // Change target ID
        {
            // Manually swap target directly
            cycleManualTarget(p2, lobby, total_players);
        }
        else if (key == ' ') // Spacebar (hard drop)
        {
            hardDrop(p2);
            // Calculate Garbage and send
            tickGame(p2);
            if (p2->outgoing_garbage > 0)
            {
                // Find target
                int current_victim = resolveTargetID(p2, lobby, total_players);
                // Pack the payload
                AttackPayload payload =
                    {
                        .source_player = 2,
                        .target_player = current_victim,
                        .lines = p2->outgoing_garbage};
                // Trigger Event Bus
                event_bus_trigger(EVENT_ATTACK_GENERATED, &payload);
                p2->outgoing_garbage = 0; // Reset after sending
            }
            p2->gravity_timer = 0;
            p2->lock_timer = 0;
        }
        else if (key == 'h' || key == 'H') // H to hold
        {
            if (!p2->has_held)
            {
                holdPiece(p2);
                p2->gravity_timer = 0;
            }
        }

        // --- PLAYER 1 (WASD) Controls ---
        else if (key == 'w' || key == 'W') // P1 Rotate CW
        {
            rotateCurrentPiece(p1);
            p1->lock_timer = 0;
        }
        else if (key == 'a' || key == 'A') // P1 Left
        {
            if (moveLeft(p1))
            {
                p1->lock_timer = 0;
            }
        }
        else if (key == 'd' || key == 'D') // P1 Right
        {
            if (moveRight(p1))
            {
                p1->lock_timer = 0;
            }
        }
        else if (key == 's' || key == 'S') // P1 Down (Soft Drop)
        {
            if (softDrop(p1))
            {
                p1->lock_timer = 0;
            }
        }
        else if (key == 'v' || key == 'V') // Change target mode
        {
            cycleTargetMode(p1);
        }
        else if (key == 'b' || key == 'B') // Change target ID
        {
            // Manually swap target directly
            cycleManualTarget(p1, lobby, total_players);
        }
        else if (key == 'g' || key == 'G') // P1 Hard Drop
        {
            hardDrop(p1);
            // Calculate Garbage and send
            tickGame(p1);
            if (p1->outgoing_garbage > 0)
            {
                // Find target
                int current_victim = resolveTargetID(p1, lobby, total_players);
                // Pack the payload
                AttackPayload payload =
                    {
                        .source_player = 1,
                        .target_player = current_victim,
                        .lines = p1->outgoing_garbage};
                // Trigger Event Bus
                event_bus_trigger(EVENT_ATTACK_GENERATED, &payload);
                p1->outgoing_garbage = 0; // Reset after sending
            }
            p1->gravity_timer = 0;
            p1->lock_timer = 0;
        }
        else if (key == 'f' || key == 'F') // P1 Hold
        {
            if (!p1->has_held)
            {
                holdPiece(p1);
                p1->lock_timer = 0;
            }
        }
        else if (key == 'q' || key == 'Q') // Q to quit
        {
            // Set game over states for both P1 and P2
            p1->game_over = true;
            p2->game_over = true;
            break; // Exit
        }
    }
}