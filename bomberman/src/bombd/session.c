#include <string.h>
#include "lib/libbombbrain/map.h"
#include "lib/libbombbrain/bomb_control.h"
#include "lib/libbombbrain/powerup_control.h"
#include "lib/libbombbrain/sim_config.h"
#include "session.h"

/* ----- SETUP HELPERS ----- */
// map_generate() only marks corner tiles as PLAYER internally, it never
// returns the coordinates, so recover them by scanning, then clear the
// markers back to EMPTY since "PLAYER" isn't real persistent map state
static void collect_spawns(Vector2* spawns_out, int max_spawns, int* found_count)
{
    *found_count = 0;
    for (int i = 0; i < map_size && *found_count < max_spawns; i++) {
        for (int j = 0; j < map_size && *found_count < max_spawns; j++) {
            if (map[i][j] == PLAYER) {
                spawns_out[*found_count] = (Vector2){i, j};
                map[i][j] = EMPTY;
                (*found_count)++;
            }
        }
    }
}

void init_session(BombSession* session, const uint32_t* client_ids, uint32_t player_count)
{
    memset(session, 0, sizeof(BombSession));
    if (player_count > BOMBD_MAX_PLAYERS) {
        player_count = BOMBD_MAX_PLAYERS;
    }

    map_generate((int)player_count);
    init_bombs();
    powerup_init();

    Vector2 spawns[BOMBD_MAX_PLAYERS];
    int spawn_count = 0;
    collect_spawns(spawns, (int)player_count, &spawn_count);

    session->count = (int)player_count;
    for (uint32_t i = 0; i < player_count; i++) {
        BombPlayer* p = &session->players[i];
        // Falls back to the top-left corner in the pathological case where
        // fewer spawn tiles were found than players (map generation bug)
        Vector2 tile = (i < (uint32_t)spawn_count) ? spawns[i] : (Vector2){1, 1};

        p->player_id = client_ids[i];
        p->active = true;
        p->alive = true;
        p->box.position = Vector2Add(tile, (Vector2){SIM_PLAYER_BOX_OFFSET_X, SIM_PLAYER_BOX_OFFSET_Y});
        p->box.size = (Vector2){SIM_PLAYER_BOX_SIZE_X, SIM_PLAYER_BOX_SIZE_Y};
        p->direction = FACING_DOWN;
        p->is_moving = false;

        p->inventory.num_bombs = 2;
        p->inventory.remaining_bombs = p->inventory.num_bombs;
        p->inventory.num_fires = 1;
        p->inventory.bomb_replenish_timer = 0;
    }
    session->dirty = true;
}

void free_session(BombSession* session)
{
    (void)session;
    powerup_free();
    map_free();
}

BombPlayer* find_bomb_player(BombSession* session, uint32_t player_id)
{
    for (int i = 0; i < session->count; i++) {
        if (session->players[i].active && session->players[i].player_id == player_id) {
            return &session->players[i];
        }
    }
    return NULL;
}

/* ----- INPUT ----- */
void apply_move(BombPlayer* player, const MovePayload* move)
{
    if (player == NULL || !player->alive) {
        return; // Dead or unknown player, nothing to steer
    }
    player->move_dx = move->dx;
    player->move_dy = move->dy;
    player->sprinting = move->sprinting != 0;
}

void apply_bomb_place(BombPlayer* player)
{
    if (player == NULL || !player->alive) {
        return;
    }
    Vector2 pos = get_center_box(&player->box);
    place_bomb(pos, &player->inventory);
}

/* ----- SIMULATION ----- */
static void apply_movement_and_pickups(BombSession* session, float delta_time)
{
    for (int i = 0; i < session->count; i++) {
        BombPlayer* p = &session->players[i];
        if (!p->active || !p->alive) {
            continue;
        }

        bool moving = (p->move_dx != 0 || p->move_dy != 0);
        if (moving) {
            float speed = p->sprinting ? SIM_PLAYER_SPRINT_SPEED : SIM_PLAYER_SPEED;
            // Up is negative Y, mirrors bombapp/player.c's on_move_performing
            Vector2 move_vec = Vector2Normalize((Vector2){(float)p->move_dx, -(float)p->move_dy});
            move_box(&p->box, Vector2Scale(move_vec, speed * delta_time));

            if (p->move_dy > 0) {
                p->direction = FACING_UP;
            } else if (p->move_dy < 0) {
                p->direction = FACING_DOWN;
            } else if (p->move_dx < 0) {
                p->direction = FACING_LEFT;
            } else if (p->move_dx > 0) {
                p->direction = FACING_RIGHT;
            }

            session->dirty = true;
        }
        if (p->is_moving != moving) {
            p->is_moving = moving;
            session->dirty = true;
        }

        // Powerup pickups: authoritative twin of bombapp/player.c's client-side check
        Vector4 tiles = get_all_overlapping_tiles(p->box.position.x, p->box.position.y, p->box.size);
        for (int ty = (int)tiles.z; ty < (int)tiles.w; ty++) {
            for (int tx = (int)tiles.x; tx < (int)tiles.y; tx++) {
                if (tx < 0 || ty < 0 || tx >= map_size || ty >= map_size) {
                    continue;
                }
                if (map[tx][ty] == POWERUP_FIRE) {
                    map[tx][ty] = EMPTY;
                    p->inventory.num_fires++;
                    session->dirty = true;
                } else if (map[tx][ty] == POWERUP_BOMB) {
                    map[tx][ty] = EMPTY;
                    p->inventory.num_bombs++;
                    p->inventory.remaining_bombs++;
                    session->dirty = true;
                }
            }
        }

        inventory_update(&p->inventory, delta_time);
    }
}

static void kill_players_in_explosions(BombSession* session)
{
    for (int i = 0; i < session->count; i++) {
        BombPlayer* p = &session->players[i];
        if (!p->active || !p->alive) {
            continue;
        }

        Vector4 tiles = get_all_overlapping_tiles(p->box.position.x, p->box.position.y, p->box.size);
        for (int ty = (int)tiles.z; ty < (int)tiles.w && p->alive; ty++) {
            for (int tx = (int)tiles.x; tx < (int)tiles.y && p->alive; tx++) {
                if (tx < 0 || ty < 0 || tx >= map_size || ty >= map_size) {
                    continue;
                }
                if (map[tx][ty] == EXPLOSION) {
                    p->alive = false;
                    session->dirty = true;
                }
            }
        }
    }
}

void tick_session(BombSession* session, float delta_time, ExplosionCallback on_explosion, void* user_data)
{
    apply_movement_and_pickups(session, delta_time);

    // Bombs/explosions are one shared pool, not per-player, so libbombbrain
    // owns them directly (bombs_queue / explodes_queue / the map array).
    // Explosions newly added by tick_bombs() must be reported before
    // tick_explosions() runs: it dequeues expired entries by swapping in the
    // last item, which would otherwise reshuffle the ones we just added
    int explosions_before = explodes_queue.rear;
    tick_bombs(delta_time);
    if (on_explosion != NULL) {
        for (int i = explosions_before; i < explodes_queue.rear; i++) {
            on_explosion(&explodes_queue.items[i], user_data);
        }
    }
    tick_explosions(delta_time);

    kill_players_in_explosions(session);
}

/* ----- MATCH END ----- */
int count_alive_players(const BombSession* session)
{
    int alive = 0;
    for (int i = 0; i < session->count; i++) {
        if (session->players[i].active && session->players[i].alive) {
            alive++;
        }
    }
    return alive;
}
