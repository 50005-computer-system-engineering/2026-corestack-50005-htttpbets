#include "camera.h"
#include <raylib.h>

Camera2D camera = { 0 };

void camera_init(Vector2 targetPos) {
    camera = (Camera2D) { 
        .target = targetPos,
        .offset = (Vector2) {GetScreenWidth() * 0.5f, GetScreenHeight() * 0.5f},
        .rotation = 0.0f,
        .zoom = 1.0f
    };
}

void camera_update(Vector2 targetPos) {
    camera.target = targetPos;
}