#include <raylib.h>
#include <raymath.h>
#include "config.h"
#include "bomberman.h"
#include "spritesheet.h"
#include "config.h"
#include "lib/libbombbrain/inventory.h"

// Cached loaded spritesheets
Spritesheet stand_sprites[4]; // Follows direction index
Spritesheet walk_sprites[4];

void bomberman_init() {
    // Init Spritesheets
    for (int i = 0; i < 4; i++) {
        spritesheet_init(&stand_sprites[i], CONFIG.ASSETS.PLAYER_STAND[i]);
        spritesheet_init(&walk_sprites[i], CONFIG.ASSETS.PLAYER_WALK[i]);
    }
}

Bomberman bomberman_create_default(Vector2 position) {
    Bomberman bomberman = {0};

    // Initial sprite is facing down
    bomberman.curr_sprite = &stand_sprites[1];
    bomberman.direction = 1;
    bomberman.is_moving = false;

    // Spawn in center of tile, at position
    bomberman.box.position = Vector2Add(position , (Vector2){0.5f, 0.5f});
    bomberman.box.size = (Vector2){0.42f, 0.85f};

    // Powerups
    bomberman.inventory.num_bombs = 2;
    bomberman.inventory.remaining_bombs = bomberman.inventory.num_bombs;
    bomberman.inventory.num_fires = 1;
    bomberman.inventory.bomb_replenish_timer = 0;
    return bomberman;
}

void bomberman_update(Bomberman* bm) {
    bm->curr_sprite = bm->is_moving ? &walk_sprites[bm->direction] : &stand_sprites[bm->direction];
    spritesheet_update(bm->curr_sprite);
    inventory_update(&bm->inventory, GetFrameTime());
}

void bomberman_draw(Bomberman* bm) {
    // HITBOX
    //DrawRectangle(bm->box.position.x * CONFIG.PHYSICS.TILE_SIZE, bm->box.position.y * CONFIG.PHYSICS.TILE_SIZE, bm->box.size.x * CONFIG.PHYSICS.TILE_SIZE, bm->box.size.y * CONFIG.PHYSICS.TILE_SIZE, RED);
    spritesheet_draw(
        bm->curr_sprite, 
        bm->box.position,
        (Vector2) { 0.5f, 0.2f},
        0,  
        WHITE
    );
}

void bomberman_cleanup() {
    for (int i = 0; i < 4; i++) {
        spritesheet_free(&stand_sprites[i]);
        spritesheet_free(&walk_sprites[i]);
    }
}