#include "raylib.h"
#include <raymath.h>
#include "bomb_render.h"
#include "lib/libbombbrain/bomb_control.h"
#include "lib/libbombbrain/powerup_control.h"
#include "config.h"
#include "tiles.h"

Texture2D texture_bomb;
Texture2D texture_explode_core;
Texture2D texture_explode_mid;
Texture2D texture_explode_cap;

void bombs_init()
{
    // Load all textures
    texture_bomb = LoadTexture(CONFIG.ASSETS.TILE_BOMB);
    texture_explode_core = LoadTexture(CONFIG.ASSETS.TILE_EXPLODE_CORE);
    texture_explode_mid = LoadTexture(CONFIG.ASSETS.TILE_EXPLODE_MID);
    texture_explode_cap = LoadTexture(CONFIG.ASSETS.TILE_EXPLODE_CAP);
    init_bombs();
    powerup_init();
}

void bombs_tick()
{
    tick_bombs(GetFrameTime());
    tick_explosions(GetFrameTime());
}

void bombs_draw()
{
    // Draw all bombs
    for (int i = bombs_queue.front + 1; i < bombs_queue.rear; i++) {
        BombInfo* bomb = &bombs_queue.items[i];
        float red_tint = (bomb->timer / BOMB_TIMER) * 255.0f; // Progress from 255 to 0 over BOMB_TIMER
        draw_static_sprite(
            texture_bomb,
            bomb->position,
            CONFIG.PHYSICS.PICKUP_SIZE,
            0.0f,
            (Color){255, red_tint, red_tint, 255});
    }

    // Draw all explosions
    for (int i = explodes_queue.front + 1; i < explodes_queue.rear; i++) {
        ExplosionInfo* explosion = &explodes_queue.items[i];
        // Core
        draw_static_sprite(
            texture_explode_core,
            explosion->center,
            CONFIG.PHYSICS.TILE_SIZE,
            0.0f,
            WHITE);

        // Spread
        for (int dir = 0; dir < 4; dir++) {
            if (explosion->spread_amt[dir] == 0)
                continue;

            Vector2 spread_vec;
            float rotation = 0.0f;
            switch (dir) {
            case 0: // Up
                spread_vec = (Vector2){0, -1};
                rotation = 90.0f;
                break;
            case 1: // Down
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
                    spread == explosion->spread_amt[dir] - 1 ? texture_explode_cap : texture_explode_mid,
                    Vector2Add(explosion->center, Vector2Scale(spread_vec, spread + 1)),
                    CONFIG.PHYSICS.TILE_SIZE,
                    rotation,
                    WHITE);
            }
        }
    }
}

void bombs_cleanup()
{
    UnloadTexture(texture_bomb);
    powerup_free();
}
