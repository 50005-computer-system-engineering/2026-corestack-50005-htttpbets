#include <raylib.h>
#include <raymath.h>
#include "tiles.h"
#include "lib/libbombbrain/map.h"
#include "config.h"

Texture2D tile_textures[TILETYPE_COUNT];

void tiles_init() {
    // Load all tile textures
    tile_textures[EMPTY] = LoadTexture(CONFIG.ASSETS.TILE_EMPTY);
    tile_textures[BREAKABLE] = LoadTexture(CONFIG.ASSETS.TILE_BREAKABLE);
    tile_textures[WALL] = LoadTexture(CONFIG.ASSETS.TILE_WALL);
    tile_textures[POWERUP_FIRE] = LoadTexture(CONFIG.ASSETS.TILE_POWERUP_FIRE);
    tile_textures[POWERUP_BOMB] = LoadTexture(CONFIG.ASSETS.TILE_POWERUP_BOMB);

    // Generate map
    map_generate(10);
}

void draw_static_sprite(Texture2D texture, Vector2 pos, float size, float rotation, Color tint) {
    // Render tile
    Rectangle source = {
        .x = pos.x * texture.width,
        .y = pos.y * texture.height,
        .width = texture.width,
        .height = texture.height
    };

    Rectangle dest = {
        .x = pos.x * CONFIG.PHYSICS.TILE_SIZE + (CONFIG.PHYSICS.TILE_SIZE - size) * 0.5f,
        .y = pos.y * CONFIG.PHYSICS.TILE_SIZE + (CONFIG.PHYSICS.TILE_SIZE - size) * 0.5f,
        .width = size,
        .height = size
    };

    Vector2 origin = Vector2Zero();
    if (rotation != 0.0f) {
        origin = (Vector2){texture.width * 0.5f, texture.height * 0.5f};
        dest.x += texture.width * 0.5f;
        dest.y += texture.height * 0.5f;
    }

    DrawTexturePro(texture, source, dest, origin, rotation, tint);
}

void tiles_draw() {
    // Draw all tiles
    for (int i = 0; i < map_size; i++) {
        for (int j = 0; j < map_size; j++) {
            Texture2D texture = tile_textures[map[i][j]];
            if (map[i][j] < BREAKABLE) // Empty / Player / Bomb / -1 Tiles
                texture = tile_textures[EMPTY];

            draw_static_sprite(texture, (Vector2){i, j}, CONFIG.PHYSICS.TILE_SIZE, 0.0f, WHITE);
        }
    }
}

void tiles_cleanup() {
    // Free all textures
    for (int i = 0; i < TILETYPE_COUNT; i++)
        UnloadTexture(tile_textures[i]);

    // Free map
    map_free();
}