#include <stdio.h>
#include <string.h>
#include "input.h"
#include "events.h"
#include "config.h"
#include "input_handler.h"
#include "lib/libtetrisprotocol/protocol.h"
#include "lib/libtetrisbrain/state.h"
#include "lib/libbattleroyale/client.h"

// Instantiate network client
extern BRClient *network_client;

// Ships one requested action to the server
static void sendAction(GameState *state, PlayerAction action)
{
    if (network_client == NULL)
    {
        return; // No connection, nothing to ask
    }

    InputPayload payload =
        {
            .player_id = state->player_id,
            .action = (uint32_t)action};

    unsigned char buffer[512] = {0};
    packInput(buffer, &payload);

    brclient_send_msg(network_client, buffer);
}

// Process all pending terminal inputs
// Every branch is now a pure keybind -> action mapping
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
                        sendAction(state, ACTION_ROTATE_CW);
                        break;
                    case 'D': // Left arrow
                        sendAction(state, ACTION_MOVE_LEFT);
                        break;
                    case 'C': // Right arrow
                        sendAction(state, ACTION_MOVE_RIGHT);
                        break;
                    case 'B': // Down arrow (soft drop + lock delay)
                        sendAction(state, ACTION_SOFT_DROP);
                        break;
                    }
                }
            }
        }
        else if (key == 'x' || key == 'X') // Rotate clockwise (alternate key)
        {
            sendAction(state, ACTION_ROTATE_CW);
        }
        else if (key == 'z' || key == 'Z') // Rotate counterclockwise
        {
            sendAction(state, ACTION_ROTATE_CCW);
        }
        else if (key == 't' || key == 'T') // Change target mode
        {
            sendAction(state, ACTION_CYCLE_TARGET_MODE);
        }
        else if (key == 'r' || key == 'R') // Change target ID
        {
            sendAction(state, ACTION_CYCLE_TARGET);
        }
        else if (key == ' ') // Spacebar (hard drop)
        {
            sendAction(state, ACTION_HARD_DROP);
        }
        else if (key == 'h' || key == 'H') // H to hold
        {
            sendAction(state, ACTION_HOLD);
        }
        else if (key == 'q' || key == 'Q') // Q to quit
        {
            // Tell the server we are leaving so it frees our slot and stops anyone from targeting us, then close down locally
            sendAction(state, ACTION_QUIT);
            state->game_over = true;
            break; // Exit
        }
    }
}