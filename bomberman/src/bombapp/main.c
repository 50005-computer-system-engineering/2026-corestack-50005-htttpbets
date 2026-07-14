#include <raylib.h>

#include "lib/libeventbus.h"
#include "events.h"
#include "bombapp/input.h"

void initalise() {
    // Initialise event bus
    event_bus_init(EVENT_COUNT);
}

void update_loop() {
    // Update input
    input_update();

    // Update player state
}

void draw_loop() {
    BeginDrawing();

    ClearBackground(DARKGREEN);
    DrawText("Hello, World!", 190, 200, 20, BLACK);
    
    EndDrawing();
}

void cleanup() {
    // Free event bus
    event_bus_free();
}


int main() {
    // (1) Init App
    // TODO: Uncomment during final release for fullscreen mode
    // SetConfigFlags(FLAG_FULLSCREEN_MODE);
    // InitWindow(0, 0, "Bombs Away! \U0001F4A3"); // bomb emoji
    // SetExitKey(KEY_NULL); // Prevent exit via escape key
    
    // Windowed mode for debugging purposes
    InitWindow(600, 400, "Bombs Away! \U0001F4A3");
    
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