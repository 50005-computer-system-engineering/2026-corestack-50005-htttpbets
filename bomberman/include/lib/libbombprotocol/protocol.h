#ifndef BOMBPROTOCOL_PROTOCOL_H
#define BOMBPROTOCOL_PROTOCOL_H

#include <stdint.h>
#include <stdbool.h>

/* ----- SHARED LIMITS ----- */
// Bomberman is played on one shared arena, unlike tetris's per-player boards,
// so these bound the single shared world rather than a per-room lobby size
#define BOMBD_MAX_PLAYERS 25
#define BOMBD_MAX_BOMBS 100      // Ticking bombs visible on the shared map at once
#define BOMBD_MAX_MAP_DIM 20    // Covers calc_map_size(BOMBD_MAX_PLAYERS) + border (20x20 at 100 players)

// Every packet is capped at 512 bytes (MSG_CONTENT_LENGTH is 1024, but this
// protocol only ever uses the first 512). At 36 bytes/player and 12
// bytes/bomb, BOMBD_MAX_PLAYERS x BOMBD_MAX_BOMBS worth of StatePayload data
// doesn't come close to fitting in one packet, so pack_state()/unpack_state()
// clamp to these counts instead.
// TODO: page StatePayload across multiple packets to lift this ceiling (and
// BOMBD_MAX_MAP_DIM has the same problem for very large maps/MapInitPayload)
#define STATE_PACKET_MAX_PLAYERS 8
#define STATE_PACKET_MAX_BOMBS 16

/* ----- PACKET TAGS ----- */
// Every packet on the app-message channel opens with one of these as a 4-byte
// tag (network byte order) so the receiver knows how to read the bytes after it.
// Transport is noted per tag: the client picks TCP vs UDP when sending, the
// server picks brserver_send_to_all (TCP) vs brserver_send_broadcast (UDP)
typedef enum {
    PACKET_MOVE = 0,        // Client -> Server (UDP), MovePayload
    PACKET_BOMB_PLACE = 1,  // Client -> Server (TCP), BombPlacePayload
    PACKET_ROSTER = 2,      // Server -> Client (TCP), RosterPayload
    PACKET_MAP_INIT = 3,    // Server -> Client (TCP), MapInitPayload
    PACKET_TILE_UPDATE = 4, // Server -> Client (TCP), TileUpdatePayload
    PACKET_STATE = 5,       // Server -> Client (TCP), StatePayload
    PACKET_EXPLOSION = 6,   // Server -> Client (UDP), ExplosionPayload
    PACKET_GAME_OVER = 7    // Server -> Client (TCP), GameOverPayload
} BombPacketType;

/* ----- FACING ----- */
typedef enum {
    FACING_UP = 0,
    FACING_DOWN = 1,
    FACING_LEFT = 2,
    FACING_RIGHT = 3
} Facing;

/* ----- PAYLOADS ----- */
// Held movement state, sent every input frame while any direction key is
// down. UDP is fine to lose one of these: the next frame's send corrects it
typedef struct
{
    uint32_t player_id;
    int32_t dx;         // -1, 0, or 1 (held horizontal direction)
    int32_t dy;         // -1, 0, or 1 (held vertical direction)
    uint32_t sprinting; // Boolean
} MovePayload;

// One bomb-placement request. Reliable, so this one must land
typedef struct
{
    uint32_t player_id;
} BombPlacePayload;

// Broadcast once at game start so every client learns who else is playing
typedef struct
{
    uint32_t count;
    uint32_t ids[BOMBD_MAX_PLAYERS];
} RosterPayload;

// Sent once at game start: the randomly generated arena every client renders
typedef struct
{
    uint32_t map_size; // Width and height (square)
    uint8_t tiles[BOMBD_MAX_MAP_DIM][BOMBD_MAX_MAP_DIM]; // TileType, row-major
} MapInitPayload;

// Sent whenever a single tile changes (bomb clears a breakable, powerup
// consumed, etc). Cheaper than re-sending the whole map every time
typedef struct
{
    uint32_t x;
    uint32_t y;
    uint32_t tile; // TileType
} TileUpdatePayload;

// One player's authoritative position/inventory, as carried inside StatePayload
typedef struct
{
    uint32_t player_id;
    float x, y; // Top-left of the player's hitbox, in grid units
    uint32_t direction;
    uint32_t is_moving;
    uint32_t alive;
    uint32_t num_bombs;
    uint32_t remaining_bombs;
    uint32_t num_fires;
} PlayerStateEntry;

// One ticking bomb's position/fuse, as carried inside StatePayload
typedef struct
{
    float x, y;
    float timer;
} BombStateEntry;

// Authoritative snapshot of every player and every ticking bomb, pushed by
// the server whenever something changes. Reliable: players popping in/out of
// existence, or a bomb disappearing, must never be missed
typedef struct
{
    uint32_t player_count;
    PlayerStateEntry players[BOMBD_MAX_PLAYERS];
    uint32_t bomb_count;
    BombStateEntry bombs[BOMBD_MAX_BOMBS];
} StatePayload;

// One-shot explosion animation event, purely cosmetic: the tiles it clears
// already arrived as PACKET_TILE_UPDATE, so losing this packet only costs
// a visual effect, never desyncs the map
typedef struct
{
    float center_x, center_y;
    uint32_t spread_amt[4]; // Up, Down, Left, Right
} ExplosionPayload;

// Sent once when the match ends
typedef struct
{
    uint32_t winner_id; // 0 = no winner / draw
} GameOverPayload;

/* ----- PACK / UNPACK ----- */
// Single place where the 4-byte tag and byte-order conversion happen
void pack_move(unsigned char buffer[512], const MovePayload* payload);
void unpack_move(const unsigned char buffer[512], MovePayload* payload);

void pack_bomb_place(unsigned char buffer[512], const BombPlacePayload* payload);
void unpack_bomb_place(const unsigned char buffer[512], BombPlacePayload* payload);

void pack_roster(unsigned char buffer[512], const RosterPayload* payload);
void unpack_roster(const unsigned char buffer[512], RosterPayload* payload);

void pack_map_init(unsigned char buffer[512], const MapInitPayload* payload);
void unpack_map_init(const unsigned char buffer[512], MapInitPayload* payload);

void pack_tile_update(unsigned char buffer[512], const TileUpdatePayload* payload);
void unpack_tile_update(const unsigned char buffer[512], TileUpdatePayload* payload);

void pack_state(unsigned char buffer[512], const StatePayload* payload);
void unpack_state(const unsigned char buffer[512], StatePayload* payload);

void pack_explosion(unsigned char buffer[512], const ExplosionPayload* payload);
void unpack_explosion(const unsigned char buffer[512], ExplosionPayload* payload);

void pack_game_over(unsigned char buffer[512], const GameOverPayload* payload);
void unpack_game_over(const unsigned char buffer[512], GameOverPayload* payload);

// Reads just the tag so a receiver can decide which unpack to call
uint32_t read_bomb_packet_tag(const unsigned char buffer[512]);

#endif
