#include "camera.h"
#include "config.h"
#include "lib/libbombbrain/map.h"
#include <raylib.h>
#include <raymath.h>

// Number of tiles to offset from actual border of map, so we can see the outside a little
#define NUM_OUTSIDE_TILES 1

Camera2D camera = { 0 };
Vector2 screen_size;
Vector2 min_pos, max_pos;

void camera_init(Vector2 targetPos) {
    screen_size = (Vector2){GetScreenWidth(), GetScreenHeight()};
    camera = (Camera2D) { 
        .target = Vector2Scale(targetPos, CONFIG.PHYSICS.TILE_SIZE),
        .offset = Vector2Scale(screen_size, 0.5f),
        .rotation = 0.0f,
        .zoom = 1.0f
    };
    
    // Store minimum and maximum positions to render in map
    min_pos = Vector2Scale(Vector2One(), -NUM_OUTSIDE_TILES * CONFIG.PHYSICS.TILE_SIZE);
    max_pos = Vector2Scale(Vector2One(), (map_size + NUM_OUTSIDE_TILES) * CONFIG.PHYSICS.TILE_SIZE);
}

void camera_update(Vector2 targetPos) {
    camera.target = Vector2Scale(targetPos, CONFIG.PHYSICS.TILE_SIZE);
    camera.offset = Vector2Scale(screen_size, 0.5f);
    Vector2 map_min = GetWorldToScreen2D(min_pos, camera);
    Vector2 map_max = GetWorldToScreen2D(max_pos, camera);

    // Clamp offsets to map boundaries
    if (map_max.x < screen_size.x) camera.offset.x = screen_size.x - (map_max.x - (float)screen_size.x * 0.5f);
    else if (map_min.x > 0.0f) camera.offset.x = (float)screen_size.x * 0.5f - map_min.x;
    if (map_max.y < screen_size.y) camera.offset.y = screen_size.y - (map_max.y - (float)screen_size.y * 0.5f);
    else if (map_min.y > 0.0f) camera.offset.y = (float)screen_size.y * 0.5f - map_min.y;
}