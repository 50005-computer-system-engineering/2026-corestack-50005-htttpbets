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
#include "lib/libtetrisprotocol/protocol.h"
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
    if (brclient_get_id(network_client, &gamestate_player.player_id) < 0) // Server-assigned ID retrieval
    {
        printf("[tetrisu] Failed to retrieve player ID from server.\n");
        gamestate_player.game_over = true; // Bail out cleanly rather than run with an uninitialized ID
    }

    // Event Bus setup -> drives the local kill feed
    event_bus_init(EVENT_COUNT);
    event_bus_listen(EVENT_ATTACK_GENERATED, on_attack_generated);

    // --- THE GAME LOOP ---
    while (!gamestate_player.game_over)
    {
        // Deal with active inputs
        processInputs(&gamestate_player);

        // Read directly from message queue
        unsigned char net_buffer[512] = {0};

        if (brclient_get_app_msg(net_buffer) == 1)
        {
            uint32_t tag = readPacketTag(net_buffer);

            if (tag == PACKET_STATE) // Authoritative board pushed by the server
            {
                StatePayload incoming;
                unpackState(net_buffer, &incoming);

                if (incoming.player_id == gamestate_player.player_id)
                {
                    applyStatePayload(&incoming, &gamestate_player);
                }
            }
            else if (tag == PACKET_ROSTER) // Server to populate who is in the lobby
            {
                RosterPayload incoming_roster;
                unpackRoster(net_buffer, &incoming_roster);

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
            }
            else if (tag == PACKET_ATTACK) // Notification only, damage already applied server-side
            {
                // Cast the raw byte buffer back into our struct
                AttackPayload incoming;
                unpackAttack(net_buffer, &incoming);

                // Kill feed is driven through the event bus
                event_bus_trigger(EVENT_ATTACK_GENERATED, &incoming);
            }
        }

        // Render whatever the server last told us
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