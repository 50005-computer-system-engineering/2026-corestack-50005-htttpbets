#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "raylib.h"
#include <raymath.h>
#include "network.h"
#include "config.h"
#include "events.h"
#include "lib/libeventbus.h"
#include "lib/libbattleroyale/client.h"
#include "lib/libbombprotocol/protocol.h"
#include "lib/libbombbrain/map.h"
#include "lib/libbombbrain/bomb_control.h"

static BRClient* client = NULL;
static uint32_t local_id = 0;

static Bomberman players[BOMBD_MAX_PLAYERS];
static uint32_t player_ids[BOMBD_MAX_PLAYERS];
static bool player_slot_used[BOMBD_MAX_PLAYERS] = {0};
static bool player_alive[BOMBD_MAX_PLAYERS];
static int player_count = 0;

static bool has_map = false;
static bool game_over = false;
static uint32_t winner_id = 0;

// Once dead, WASD flies this free-roaming camera target around the map
// instead of moving a body, clamped so it can't drift past the border walls
#define SPECTATOR_CAM_SPEED 6.0f // Grid units/sec
static Vector2 spectator_cam_pos = {1.0f, 1.0f};

/* ----- CONNECTION ----- */
bool network_connect(const char* server_ip)
{
    if (brclient_init(&client) < 0) {
        printf("[bombapp] Failed to create network client.\n");
        return false;
    }
    if (brclient_join(client, (char*)server_ip) < 0) {
        printf("[bombapp] Failed to join lobby at %s.\n", server_ip);
        printf("[bombapp] Check bombd is running there and the IP is correct.\n");
        return false;
    }

    printf("[bombapp] Connected, waiting for the match to start...\n");
    int waited_ms = 0;
    while (brclient_get_state(client) != BRCLIENT_STATE_GAME) {
        usleep(100000);
        waited_ms += 100;
        if (waited_ms >= NETWORK_LOBBY_WAIT_TIMEOUT_MS) {
            printf("[bombapp] Timed out waiting for the match to start.\n");
            return false;
        }
    }

    if (brclient_get_id(client, &local_id) < 0) {
        printf("[bombapp] Failed to retrieve player ID from server.\n");
        return false;
    }

    printf("[bombapp] Match started! Playing as P%u.\n", local_id);
    return true;
}

bool network_wait_for_map(void)
{
    int waited_ms = 0;
    while (!has_map) {
        network_poll();
        if (has_map) {
            break;
        }
        usleep(50000);
        waited_ms += 50;
        if (waited_ms >= NETWORK_MAP_WAIT_TIMEOUT_MS) {
            printf("[bombapp] Timed out waiting for the starting map.\n");
            return false;
        }
    }
    return true;
}

/* ----- ROSTER / PLAYER SLOTS ----- */
static int find_or_add_slot(uint32_t player_id)
{
    for (int i = 0; i < BOMBD_MAX_PLAYERS; i++) {
        if (player_slot_used[i] && player_ids[i] == player_id) {
            return i;
        }
    }
    for (int i = 0; i < BOMBD_MAX_PLAYERS; i++) {
        if (!player_slot_used[i]) {
            player_slot_used[i] = true;
            player_ids[i] = player_id;
            player_alive[i] = true;
            players[i] = bomberman_create_default((Vector2){1, 1});
            if (i >= player_count) {
                player_count = i + 1;
            }
            return i;
        }
    }
    return -1; // Every slot taken, drop it (shouldn't happen: BOMBD_MAX_PLAYERS matches the server)
}

static void apply_roster(const RosterPayload* roster)
{
    for (uint32_t i = 0; i < roster->count && i < BOMBD_MAX_PLAYERS; i++) {
        find_or_add_slot(roster->ids[i]);
    }
}

/* ----- MAP ----- */
static void apply_map_init(const MapInitPayload* payload)
{
    map_free(); // Discard tiles_init()'s throwaway local map

    map_size = (int)payload->map_size;
    map = (TileType**)malloc((size_t)map_size * sizeof(TileType*));
    for (int i = 0; i < map_size; i++) {
        map[i] = (TileType*)malloc((size_t)map_size * sizeof(TileType));
        for (int j = 0; j < map_size; j++) {
            map[i][j] = (i < BOMBD_MAX_MAP_DIM && j < BOMBD_MAX_MAP_DIM)
                ? (TileType)payload->tiles[i][j]
                : EMPTY;
        }
    }

    has_map = true;
}

static void apply_tile_update(const TileUpdatePayload* payload)
{
    if ((int)payload->x < map_size && (int)payload->y < map_size && map != NULL) {
        map[payload->x][payload->y] = (TileType)payload->tile;
    }
}

/* ----- STATE ----- */
static void apply_state(const StatePayload* payload)
{
    for (uint32_t i = 0; i < payload->player_count && i < BOMBD_MAX_PLAYERS; i++) {
        const PlayerStateEntry* entry = &payload->players[i];
        int slot = find_or_add_slot(entry->player_id);
        if (slot < 0) {
            continue;
        }

        Bomberman* bm = &players[slot];
        bm->box.position = (Vector2){entry->x, entry->y};
        bm->direction = (int)entry->direction;
        bm->is_moving = entry->is_moving != 0;
        bm->inventory.num_bombs = (int)entry->num_bombs;
        bm->inventory.remaining_bombs = (int)entry->remaining_bombs;
        bm->inventory.num_fires = (int)entry->num_fires;

        bool now_alive = entry->alive != 0;
        if (player_alive[slot] && !now_alive && entry->player_id == local_id) {
            event_bus_trigger(EVENT_PLAYER_DIED, NULL);
            spectator_cam_pos = bm->box.position; // Start flying from where we died
        }
        player_alive[slot] = now_alive;
    }

    // Re-sync the ticking-bomb visuals straight from the server instead of
    // simulating countdowns locally, so the red-tint animation never drifts
    Bombs_init(&bombs_queue);
    for (uint32_t i = 0; i < payload->bomb_count && i < BOMBD_MAX_BOMBS; i++) {
        BombInfo bomb = {
            .position = (Vector2){payload->bombs[i].x, payload->bombs[i].y},
            .spread = 0, // Unused for rendering
            .timer = payload->bombs[i].timer};
        Bombs_enqueue(&bombs_queue, bomb);
    }
}

/* ----- EXPLOSIONS ----- */
static void apply_explosion(const ExplosionPayload* payload)
{
    ExplosionInfo explosion = {0};
    explosion.center = (Vector2){payload->center_x, payload->center_y};
    for (int i = 0; i < 4; i++) {
        explosion.spread_amt[i] = (int)payload->spread_amt[i];
    }
    // Purely visual: no map tiles to clear here, TILE_UPDATE already did that
    explosion.spread_positions = NULL;
    explosion.spread_positions_size = 0;
    explosion.timer = EXPLODE_LIFETIME;
    Explodes_enqueue(&explodes_queue, explosion);

    // The server is the one that actually calls trigger_explosion()
    // (libbombbrain), so it's the only place EVENT_BOMB_EXPLODED normally
    // fires from - but bombd never has audio initialised. Firing it here
    // too is what makes audio.c's listener actually play the SFX locally
    event_bus_trigger(EVENT_BOMB_EXPLODED, &(TileEventArgs){.x = (int)payload->center_x, .y = (int)payload->center_y});
}

void network_tick_explosions(float delta_time)
{
    // Client-only twin of libbombbrain's tick_explosions(): expiry only,
    // never touches the map (spread_positions is never populated here)
    if (Explodes_empty(&explodes_queue)) {
        return;
    }
    for (int i = explodes_queue.front + 1; i < explodes_queue.rear; i++) {
        ExplosionInfo* explosion = &explodes_queue.items[i];
        explosion->timer -= delta_time;
        if (explosion->timer <= 0) {
            explodes_queue.items[i] = explodes_queue.items[--explodes_queue.rear];
            i--;
        }
    }
}

/* ----- GAME OVER ----- */
static void apply_game_over(const GameOverPayload* payload)
{
    game_over = true;
    winner_id = payload->winner_id;

    if (winner_id != 0 && winner_id == local_id) {
        event_bus_trigger(EVENT_MATCH_WON, NULL);
    }
}

/* ----- POLLING ----- */
void network_poll(void)
{
    unsigned char buffer[1024] = {0};
    while (brclient_get_app_msg(buffer) == 1) {
        uint32_t tag = read_bomb_packet_tag(buffer);
        switch (tag) {
        case PACKET_ROSTER: {
            RosterPayload roster;
            unpack_roster(buffer, &roster);
            apply_roster(&roster);
            break;
        }
        case PACKET_MAP_INIT: {
            MapInitPayload map_init;
            unpack_map_init(buffer, &map_init);
            apply_map_init(&map_init);
            break;
        }
        case PACKET_TILE_UPDATE: {
            TileUpdatePayload tile_update;
            unpack_tile_update(buffer, &tile_update);
            apply_tile_update(&tile_update);
            break;
        }
        case PACKET_STATE: {
            StatePayload state;
            unpack_state(buffer, &state);
            apply_state(&state);
            break;
        }
        case PACKET_EXPLOSION: {
            ExplosionPayload explosion;
            unpack_explosion(buffer, &explosion);
            apply_explosion(&explosion);
            break;
        }
        case PACKET_GAME_OVER: {
            GameOverPayload over;
            unpack_game_over(buffer, &over);
            apply_game_over(&over);
            break;
        }
        default:
            break; // Unknown tag, ignore
        }
    }
}

/* ----- INPUT ----- */
// Moves the spectator camera with WASD and clamps it inside the map, leaving
// a 1-tile margin so it never sits on top of the border wall
static void update_spectator_camera(void)
{
    float dx = (IsKeyDown(CONFIG.KEYS.MOVE_RIGHT) ? 1.0f : 0.0f) - (IsKeyDown(CONFIG.KEYS.MOVE_LEFT) ? 1.0f : 0.0f);
    float dy = (IsKeyDown(CONFIG.KEYS.MOVE_DOWN) ? 1.0f : 0.0f) - (IsKeyDown(CONFIG.KEYS.MOVE_UP) ? 1.0f : 0.0f);

    if (dx != 0.0f || dy != 0.0f) {
        Vector2 move = Vector2Scale(Vector2Normalize((Vector2){dx, dy}), SPECTATOR_CAM_SPEED * GetFrameTime());
        spectator_cam_pos = Vector2Add(spectator_cam_pos, move);
    }

    float max_coord = (float)map_size - 1.0f;
    if (spectator_cam_pos.x < 1.0f) {
        spectator_cam_pos.x = 1.0f;
    } else if (spectator_cam_pos.x > max_coord) {
        spectator_cam_pos.x = max_coord;
    }
    if (spectator_cam_pos.y < 1.0f) {
        spectator_cam_pos.y = 1.0f;
    } else if (spectator_cam_pos.y > max_coord) {
        spectator_cam_pos.y = max_coord;
    }
}

void network_send_input(void)
{
    int slot = network_local_slot();
    if (slot < 0 || !player_alive[slot]) {
        // Dead (or not yet assigned a slot): nothing to move server-side,
        // WASD flies the spectator camera instead
        update_spectator_camera();
        return;
    }

    int dx = IsKeyDown(CONFIG.KEYS.MOVE_RIGHT) ? 1 : IsKeyDown(CONFIG.KEYS.MOVE_LEFT) ? -1
                                                                                       : 0;
    int dy = IsKeyDown(CONFIG.KEYS.MOVE_UP) ? 1 : IsKeyDown(CONFIG.KEYS.MOVE_DOWN) ? -1
                                                                                    : 0;
    bool sprinting = IsKeyDown(CONFIG.KEYS.SPRINT);

    MovePayload move = {
        .player_id = local_id,
        .dx = dx,
        .dy = dy,
        .sprinting = sprinting ? 1u : 0u};
    unsigned char buffer[512] = {0};
    pack_move(buffer, &move);
    brclient_send_msg_udp(client, buffer);

    if (IsKeyPressed(CONFIG.KEYS.BOMB)) {
        BombPlacePayload bomb_place = {.player_id = local_id};
        unsigned char bomb_buffer[512] = {0};
        pack_bomb_place(bomb_buffer, &bomb_place);
        brclient_send_msg(client, bomb_buffer);
    }
}

/* ----- ACCESSORS ----- */
uint32_t network_local_player_id(void) { return local_id; }

int network_local_slot(void)
{
    for (int i = 0; i < BOMBD_MAX_PLAYERS; i++) {
        if (player_slot_used[i] && player_ids[i] == local_id) {
            return i;
        }
    }
    return -1;
}

// What the camera should be looking at: the local player's real position
// while alive, or the free-flying spectator position once dead
Vector2 network_get_camera_target(void)
{
    int slot = network_local_slot();
    if (slot >= 0 && player_alive[slot]) {
        return players[slot].box.position;
    }
    return spectator_cam_pos;
}

int network_player_count(void) { return player_count; }
Bomberman* network_get_player(int index) { return &players[index]; }
uint32_t network_get_player_id(int index) { return player_ids[index]; }
bool network_is_player_alive(int index) { return player_alive[index]; }
bool network_game_over(void) { return game_over; }
uint32_t network_winner_id(void) { return winner_id; }
