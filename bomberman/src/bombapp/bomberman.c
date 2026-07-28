#include <raylib.h>
#include <raymath.h>
#include "config.h"
#include "bomberman.h"
#include "spritesheet.h"
#include "lib/libbombbrain/inventory.h"


Bomberman bomberman_create_default(Vector2 position) {
    Bomberman bomberman = {0};

    // Init Spritesheets
    for (int i = 0; i < 4; i++) {
        spritesheet_init(&bomberman.stand_sprites[i], CONFIG.ASSETS.PLAYER_STAND[i]);
        spritesheet_init(&bomberman.walk_sprites[i], CONFIG.ASSETS.PLAYER_WALK[i]);
    }

    // Initial sprite is facing down
    bomberman.curr_sprite = &bomberman.stand_sprites[1];
    bomberman.direction = 1;
    bomberman.is_moving = false;

    // Spawn in center of tile, at position
    bomberman.box.position = Vector2Add(position , (Vector2){0.35f, 0.1f});
    bomberman.box.size = (Vector2){0.42f, 0.85f};

    // Powerups
    bomberman.inventory.num_bombs = 2;
    bomberman.inventory.remaining_bombs = bomberman.inventory.num_bombs;
    bomberman.inventory.num_fires = 1;
    bomberman.inventory.bomb_replenish_timer = 0;
    return bomberman;
}

void bomberman_update(Bomberman* bm) {
    bm->curr_sprite = bm->is_moving ? &bm->walk_sprites[bm->direction] : &bm->stand_sprites[bm->direction];
    spritesheet_update(bm->curr_sprite);
    inventory_update(&bm->inventory, GetFrameTime());
}

void bomberman_draw(Bomberman* bm, Color tint) {
    // HITBOX
    //DrawRectangle(bm->box.position.x * CONFIG.PHYSICS.TILE_SIZE, bm->box.position.y * CONFIG.PHYSICS.TILE_SIZE, bm->box.size.x * CONFIG.PHYSICS.TILE_SIZE, bm->box.size.y * CONFIG.PHYSICS.TILE_SIZE, RED);
    spritesheet_draw(
        bm->curr_sprite, 
        bm->box.position,
        (Vector2) { 0.5f, 0.2f},
        0,  
        tint
    );
}

void bomberman_cleanup(Bomberman* bm) {
    for (int i = 0; i < 4; i++) {
        spritesheet_free(&bm->stand_sprites[i]);
        spritesheet_free(&bm->walk_sprites[i]);
    }
}