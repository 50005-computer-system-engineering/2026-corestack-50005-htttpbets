#ifndef BOMBD_SESSION_H
#define BOMBD_SESSION_H

#include <stdbool.h>
#include <stdint.h>
#include "lib/libbombbrain/bomb_control.h"
#include "lib/libbombbrain/collision.h"
#include "lib/libbombbrain/inventory.h"
#include "lib/libbombprotocol/protocol.h"
#include <raymath.h>

// How often the authoritative simulation advances
#define TICK_MICROSECONDS 16000 // ~60Hz, matches bombapp's SetTargetFPS(60)

// Upper bound on messages drained per tick, stops a flood of client packets
// from starving the simulation
#define MAX_MSGS_PER_TICK 128

/* ----- PLAYER SLOT ----- */
// Bomberman is one shared arena rather than per-player boards, so a slot only
// tracks this player's own body + inventory; the map/bombs/explosions below
// live once, globally, in libbombbrain (map.c / bomb_control.c)
typedef struct
{
    uint32_t player_id;
    bool active; // Slot is in use (connected)
    bool alive;  // Still alive this match (false once caught in an explosion)

    BoundBox box;
    int direction; // See Facing in libbombprotocol/protocol.h
    bool is_moving;
    InventoryStock inventory;

    // Latest held-movement state from the client, applied every tick until replaced
    int move_dx, move_dy;
    bool sprinting;
} BombPlayer;

/* ----- SESSION ----- */
typedef struct
{
    BombPlayer players[BOMBD_MAX_PLAYERS];
    int count;
    bool dirty; // Something changed since the last state broadcast
} BombSession;

// Generates the shared map and spawns every player into a corner
void init_session(BombSession* session, const uint32_t* client_ids, uint32_t player_count);
void free_session(BombSession* session);

// Finds a player's slot by ID, returns NULL if that ID is not in the match
BombPlayer* find_bomb_player(BombSession* session, uint32_t player_id);

// Records the client's currently-held movement direction; applied on the next tick
void apply_move(BombPlayer* player, const MovePayload* move);

// Validates and enqueues a bomb at the player's current tile
void apply_bomb_place(BombPlayer* player);

// Called once per explosion newly triggered this tick, before the explosion
// queue can be reshuffled by tick_explosions() dequeuing an expired one
typedef void (*ExplosionCallback)(const ExplosionInfo* explosion, void* user_data);

// Advances movement, bombs, explosions, and powerup pickups by one tick
void tick_session(BombSession* session, float delta_time, ExplosionCallback on_explosion, void* user_data);

int count_alive_players(const BombSession* session);

#endif
