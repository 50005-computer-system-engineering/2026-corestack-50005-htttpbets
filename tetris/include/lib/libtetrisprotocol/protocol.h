#ifndef TETRISPROTOCOL_PROTOCOL_H
#define TETRISPROTOCOL_PROTOCOL_H

#include <stdint.h>
#include <stdbool.h>
#include "lib/libtetrisbrain/state.h"

/* ----- SHARED LIMITS ----- */
#define MAX_LOBBY_SIZE 16 // TO BE KEPT IN SYNC with libtetrisbrain's MAX_LOBBY_PLAYERS

/* ----- PACKET TAGS ----- */
// Every packet on the app-message channel opens with one of these as a 4-byte
// tag (network byte order) so the receiver knows how to read the bytes after it
typedef enum
{
    PACKET_ATTACK = 0, // Client -> Server, carries AttackPayload
    PACKET_ROSTER = 1, // Server -> Client, carries RosterPayload
    PACKET_INPUT = 2,  // Client -> Server, carries InputPayload
    PACKET_STATE = 3   // Server -> Client, carries StatePayload (one packet per player)
} PacketType;

/* ----- PLAYER ACTIONS ----- */
// Every key press the client can ask the server to perform on its behalf
typedef enum
{
    ACTION_NONE = 0,
    ACTION_MOVE_LEFT,
    ACTION_MOVE_RIGHT,
    ACTION_ROTATE_CW,
    ACTION_ROTATE_CCW,
    ACTION_SOFT_DROP,
    ACTION_HARD_DROP,
    ACTION_HOLD,
    ACTION_CYCLE_TARGET_MODE, // 'T' key
    ACTION_CYCLE_TARGET,      // 'R' key
    ACTION_QUIT               // 'Q' key
} PlayerAction;

/* ----- PAYLOADS ----- */
// Attack (garbage) sent from one player to another
typedef struct
{
    uint32_t source_player; // Player that sent it
    uint32_t target_player; // Player that recieves it
    uint32_t lines;         // How much damage
} AttackPayload;

// Broadcast once at game start so every client learns who else is in the lobby
typedef struct
{
    uint32_t count;               // Determined lobby size
    uint32_t ids[MAX_LOBBY_SIZE]; // Connected players
} RosterPayload;

// A single requested action from one client
typedef struct
{
    uint32_t player_id; // Who is asking
    uint32_t action;    // Which PlayerAction is being requested
} InputPayload;

// Authoritative snapshot of ONE player's board, pushed by the server every tick
typedef struct
{
    uint8_t board[BOARD_HEIGHT][BOARD_WIDTH]; // The locked-in playfield

    // Active pieceS (each field converted individually)
    uint32_t current_type;
    uint32_t current_rot;
    int32_t current_x;
    int32_t current_y;
    uint32_t next_type;

    // The 3 upcoming pieces the preview box renders
    uint32_t preview[3];

    // Hold box
    int32_t held_type;
    uint32_t has_held;

    // Dashboard counters
    uint32_t player_id;
    uint32_t score;
    uint32_t level;
    uint32_t lines_cleared;
    int32_t combo;
    uint32_t b2b;
    uint32_t t_spins;
    uint32_t tetrises;
    uint32_t pending_garbage;

    // Targeting display
    uint32_t target_player_id;
    uint32_t target_mode;

    uint32_t game_over;
} StatePayload;

/* ----- PACK / UNPACK ----- */
// Single place where the 4-byte tag and byte-order conversion happen
void packAttack(unsigned char buffer[512], const AttackPayload *payload);
void unpackAttack(const unsigned char buffer[512], AttackPayload *payload);

void packRoster(unsigned char buffer[512], const RosterPayload *payload);
void unpackRoster(const unsigned char buffer[512], RosterPayload *payload);

void packInput(unsigned char buffer[512], const InputPayload *payload);
void unpackInput(const unsigned char buffer[512], InputPayload *payload);

void packState(unsigned char buffer[512], const StatePayload *payload);
void unpackState(const unsigned char buffer[512], StatePayload *payload);

// Reads just the tag so a receiver can decide which unpack to call
uint32_t readPacketTag(const unsigned char buffer[512]);

/* ----- GAMESTATE <-> STATEPAYLOAD ----- */
// Server side: flatten its authoritative GameState into a sendable snapshot
void buildStatePayload(const GameState *state, StatePayload *payload);

// Client side: copy a received snapshot into the local GameState render mirror
void applyStatePayload(const StatePayload *payload, GameState *state);

#endif