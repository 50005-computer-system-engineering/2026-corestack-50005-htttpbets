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
    tile_textures[BOMB] = LoadTexture(CONFIG.ASSETS.TILE_BOMB);
    tile_textures[POWERUP_FIRE] = LoadTexture(CONFIG.ASSETS.TILE_POWERUP_FIRE);
    tile_textures[POWERUP_BOMB] = LoadTexture(CONFIG.ASSETS.TILE_POWERUP_BOMB);

    // Generate map
    map_generate(10);
}

void tiles_draw() {
    // Draw all tiles
    for (int i = 0; i < map_size; i++) {
        for (int j = 0; j < map_size; j++) {
            Texture2D texture = tile_textures[map[i][j]];
            if (map[i][j] < BREAKABLE) // Empty / Player / -1 Tiles
                texture = tile_textures[EMPTY];

            // Render tile
            Rectangle source = {
                .x = i * texture.width,
                .y = j * texture.height,
                .width = texture.width,
                .height = texture.height
            };

            Rectangle dest = {
                .x = i * CONFIG.PHYSICS.TILE_SIZE,
                .y = j * CONFIG.PHYSICS.TILE_SIZE,
                .width = CONFIG.PHYSICS.TILE_SIZE,
                .height = CONFIG.PHYSICS.TILE_SIZE
            };

            DrawTexturePro(texture, source, dest, Vector2Zero(), 0.0f, WHITE);
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