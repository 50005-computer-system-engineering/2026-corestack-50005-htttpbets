#include <string.h>
#include <arpa/inet.h>
#include "protocol.h"

// Size of the tag that opens every packet
#define TAG_SIZE sizeof(uint32_t)

/* ----- INTERNAL HELPERS ----- */
// Writes the packet tag at offset 0 and returns where the payload should start
static unsigned char* write_tag(unsigned char buffer[512], BombPacketType type)
{
    uint32_t tag = htonl(type);
    memset(buffer, 0, 512); // Clear any stale bytes from a previous send
    memcpy(buffer, &tag, TAG_SIZE);
    return buffer + TAG_SIZE; // Payload always starts right after the tag
}

// Mirror of write_tag for the receiving side
static const unsigned char* payload_start(const unsigned char buffer[512])
{
    return buffer + TAG_SIZE;
}

// htonl/ntohl only understand integers, so floats are reinterpreted as their
// raw IEEE754 bits before conversion. Both ends target the same architecture
// family (x86), so this round-trips exactly.
//
// The result is stored back into a float-typed destination via memcpy rather
// than a plain assignment: `float_field = some_uint32_t` would silently ask
// the compiler for a NUMERIC int->float conversion (eg: 0xCDCCAC3F becomes
// ~3.45e9, not the intended bit pattern), which is not what byte-swapping
// wants. memcpy sidesteps that entirely by copying raw bytes
static void write_net_float(float* dst, float src)
{
    uint32_t bits;
    memcpy(&bits, &src, sizeof(bits));
    bits = htonl(bits);
    memcpy(dst, &bits, sizeof(bits));
}

// Mirror of write_net_float: net_value already holds raw network-order bits
// (copied verbatim off the wire into a float-typed struct field), so this
// must not be handed to ntohl by value either - same numeric-conversion trap
static float read_net_float(float net_value)
{
    uint32_t bits;
    memcpy(&bits, &net_value, sizeof(bits));
    bits = ntohl(bits);
    float f;
    memcpy(&f, &bits, sizeof(f));
    return f;
}

/* ----- TAG ----- */
uint32_t read_bomb_packet_tag(const unsigned char buffer[512])
{
    uint32_t tag;
    memcpy(&tag, buffer, TAG_SIZE);
    return ntohl(tag);
}

/* ----- MOVE ----- */
void pack_move(unsigned char buffer[512], const MovePayload* payload)
{
    unsigned char* out = write_tag(buffer, PACKET_MOVE);

    MovePayload net = {
        .player_id = htonl(payload->player_id),
        .dx = (int32_t)htonl((uint32_t)payload->dx),
        .dy = (int32_t)htonl((uint32_t)payload->dy),
        .sprinting = htonl(payload->sprinting)};

    memcpy(out, &net, sizeof(MovePayload));
}

void unpack_move(const unsigned char buffer[512], MovePayload* payload)
{
    MovePayload net;
    memcpy(&net, payload_start(buffer), sizeof(MovePayload));

    payload->player_id = ntohl(net.player_id);
    payload->dx = (int32_t)ntohl((uint32_t)net.dx);
    payload->dy = (int32_t)ntohl((uint32_t)net.dy);
    payload->sprinting = ntohl(net.sprinting);
}

/* ----- BOMB PLACE ----- */
void pack_bomb_place(unsigned char buffer[512], const BombPlacePayload* payload)
{
    unsigned char* out = write_tag(buffer, PACKET_BOMB_PLACE);

    BombPlacePayload net = {.player_id = htonl(payload->player_id)};

    memcpy(out, &net, sizeof(BombPlacePayload));
}

void unpack_bomb_place(const unsigned char buffer[512], BombPlacePayload* payload)
{
    BombPlacePayload net;
    memcpy(&net, payload_start(buffer), sizeof(BombPlacePayload));

    payload->player_id = ntohl(net.player_id);
}

/* ----- ROSTER ----- */
void pack_roster(unsigned char buffer[512], const RosterPayload* payload)
{
    unsigned char* out = write_tag(buffer, PACKET_ROSTER);

    RosterPayload net = {0};
    net.count = htonl(payload->count);
    for (uint32_t i = 0; i < BOMBD_MAX_PLAYERS; i++) {
        net.ids[i] = htonl(payload->ids[i]);
    }

    memcpy(out, &net, sizeof(RosterPayload));
}

void unpack_roster(const unsigned char buffer[512], RosterPayload* payload)
{
    RosterPayload net;
    memcpy(&net, payload_start(buffer), sizeof(RosterPayload));

    payload->count = ntohl(net.count);
    for (uint32_t i = 0; i < BOMBD_MAX_PLAYERS; i++) {
        payload->ids[i] = ntohl(net.ids[i]);
    }
}

/* ----- MAP INIT ----- */
void pack_map_init(unsigned char buffer[512], const MapInitPayload* payload)
{
    unsigned char* out = write_tag(buffer, PACKET_MAP_INIT);

    MapInitPayload net;
    net.map_size = htonl(payload->map_size);
    // Tiles are single bytes, no byte-order conversion needed
    memcpy(net.tiles, payload->tiles, sizeof(net.tiles));

    memcpy(out, &net, sizeof(MapInitPayload));
}

void unpack_map_init(const unsigned char buffer[512], MapInitPayload* payload)
{
    MapInitPayload net;
    memcpy(&net, payload_start(buffer), sizeof(MapInitPayload));

    payload->map_size = ntohl(net.map_size);
    memcpy(payload->tiles, net.tiles, sizeof(payload->tiles));
}

/* ----- TILE UPDATE ----- */
void pack_tile_update(unsigned char buffer[512], const TileUpdatePayload* payload)
{
    unsigned char* out = write_tag(buffer, PACKET_TILE_UPDATE);

    TileUpdatePayload net = {
        .x = htonl(payload->x),
        .y = htonl(payload->y),
        .tile = htonl(payload->tile)};

    memcpy(out, &net, sizeof(TileUpdatePayload));
}

void unpack_tile_update(const unsigned char buffer[512], TileUpdatePayload* payload)
{
    TileUpdatePayload net;
    memcpy(&net, payload_start(buffer), sizeof(TileUpdatePayload));

    payload->x = ntohl(net.x);
    payload->y = ntohl(net.y);
    payload->tile = ntohl(net.tile);
}

/* ----- STATE ----- */
void pack_state(unsigned char buffer[512], const StatePayload* payload)
{
    unsigned char* out = write_tag(buffer, PACKET_STATE);

    StatePayload net = {0};
    net.player_count = htonl(payload->player_count);
    for (uint32_t i = 0; i < payload->player_count && i < BOMBD_MAX_PLAYERS; i++) {
        const PlayerStateEntry* src = &payload->players[i];
        PlayerStateEntry* dst = &net.players[i];
        dst->player_id = htonl(src->player_id);
        write_net_float(&dst->x, src->x);
        write_net_float(&dst->y, src->y);
        dst->direction = htonl(src->direction);
        dst->is_moving = htonl(src->is_moving);
        dst->alive = htonl(src->alive);
        dst->num_bombs = htonl(src->num_bombs);
        dst->remaining_bombs = htonl(src->remaining_bombs);
        dst->num_fires = htonl(src->num_fires);
    }

    net.bomb_count = htonl(payload->bomb_count);
    for (uint32_t i = 0; i < payload->bomb_count && i < BOMBD_MAX_BOMBS; i++) {
        write_net_float(&net.bombs[i].x, payload->bombs[i].x);
        write_net_float(&net.bombs[i].y, payload->bombs[i].y);
        write_net_float(&net.bombs[i].timer, payload->bombs[i].timer);
    }

    memcpy(out, &net, sizeof(StatePayload));
}

void unpack_state(const unsigned char buffer[512], StatePayload* payload)
{
    StatePayload net;
    memcpy(&net, payload_start(buffer), sizeof(StatePayload));

    payload->player_count = ntohl(net.player_count);
    if (payload->player_count > BOMBD_MAX_PLAYERS) {
        payload->player_count = BOMBD_MAX_PLAYERS; // Failsafe against a malformed packet
    }
    for (uint32_t i = 0; i < payload->player_count; i++) {
        const PlayerStateEntry* src = &net.players[i];
        PlayerStateEntry* dst = &payload->players[i];
        dst->player_id = ntohl(src->player_id);
        dst->x = read_net_float(src->x);
        dst->y = read_net_float(src->y);
        dst->direction = ntohl(src->direction);
        dst->is_moving = ntohl(src->is_moving);
        dst->alive = ntohl(src->alive);
        dst->num_bombs = ntohl(src->num_bombs);
        dst->remaining_bombs = ntohl(src->remaining_bombs);
        dst->num_fires = ntohl(src->num_fires);
    }

    payload->bomb_count = ntohl(net.bomb_count);
    if (payload->bomb_count > BOMBD_MAX_BOMBS) {
        payload->bomb_count = BOMBD_MAX_BOMBS;
    }
    for (uint32_t i = 0; i < payload->bomb_count; i++) {
        payload->bombs[i].x = read_net_float(net.bombs[i].x);
        payload->bombs[i].y = read_net_float(net.bombs[i].y);
        payload->bombs[i].timer = read_net_float(net.bombs[i].timer);
    }
}

/* ----- EXPLOSION ----- */
void pack_explosion(unsigned char buffer[512], const ExplosionPayload* payload)
{
    unsigned char* out = write_tag(buffer, PACKET_EXPLOSION);

    ExplosionPayload net;
    write_net_float(&net.center_x, payload->center_x);
    write_net_float(&net.center_y, payload->center_y);
    for (int i = 0; i < 4; i++) {
        net.spread_amt[i] = htonl(payload->spread_amt[i]);
    }

    memcpy(out, &net, sizeof(ExplosionPayload));
}

void unpack_explosion(const unsigned char buffer[512], ExplosionPayload* payload)
{
    ExplosionPayload net;
    memcpy(&net, payload_start(buffer), sizeof(ExplosionPayload));

    payload->center_x = read_net_float(net.center_x);
    payload->center_y = read_net_float(net.center_y);
    for (int i = 0; i < 4; i++) {
        payload->spread_amt[i] = ntohl(net.spread_amt[i]);
    }
}

/* ----- GAME OVER ----- */
void pack_game_over(unsigned char buffer[512], const GameOverPayload* payload)
{
    unsigned char* out = write_tag(buffer, PACKET_GAME_OVER);

    GameOverPayload net = {.winner_id = htonl(payload->winner_id)};

    memcpy(out, &net, sizeof(GameOverPayload));
}

void unpack_game_over(const unsigned char buffer[512], GameOverPayload* payload)
{
    GameOverPayload net;
    memcpy(&net, payload_start(buffer), sizeof(GameOverPayload));

    payload->winner_id = ntohl(net.winner_id);
}
