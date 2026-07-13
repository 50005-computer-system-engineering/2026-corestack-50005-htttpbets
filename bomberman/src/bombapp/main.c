#include <raylib.h>

int main() {
    // (1) Init App
    InitWindow(800, 450, "Bomberman");
    SetTargetFPS(60);

    // (2) Main Loop
    while (!WindowShouldClose()) {
        BeginDrawing();
        ClearBackground(SKYBLUE);
        DrawText("Hello, World!", 190, 200, 20, LIGHTGRAY);
        EndDrawing();
    }

    // (3) Close App
    CloseWindow();
    return 0;
}