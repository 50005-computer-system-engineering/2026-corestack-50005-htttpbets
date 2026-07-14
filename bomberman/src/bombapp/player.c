#include <raylib.h>
#include <raymath.h>
#include "bombapp/config.h"
#include "bombapp/player.h"
#include "bombapp/spritesheet.h"
#include "bombapp/config.h"
#include "lib/libeventbus.h"
#include "events.h"

int direction = 0; // 0 = Up, 1 = Down, 2 = Left, 3 = Right
Vector2 player_position;

Spritesheet* curr_sprite;
Spritesheet stand_sprites[4]; // Follows direction index
Spritesheet walk_sprites[4];

void on_move_performing(void *args) {
    MoveEventArgs *a = (MoveEventArgs *)args;

    // Normalise direction, this fixes diagonal movement being twice as fast
    Vector2 moveVec = (Vector2){a->x, a->y};
    moveVec = Vector2Normalize(moveVec);

    // Update position
    // Note that up is negative, hence -=
    player_position.x += moveVec.x * CONFIG.PHYSICS.PLAYER_SPEED * GetFrameTime();
    player_position.y -= moveVec.y * CONFIG.PHYSICS.PLAYER_SPEED * GetFrameTime();

    // Update direction
    if (a->y > 0) // Up (highest prio)
        direction = 0;
    else if (a->y < 0) // Down
        direction = 1;
    else if (a->x < 0) // Left
        direction = 2;
    else if (a->x > 0) // Right
        direction = 3;

    // Update spritesheet
    curr_sprite = &walk_sprites[direction];
}

void on_move_released(void *args) {
    (void)args; // Unused

    // Update spritesheet, retaining old direction
    curr_sprite = &stand_sprites[direction];
}

void player_init() {
    // Init Spritesheets
    for (int i = 0; i < 4; i++) {
        spritesheet_init(&stand_sprites[i], CONFIG.ASSETS.PLAYER_STAND[i]);
        spritesheet_init(&walk_sprites[i], CONFIG.ASSETS.PLAYER_WALK[i]);
    }

    // Initial sprite is facing down
    curr_sprite = &stand_sprites[1];
    direction = 1;

    // Listen for input
    event_bus_listen(EVENT_INPUT_MOVE_PERFORMING, on_move_performing);
    event_bus_listen(EVENT_INPUT_MOVE_RELEASED, on_move_released);
}

void player_update() {
    spritesheet_update(curr_sprite);
}

void player_draw() {
    spritesheet_draw(curr_sprite, player_position, 0, WHITE);
}

void player_cleanup() {
    for (int i = 0; i < 4; i++) {
        spritesheet_free(&stand_sprites[i]);
        spritesheet_free(&walk_sprites[i]);
    }
}