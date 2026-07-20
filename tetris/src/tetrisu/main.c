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
static void updateGameTimers(GameState *player, GameState *lobby[], int total_players)
{
    // Track current gravity of piece for lock delay
    int current_gravity = GRAVITY_THRESHOLD_START - ((player->level - 1) * 5);
    if (current_gravity < 5)
    {
        current_gravity = 5;
    }

    // Gravity + Lock Delay
    bool is_resting = !isValidPos(player, player->current.type, player->current.rot, player->current.x, player->current.y + 1);
    if (is_resting)
    {
        // Lock Timer
        player->lock_timer++;
        if (player->lock_timer >= LOCK_THRESHOLD_START)
        {
            // Calculate Garbage and send
            tickGame(player);
            if (player->outgoing_garbage > 0)
            {
                // Find target
                int current_victim = resolveTargetID(player, lobby, total_players);
                // Pack the payload
                AttackPayload payload =
                    {
                        .source_player = player->player_id,
                        .target_player = current_victim,
                        .lines = player->outgoing_garbage};
                // Trigger Event Bus
                event_bus_trigger(EVENT_ATTACK_GENERATED, &payload);
                player->outgoing_garbage = 0; // Reset after sending
            }

            // Reset env variables
            player->lock_timer = 0;
            player->gravity_timer = 0;
        }
    }
    else
    {
        // Gravity Timer
        player->lock_timer = 0;
        player->gravity_timer++;
        if (player->gravity_timer >= current_gravity)
        {
            tickGame(player);
            player->gravity_timer = 0;
        }
    }
}

// --- MAIN GAME LOOP ---
int main()
{
    // Clear terminal screen
    printf("\e[1;1H\e[2J");
    fflush(stdout);
    // Network Initialization (stopgap version)
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

    // TODO: REPLACE WITH DYNAMIC LOBBY
    int total_players = 2;
    GameState *lobby[2] = {&gamestate_p1, &gamestate_p2};

    startGame(&gamestate_p1);
    gamestate_p1.player_id = 1;        // Assign player ID
    gamestate_p1.target_player_id = 2; // Fixed player target
    gamestate_p1.held_type = 0;        // Initialize hold box
    gamestate_p1.has_held = false;

    startGame(&gamestate_p2);
    gamestate_p2.player_id = 2;        // Assign player ID
    gamestate_p2.target_player_id = 1; // Fixed player target
    gamestate_p2.held_type = 0;        // Initialize hold box
    gamestate_p2.has_held = false;

    // Event Bus setup
    event_bus_init(EVENT_COUNT);
    event_bus_listen(EVENT_ATTACK_GENERATED, on_attack_generated);

    // --- THE GAME LOOP ---
    while (!gamestate_p1.game_over && !gamestate_p2.game_over)
    {
        // Deal with active inputs
        processInputs(lobby, total_players);

        // Refresh internal variables
        updateGameTimers(lobby[0], lobby, total_players);
        updateGameTimers(lobby[1], lobby, total_players);

        // Render the boards
        drawBothBoards(&gamestate_p1, &gamestate_p2);

        // Delay frames to be visible to the human eye
        usleep(DELAY_MICROSECONDS);
    }

    // Clean up after game ends
    drawBothBoards(&gamestate_p1, &gamestate_p2);
    printf("\n\n");
    printf("<!> ====================== <!>\n");

    if (gamestate_p1.game_over && !gamestate_p2.game_over)
    {
        printf("<!>    PLAYER 2 WINS!      <!>\n");
    }
    else if (gamestate_p2.game_over && !gamestate_p1.game_over)
    {
        printf("<!>    PLAYER 1 WINS!      <!>\n");
    }
    else
    {
        printf("<!>         DRAW!          <!>\n");
    }
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