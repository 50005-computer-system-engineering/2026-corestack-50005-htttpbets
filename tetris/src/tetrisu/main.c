#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
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
#include "lib/libtetrisbrain/killfeed.h"
#include "lib/libbattleroyale/client.h"

// Networking purposes
#define LOCAL_HOST "127.0.0.1"

// Global network client
BRClient *network_client = NULL;

// Populate lobby from server's PACKET_ROSTER broadcast at game start
Roster lobby = {0};

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
                uint32_t target_victim = resolveTargetID(state, &lobby);
                if (target_victim != 0 && target_victim != state->player_id)
                {
                    // Pack the payload dynamically
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
int main(void)
{
    // PATCH FIX FOR FLICKERING TERMINAL
    setvbuf(stdout, NULL, _IOFBF, 16384);

    // Clear terminal screen
    printf("\e[1;1H\e[2J");
    fflush(stdout);

    // Network Client Initialization
    if (brclient_init(&network_client) < 0) // Failed
    {
        printf("[tetrisu] Failed to create network client.\n");
        network_client = NULL;
    }
    else if (brclient_join(network_client, LOCAL_HOST) < 0)
    {
        printf("[tetrisu] Failed to join lobby.\n");
        network_client = NULL;
    }
    else
    {
        printf("[tetrisu] Connected to lobby successfully!");
    }

    // Allow time for server to reach LOBBY_SIZE and enter GAME state
    printf("[tetrisu] Waiting for lobby to fill and game to start...\n");
    sleep(5);                            // Brief delay to read connection status
    printf("[tetrisu] Game started!\n"); // Game start flag

    // Clear terminal screen
    // Set up the terminal for the game
    enableRawMode();
    printf("\e[1;1H\e[2J");
    fflush(stdout);

    startGame(&gamestate_player);
    if (brclient_get_id(network_client, &gamestate_player.player_id) < 0) // Replaced hardcoded initialization with server-assigned ID retrieval
    {
        printf("[tetrisu] Failed to retrieve player ID from server.\n");
        gamestate_player.game_over = true; // Bail out cleanly rather than run with an uninitialized ID
    }
    gamestate_player.target_player_id = 0; // Set as unknown until populated afterwards
    gamestate_player.last_attacker_id = 0; // Set as unknown until populated afterwards
    gamestate_player.held_type = 0;        // Initialize hold box
    gamestate_player.has_held = false;     // Clear flag for hold box

    // Event Bus setup
    event_bus_init(EVENT_COUNT);
    event_bus_listen(EVENT_ATTACK_GENERATED, on_attack_generated);

    // --- THE GAME LOOP ---
    while (!gamestate_player.game_over)
    {
        // Deal with active inputs
        processInputs(&gamestate_player);

        // Read directly from message queue for incoming attacks
        unsigned char net_buffer[512] = {0};

        if (brclient_get_app_msg(net_buffer) == 1)
        {
            // Attach 4-byte tag to identify action
            uint32_t tag;
            memcpy(&tag, net_buffer, sizeof(tag));
            tag = ntohl(tag);

            if (tag == PACKET_ROSTER) // Server to populate who is in the lobby
            {
                RosterPayload incoming_roster;
                memcpy(&incoming_roster, net_buffer + sizeof(tag), sizeof(RosterPayload));

                lobby.count = (int)ntohl(incoming_roster.count);
                if (lobby.count > MAX_LOBBY_PLAYERS)
                {
                    lobby.count = MAX_LOBBY_PLAYERS; // Failsafe (technically shouldn't happen)
                }
                for (int i = 0; i < lobby.count; i++)
                {
                    lobby.ids[i] = ntohl(incoming_roster.ids[i]);
                    lobby.eliminated[i] = false; // Nobody is out yet at game start
                }

                // If no locked target, help to pick
                if (gamestate_player.target_player_id == 0)
                {
                    for (int i = 0; i < lobby.count; i++)
                    {
                        if (lobby.ids[i] != gamestate_player.player_id)
                        {
                            gamestate_player.target_player_id = lobby.ids[i];
                            break;
                        }
                    }
                }
            }
            else if (tag == PACKET_ATTACK) // Receiving garbage from opponent
            {
                // Cast the raw byte buffer back into our struct
                AttackPayload incoming;
                memcpy(&incoming, net_buffer + sizeof(tag), sizeof(AttackPayload));

                // Convert the network bytes back to readable integers
                uint32_t source_id = ntohl(incoming.source_player);
                uint32_t target_id = ntohl(incoming.target_player);
                uint32_t lines = ntohl(incoming.lines);

                // Apply incoming garbage
                if (target_id == gamestate_player.player_id && lines > 0)
                {
                    gamestate_player.pending_garbage += lines;
                    gamestate_player.last_attacker_id = source_id;
                    addKillFeed(source_id, target_id, lines);
                }
            }
        }

        // Refresh internal variables
        updateGameTimers(&gamestate_player);

        // Render the boards
        drawBoard(&gamestate_player);

        // Delay frames to be visible to the human eye
        usleep(DELAY_MICROSECONDS);
    }

    // Clean up after game ends
    drawBoard(&gamestate_player);
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