#include <raylib.h>
#include <raymath.h>
#include "bomb_render.h"
#include "lib/libbombbrain/bomb_control.h"
#include "config.h"
#include "tiles.h"

Texture2D texture_bomb;
Texture2D texture_explode_core;
Texture2D texture_explode_mid;

void bombs_init() {
    // Load all textures
    texture_bomb = LoadTexture(CONFIG.ASSETS.TILE_BOMB);
    texture_explode_core = LoadTexture(CONFIG.ASSETS.TILE_EXPLODE_CORE);
    texture_explode_mid = LoadTexture(CONFIG.ASSETS.TILE_EXPLODE_MID);
    init_bombs();
}

void bombs_tick() {
    tick_bombs(GetFrameTime());
    tick_explosions(GetFrameTime());
}

void bombs_draw() {
    // Draw all bombs
    for (int i = bombs_queue.front + 1; i < bombs_queue.rear; i++) {
        BombInfo *bomb = &bombs_queue.items[i];
        draw_static_sprite(
            texture_bomb, 
            bomb->position,
            CONFIG.PHYSICS.PICKUP_SIZE, 
            0.0f,
            WHITE
        );
    }

    // Draw all explosions
    for (int i = explodes_queue.front + 1; i < explodes_queue.rear; i++) {
        ExplosionInfo *explosion = &explodes_queue.items[i];
        // Core
        draw_static_sprite(
            texture_explode_core, 
            explosion->center,
            CONFIG.PHYSICS.TILE_SIZE, 
            0.0f,
            WHITE
        );

        // Spread
        for (int dir = 0; dir < 4; dir++) {
            if (explosion->spread_amt[dir] == 0)
                continue;

            Vector2 spread_vec;
            float rotation = 0.0f;
            switch (dir) {
                case 0:  // Up
                    spread_vec = (Vector2){0, -1}; 
                    rotation = 90.0f;
                    break;
                case 1:  // Down
                    spread_vec = (Vector2){0, 1}; 
                    rotation = 270.0f;
                    break;
                case 2: // Left
                    spread_vec = (Vector2){-1, 0}; 
                    break;
                case 3: // Right
                    spread_vec = (Vector2){1, 0};
                    rotation = 180.0f;
                    break;
            }
            for (int spread = 0; spread < explosion->spread_amt[dir]; spread++) {
                draw_static_sprite(
                    texture_explode_mid, 
                    Vector2Add(explosion->center, spread_vec),
                    CONFIG.PHYSICS.TILE_SIZE, 
                    rotation,
                    WHITE
                );
            }
        }
    }
}

void bombs_cleanup() {
    UnloadTexture(texture_bomb);
}