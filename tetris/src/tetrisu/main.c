#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
#include "lib/libeventbus.h"
#include "config.h"
#include "events.h"
#include "input.h"
#include "renderer.h"
#include "input_handler.h"
#include "lib/libtetrisbrain/engine.h"
#include "lib/libtetrisbrain/board_control.h"
#include "lib/libtetrisbrain/hold.h"
#include "lib/libtetrisbrain/state.h"
#include "lib/libtetrisbrain/targeting.h"
#include "lib/libhtttp/client.h"

// Networking purposes
#define LOCAL_HOST "127.0.0.1"

// Global network client
LibhtttpClient *network_client = NULL;

// Process gravity and lock delay intervals
static void updateGameTimers(GameState *state)
{
    // Track current gravity of piece for lock delay
    int current_gravity = GRAVITY_THRESHOLD_START - ((state->level - 1) * 5);
    if (current_gravity < 5)
    {
        current_gravity = 5;
    }

    // Gravity + Lock Delay
    bool is_resting = !isValidPos(state, state->current.type, state->current.rot, state->current.x, state->current.y + 1);
    if (is_resting)
    {
        // Lock Timer
        state->lock_timer++;
        if (state->lock_timer >= LOCK_THRESHOLD_START)
        {
            // Calculate Garbage and send
            tickGame(state);
            if (state->outgoing_garbage > 0)
            {
                // Find target player
                // TODO: MANUAL PATCH FIX, NEED TO CHANGE !!
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
            // Reset env variables
            state->lock_timer = 0;
            state->gravity_timer = 0;
        }
    }
    else
    {
        // Gravity Timer
        state->lock_timer = 0;
        state->gravity_timer++;
        if (state->gravity_timer >= current_gravity)
        {
            tickGame(state);
            state->gravity_timer = 0;
        }
    }
}

// --- MAIN GAME LOOP ---
int main()
{
    // Clear terminal screen
    printf("\e[1;1H\e[2J");
    fflush(stdout);

    // Network Initialization
    if (createClient(&network_client) < 0) // Failed
    {
        printf("[tetrisu] Failed to create network client.\n");
        network_client = NULL;
    }
    else if (joinLobby(network_client, LOCAL_HOST) < 0)
    {
        printf("[tetrisu] Failed to join lobby.\n");
        network_client = NULL;
    }
    else
    {
        printf("[tetrisu] Connected to lobby successfully!\n");
    }
    sleep(5); // Brief delay to read connection status

    // Clear terminal screen
    // Set up the terminal for the game
    enableRawMode();
    printf("\e[1;1H\e[2J");
    fflush(stdout);

    startGame(&gamestate_p1);
    gamestate_p1.player_id = 1;        // Assign player ID
    gamestate_p1.target_player_id = 0; // Fixed player target (unknown until broadcast exists)
    gamestate_p1.held_type = 0;        // Initialize hold 0box
    gamestate_p1.has_held = false;

    // Event Bus setup
    event_bus_init(EVENT_COUNT);
    event_bus_listen(EVENT_ATTACK_GENERATED, on_attack_generated);

    // --- THE GAME LOOP ---
    while (!gamestate_p1.game_over)
    {
        // Deal with active inputs
        processInputs(&gamestate_p1);

        // Refresh internal variables
        updateGameTimers(&gamestate_p1);

        // Render the boards
        drawBoard(&gamestate_p1);

        // Delay frames to be visible to the human eye
        usleep(DELAY_MICROSECONDS);
    }

    // Clean up after game ends
    drawBoard(&gamestate_p1);
    printf("\n\n");
    printf("<!> ====================== <!>\n");
    printf("<!>       GAME OVER!       <!>\n");
    printf("<!> ====================== <!>\n");
    printf("\nPress any key to exit...\n");

    while (!kbhit())
    {
        usleep(DELAY_MICROSECONDS);
    }
    getchar();

    return 0;
}