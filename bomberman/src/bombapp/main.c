#include <raylib.h>

int main() {
    // (1) Init App
    SetConfigFlags(FLAG_FULLSCREEN_MODE);
    InitWindow(0, 0, "Bombs Away!");
    SetTargetFPS(60);

    // (2) Main Loop
    while (!WindowShouldClose()) {
        BeginDrawing();
        ClearBackground(SKYBLUE);
        DrawText("Hello, World!", 190, 200, 20, BLACK);
        EndDrawing();
    }

    // (3) Close App
    CloseWindow();
    return 0;
}