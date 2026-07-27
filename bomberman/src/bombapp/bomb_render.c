#include <raylib.h>
#include <raymath.h>
#include "bomb_render.h"
#include "lib/libbombbrain/bomb_control.h"
#include "config.h"
#include "tiles.h"

Texture2D bomb_texture;
BombsQueue bombs_queue;

void bombs_init() {
    // Load all textures
    bomb_texture = LoadTexture(CONFIG.ASSETS.TILE_BOMB);
    init_bombs();
}

void bombs_tick() {
    tick_bombs(GetFrameTime());
}

void bombs_draw() {
    // Draw all bombs
    for (int i = bombs_queue.front + 1; i < bombs_queue.rear; i++) {
        Bomb *bomb = &bombs_queue.items[i];
        draw_static_sprite(
            bomb_texture, 
            bomb->position,
            CONFIG.PHYSICS.PICKUP_SIZE, 
            WHITE
        );
    }
}

void bombs_cleanup() {
    UnloadTexture(bomb_texture);
}