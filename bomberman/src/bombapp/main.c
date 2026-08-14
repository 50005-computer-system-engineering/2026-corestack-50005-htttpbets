#include "raylib.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include "lib/libeventbus.h"
#include "camera.h"
#include "events.h"
#include "input.h"
#include "player.h"
#include "audio.h"
#include "tiles.h"
#include "bomberman.h"
#include "bomb_render.h"
#include "network.h"

// Standalone (no server IP given) keeps the original single-player demo
// exactly as before. Networked mode is driven entirely by bombd's authoritative
// state; the local input handlers in player.c are never wired up for it
static bool networked = false;
static char server_ip[64] = {0};

// Standalone-only: the one locally-simulated player
Bomberman player_bm;

void initalise()
{
    event_bus_init(EVENT_COUNT);

    // Loads tile textures and a throwaway local map; networked mode replaces
    // the map as soon as bombd's PACKET_MAP_INIT arrives
    tiles_init();
    bombs_init();
    audio_init();

    if (networked) {
        if (!network_connect(server_ip) || !network_wait_for_map()) {
            printf("[bombapp] Could not join a networked match, exiting.\n");
            CloseWindow();
            exit(1);
        }

        int slot = network_local_slot();
        Vector2 start_pos = (slot >= 0) ? network_get_player(slot)->box.position : (Vector2){1, 1};
        camera_init(start_pos);
    } else {
        player_bm = bomberman_create_default((Vector2){1, 1});
        player_init(&player_bm);
        camera_init(player_bm.box.position);
    }

    play_bgm(BGM_BATTLE);
}

void update_loop()
{
    audio_update();

    if (networked) {
        network_poll();
        network_tick_explosions(GetFrameTime());

        if (!network_game_over()) {
            network_send_input();
        }

        for (int i = 0; i < network_player_count(); i++) {
            bomberman_update(network_get_player(i));
        }

        int slot = network_local_slot();
        if (slot >= 0) {
            camera_update(network_get_player(slot)->box.position);
        }
    } else {
        bombs_tick();
        input_update();
        bomberman_update(&player_bm);
        camera_update(player_bm.box.position);
    }
}

void draw_loop()
{
    BeginDrawing();
    ClearBackground(SKYBLUE);

    BeginMode2D(camera);
    tiles_draw();
    bombs_draw();

    if (networked) {
        int local_slot = network_local_slot();
        for (int i = 0; i < network_player_count(); i++) {
            if (!network_is_player_alive(i)) {
                continue; // Caught in an explosion: sprite disappears
            }
            Color tint = (i == local_slot) ? WHITE : (Color){190, 210, 255, 255};
            bomberman_draw(network_get_player(i), tint);
        }
    } else {
        bomberman_draw(&player_bm, WHITE);
    }
    EndMode2D();

    // Static UI Renders
    if (networked) {
        int slot = network_local_slot();
        if (slot >= 0) {
            Bomberman* local_bm = network_get_player(slot);
            DrawText(TextFormat("Bombs: %i/%i | Fire: %i", local_bm->inventory.remaining_bombs, local_bm->inventory.num_bombs, local_bm->inventory.num_fires), 40, 40, 40, BLACK);
        }
        if (network_game_over()) {
            uint32_t winner = network_winner_id();
            const char* message = (winner == 0) ? "NO SURVIVORS"
                : (winner == network_local_player_id())     ? "YOU WIN!"
                                                              : TextFormat("P%u WINS!", winner);
            int width = MeasureText(message, 80);
            DrawText(message, GetScreenWidth() / 2 - width / 2, GetScreenHeight() / 2 - 40, 80, RED);
        } else if (slot >= 0 && !network_is_player_alive(slot)) {
            // Match is still going (winner not yet decided): let the local
            // player know they're out, without interrupting everyone else
            const char* message = "You died!";
            int width = MeasureText(message, 80);
            DrawText(message, GetScreenWidth() / 2 - width / 2, GetScreenHeight() / 2 - 40, 80, RED);
        }
    } else {
        DrawText(TextFormat("Bombs: %i/%i | Fire: %i", player_bm.inventory.remaining_bombs, player_bm.inventory.num_bombs, player_bm.inventory.num_fires), 40, 40, 40, BLACK);
    }
    EndDrawing();
}

void cleanup()
{
    event_bus_free();
    tiles_cleanup();
    bombs_cleanup();

    if (networked) {
        for (int i = 0; i < network_player_count(); i++) {
            bomberman_cleanup(network_get_player(i));
        }
    } else {
        bomberman_cleanup(&player_bm);
    }

    audio_cleanup();
}

int main(int argc, char* argv[])
{
    if (argc >= 2) {
        networked = true;
        strncpy(server_ip, argv[1], sizeof(server_ip) - 1);
    }

    // (1) Init App
    // Force the program to look in the directory where the executable is running
    ChangeDirectory(GetApplicationDirectory());

    // TODO: Uncomment during final release for fullscreen mode
    SetConfigFlags(FLAG_FULLSCREEN_MODE);
    InitWindow(0, 0, "Bombs Away! \U0001F4A3"); // bomb emoji
    SetExitKey(KEY_NULL);                       // Prevent exit via escape key

    // Windowed mode for debugging purposes
    // InitWindow(1200, 675, "Bombs Away! \U0001F4A3");
    SetTargetFPS(60);

    initalise();

    // (2) Main Loop
    while (!WindowShouldClose()) {
        update_loop();
        draw_loop();
    }

    // (3) Close App
    cleanup();
    CloseWindow();
    return 0;
}
