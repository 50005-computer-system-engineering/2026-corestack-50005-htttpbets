#include <raylib.h>
#include <raymath.h>
#include "config.h"
#include "player.h"
#include "config.h"
#include "lib/libeventbus.h"
#include "events.h"
#include "bomberman.h"
#include "audio.h"
#include "lib/libbombbrain/bomb_control.h"
#include "lib/libbombbrain/map.h"

// Controlling Bomberman
Bomberman* curr_bm;
float active_speed;

void on_move_performing(void* args)
{
    MoveEventArgs* a = (MoveEventArgs*)args;

    // Normalise direction, this fixes diagonal movement being twice as fast
    // Note that up is negative, hence -=
    Vector2 move_vec = (Vector2){a->x, -a->y};
    move_vec = Vector2Normalize(move_vec);

    // Update position
    move_box(&curr_bm->box, Vector2Scale(move_vec, active_speed * GetFrameTime()));

    // Update direction
    if (a->y > 0) // Up (highest prio)
        curr_bm->direction = 0;
    else if (a->y < 0) // Down
        curr_bm->direction = 1;
    else if (a->x < 0) // Left
        curr_bm->direction = 2;
    else if (a->x > 0) // Right
        curr_bm->direction = 3;

    // TODO: Move me to libbombbrain
    // Check for special collisions after moving
    Vector4 tiles = get_all_overlapping_tiles(curr_bm->box.position.x, curr_bm->box.position.y, curr_bm->box.size);
    // Loop through all overlapping tiles
    for (int ty = tiles.z; ty < tiles.w; ty++) {
        for (int tx = tiles.x; tx < tiles.y; tx++) {
            TileType tile = map[tx][ty];
            switch (tile) {
            case EXPLOSION:
                // Die :(
                break;

            case POWERUP_FIRE: // Consume Powerup
                map[tx][ty] = EMPTY;
                curr_bm->inventory.num_fires++;
                play_sound(SFX_POWERUP_FIRE);
                break;

            case POWERUP_BOMB: // Consume Powerup
                map[tx][ty] = EMPTY;
                curr_bm->inventory.num_bombs++;
                curr_bm->inventory.remaining_bombs++;
                play_sound(SFX_POWERUP_BOMB);
                break;

            default:
                break;
            }
        }
    }

    curr_bm->is_moving = true;
}

void on_move_released(void* args)
{
    (void)args; // Unused
    curr_bm->is_moving = false;
}

void on_sprint_toggled(void* args)
{
    SprintEventArgs* a = (SprintEventArgs*)args;
    active_speed = a->toggled ? CONFIG.PHYSICS.PLAYER_SPRINT_SPEED : CONFIG.PHYSICS.PLAYER_SPEED;
}

void on_bomb_pressed(void* args)
{
    (void)args; // Unused

    // TODO: Move me to bombd
    Vector2 pos = get_center_box(&curr_bm->box);
    if (place_bomb(pos, &curr_bm->inventory))
        play_sound(SFX_PLACEBOMB);
}

void player_init(Bomberman* bm)
{
    curr_bm = bm;

    // Listen for input
    event_bus_listen(EVENT_INPUT_MOVE_PERFORMING, on_move_performing);
    event_bus_listen(EVENT_INPUT_MOVE_RELEASED, on_move_released);
    event_bus_listen(EVENT_INPUT_SPRINT_CHANGED, on_sprint_toggled);
    event_bus_listen(EVENT_INPUT_BOMB_PRESSED, on_bomb_pressed);
}
