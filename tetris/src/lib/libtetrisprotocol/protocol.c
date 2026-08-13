#include <string.h>
#include <arpa/inet.h>
#include "protocol.h"

// Size of the tag that opens every packet
#define TAG_SIZE sizeof(uint32_t)

/* ----- INTERNAL HELPERS ----- */
// Writes the packet tag at offset 0 and returns where the payload should start
static unsigned char* write_tag(unsigned char buffer[512], PacketType type)
{
    uint32_t tag = htonl(type);
    memset(buffer, 0, 512); // Clear any stale bytes from a previous send
    memcpy(buffer, &tag, TAG_SIZE);
    return buffer + TAG_SIZE; // Payload always starts right after the tag
}

// Mirror of writeTag for the receiving side
static const unsigned char* payload_start(const unsigned char buffer[512])
{
    return buffer + TAG_SIZE;
}

/* ----- TAG ----- */
uint32_t read_packet_tag(const unsigned char buffer[512])
{
    uint32_t tag;
    memcpy(&tag, buffer, TAG_SIZE);
    return ntohl(tag); // Converted back so callers can compare against PacketType
}

/* ----- ATTACK ----- */
void pack_attack(unsigned char buffer[512], const AttackPayload* payload)
{
    unsigned char* out = write_tag(buffer, PACKET_ATTACK);

    // Convert every field into network byte order before it leaves client
    AttackPayload net = {
        .source_player = htonl(payload->source_player),
        .target_player = htonl(payload->target_player),
        .lines = htonl(payload->lines)};

    memcpy(out, &net, sizeof(AttackPayload));
}

void unpack_attack(const unsigned char buffer[512], AttackPayload* payload)
{
    AttackPayload net;
    memcpy(&net, payload_start(buffer), sizeof(AttackPayload));

    // Convert back into readable host integers
    payload->source_player = ntohl(net.source_player);
    payload->target_player = ntohl(net.target_player);
    payload->lines = ntohl(net.lines);
}

/* ----- ROSTER ----- */
void pack_roster(unsigned char buffer[512], const RosterPayload* payload)
{
    unsigned char* out = write_tag(buffer, PACKET_ROSTER);

    RosterPayload net = {0};
    net.count = htonl(payload->count);
    for (uint32_t i = 0; i < MAX_LOBBY_SIZE; i++) {
        net.ids[i] = htonl(payload->ids[i]);
    }

    memcpy(out, &net, sizeof(RosterPayload));
}

void unpack_roster(const unsigned char buffer[512], RosterPayload* payload)
{
    RosterPayload net;
    memcpy(&net, payload_start(buffer), sizeof(RosterPayload));

    payload->count = ntohl(net.count);
    for (uint32_t i = 0; i < MAX_LOBBY_SIZE; i++) {
        payload->ids[i] = ntohl(net.ids[i]);
    }
}

/* ----- INPUT ----- */
void pack_input(unsigned char buffer[512], const InputPayload* payload)
{
    unsigned char* out = write_tag(buffer, PACKET_INPUT);

    InputPayload net = {
        .player_id = htonl(payload->player_id),
        .action = htonl(payload->action)};

    memcpy(out, &net, sizeof(InputPayload));
}

void unpack_input(const unsigned char buffer[512], InputPayload* payload)
{
    InputPayload net;
    memcpy(&net, payload_start(buffer), sizeof(InputPayload));

    payload->player_id = ntohl(net.player_id);
    payload->action = ntohl(net.action);
}

/* ----- STATE ----- */
void pack_state(unsigned char buffer[512], const StatePayload* payload)
{
    unsigned char* out = write_tag(buffer, PACKET_STATE);

    StatePayload net;

    // Board is a byte array, copies straight over
    memcpy(net.board, payload->board, sizeof(net.board));

    // Every remaining field is 32-bit and must be converted
    net.current_type = htonl(payload->current_type);
    net.current_rot = htonl(payload->current_rot);
    net.current_x = (int32_t)htonl((uint32_t)payload->current_x);
    net.current_y = (int32_t)htonl((uint32_t)payload->current_y);
    for (int i = 0; i < 3; i++) {
        net.preview[i] = htonl(payload->preview[i]);
    }
    net.held_type = (int32_t)htonl((uint32_t)payload->held_type);
    net.has_held = htonl(payload->has_held);
    net.player_id = htonl(payload->player_id);
    net.score = htonl(payload->score);
    net.level = htonl(payload->level);
    net.lines_cleared = htonl(payload->lines_cleared);
    net.combo = (int32_t)htonl((uint32_t)payload->combo);
    net.b2b = htonl(payload->b2b);
    net.t_spins = htonl(payload->t_spins);
    net.tetrises = htonl(payload->tetrises);
    net.pending_garbage = htonl(payload->pending_garbage);
    net.target_player_id = htonl(payload->target_player_id);
    net.target_mode = htonl(payload->target_mode);
    net.game_over = htonl(payload->game_over);
    net.winner_id = htonl(payload->winner_id);

    memcpy(out, &net, sizeof(StatePayload));
}

void unpack_state(const unsigned char buffer[512], StatePayload* payload)
{
    StatePayload net;
    memcpy(&net, payload_start(buffer), sizeof(StatePayload));

    memcpy(payload->board, net.board, sizeof(payload->board));

    payload->current_type = ntohl(net.current_type);
    payload->current_rot = ntohl(net.current_rot);
    payload->current_x = (int32_t)ntohl((uint32_t)net.current_x);
    payload->current_y = (int32_t)ntohl((uint32_t)net.current_y);
    for (int i = 0; i < 3; i++) {
        payload->preview[i] = ntohl(net.preview[i]);
    }
    payload->held_type = (int32_t)ntohl((uint32_t)net.held_type);
    payload->has_held = ntohl(net.has_held);
    payload->player_id = ntohl(net.player_id);
    payload->score = ntohl(net.score);
    payload->level = ntohl(net.level);
    payload->lines_cleared = ntohl(net.lines_cleared);
    payload->combo = (int32_t)ntohl((uint32_t)net.combo);
    payload->b2b = ntohl(net.b2b);
    payload->t_spins = ntohl(net.t_spins);
    payload->tetrises = ntohl(net.tetrises);
    payload->pending_garbage = ntohl(net.pending_garbage);
    payload->target_player_id = ntohl(net.target_player_id);
    payload->target_mode = ntohl(net.target_mode);
    payload->game_over = ntohl(net.game_over);
    payload->winner_id = ntohl(net.winner_id);
}

/* ----- GAMESTATE <-> STATEPAYLOAD ----- */
// Server side, flattens the authoritative board into something sendable
void build_state_payload(const GameState* state, StatePayload* payload)
{
    memcpy(payload->board, state->board.cells, sizeof(payload->board));

    payload->current_type = (uint32_t)state->current.type;
    payload->current_rot = (uint32_t)state->current.rot;
    payload->current_x = (int32_t)state->current.x;
    payload->current_y = (int32_t)state->current.y;

    // Pull the same 3 pieces the client's preview box will draw
    for (int i = 0; i < 3; i++) {
        payload->preview[i] = (uint32_t)state->bag[(state->bag_index + i) % 14];
    }

    payload->held_type = (int32_t)state->held_type;
    payload->has_held = (uint32_t)state->has_held;

    payload->player_id = state->player_id;
    payload->score = (uint32_t)state->score;
    payload->level = (uint32_t)state->level;
    payload->lines_cleared = (uint32_t)state->lines_cleared;
    payload->combo = (int32_t)state->combo;
    payload->b2b = (uint32_t)state->b2b;
    payload->t_spins = (uint32_t)state->t_spins;
    payload->tetrises = (uint32_t)state->tetrises;
    payload->pending_garbage = state->pending_garbage;

    payload->target_player_id = state->target_player_id;
    payload->target_mode = (uint32_t)state->target_mode;
    payload->game_over = (uint32_t)state->game_over;
    payload->winner_id = state->winner_id;
}

// Client side, copies a received snapshot into the local GameState render mirror
// Only the fields the renderer reads are touched
void apply_state_payload(const StatePayload* payload, GameState* state)
{
    memcpy(state->board.cells, payload->board, sizeof(payload->board));

    state->current.type = (PieceType)payload->current_type;
    state->current.rot = (Rotation)payload->current_rot;
    state->current.x = (int)payload->current_x;
    state->current.y = (int)payload->current_y;

    // Rebuild just enough of the bag for the preview box to read normally
    // Index is pinned to 0 so bag[0], bag[1], bag[2] line up with the 3 sent pieces
    state->bag_index = 0;
    for (int i = 0; i < 3; i++) {
        state->bag[i] = (int)payload->preview[i];
    }

    state->held_type = (int)payload->held_type;
    state->has_held = (bool)payload->has_held;

    state->player_id = payload->player_id;
    state->score = (int)payload->score;
    state->level = (int)payload->level;
    state->lines_cleared = (int)payload->lines_cleared;
    state->combo = (int)payload->combo;
    state->b2b = (bool)payload->b2b;
    state->t_spins = (int)payload->t_spins;
    state->tetrises = (int)payload->tetrises;
    state->pending_garbage = payload->pending_garbage;

    state->target_player_id = payload->target_player_id;
    state->target_mode = (TargetingMode)payload->target_mode;
    state->game_over = (int)payload->game_over;
    state->winner_id = payload->winner_id;
}
