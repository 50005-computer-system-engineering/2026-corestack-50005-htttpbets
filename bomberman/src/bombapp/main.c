#include "raylib.h"

#include "lib/libeventbus.h"
#include "camera.h"
#include "events.h"
#include "input.h"
#include "player.h"
#include "audio.h"
#include "tiles.h"
#include "bomberman.h"
#include "bomb_render.h"

// TODO: Dynamically spawn and assign me!
Bomberman player_bm;

void initalise()
{
    // Initialise event bus
    event_bus_init(EVENT_COUNT);

    // Init map
    tiles_init();

    // Initialise player
    player_bm = bomberman_create_default((Vector2){1, 1});
    player_init(&player_bm);

    // Init Camera after player
    camera_init(player_bm.box.position);

    // Init Bombs
    bombs_init();

    // Initialise audio
    audio_init();

    // Start playing battle music
    play_bgm(BGM_BATTLE);
}

void update_loop()
{
    // Update audio
    audio_update();

    // Update bombs
    bombs_tick();

    // Update input
    input_update();

    // Update player state
    bomberman_update(&player_bm);

    // Update camera after player movement
    camera_update(player_bm.box.position);
}

void draw_loop()
{
    BeginDrawing();
    ClearBackground(SKYBLUE);

    // World-Space Renders
    BeginMode2D(camera);
    tiles_draw();
    bombs_draw();
    bomberman_draw(&player_bm, WHITE);
    EndMode2D();

    // Static UI Renders
    DrawText(TextFormat("Bombs: %i/%i | Fire: %i", player_bm.inventory.remaining_bombs, player_bm.inventory.num_bombs, player_bm.inventory.num_fires), 40, 40, 40, BLACK);
    EndDrawing();
}

void cleanup()
{
    // Free event bus
    event_bus_free();

    // Free map
    tiles_cleanup();

    // Free bombs
    bombs_cleanup();

    // Free player
    bomberman_cleanup(&player_bm);

    // Free bgm & sfx
    audio_cleanup();
}

int main()
{
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
