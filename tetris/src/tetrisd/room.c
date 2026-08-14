#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdarg.h>
#include <unistd.h>
#include <time.h>
#include "lib/libbattleroyale/server.h"
#include "lib/libtetrisprotocol/protocol.h"
#include "lib/libtetrisbrain/garbage.h"
#include "lib/libtetrislog/logclient.h"
#include "game.h"
#include "hub.h"
#include "room.h"

// How long a started room waits for redirected stragglers before playing on
// TODO: Move to tetrisrc
#define START_GRACE_SECONDS 3

static LogClient room_log;
static int room_no;

// Logs a message to the room's log, then forwards it to the master
static void room_log_msg(LogLevel level, const char* fmt, ...)
{
    char message[LOG_MSG_LENGTH];
    va_list args;
    va_start(args, fmt);
    vsnprintf(message, sizeof(message), fmt, args);
    va_end(args);

    printf("%s\n", message);
    log_client_push(&room_log, level, message); // Forwarded, non-blocking, may be dropped
}

// Broadcasts one attack to this room's kill feeds
static void broadcast_feed(BRServer* server, uint32_t source, uint32_t target, uint32_t lines)
{
    AttackPayload feed = {
        .source_player = source,
        .target_player = target,
        .lines = lines
    };

    unsigned char feed_buffer[512] = {0};
    pack_attack(feed_buffer, &feed);
    brserver_send_to_all(server, feed_buffer);
}

// Garbage forwarded by the master from another room lands here
static void apply_remote_garbage(BRServer* server, GameSession* session, const HubAttack* attack)
{
    PlayerSlot* victim = find_player(session, attack->victim_id);
    // Check if victim died in the meantime between when this was sent out and now
    // If so, ignore!
    if (victim == NULL || victim->state.game_over) {
        return;
    }

    // Add garbage to the victim's board
    queue_garbage(&victim->state, (int)attack->lines);
    victim->state.last_attacker_id = attack->attacker_id;
    victim->dirty = true;

    // Send log message to the room
    room_log_msg(LOG_LEVEL_INFO, " <!> [room %d] incoming: P%u attacked P%u with %u lines!", room_no, attack->attacker_id, attack->victim_id, attack->lines);
    broadcast_feed(server, attack->attacker_id, attack->victim_id, attack->lines);
}

// Picks who an attack lands on. Random draws from every alive player in the
// whole match and KO retaliates across rooms. Everything else stays local.
static uint32_t resolve_victim(GameSession* session, PlayerSlot* attacker, const HubRoster* global)
{
    GameState* state = &attacker->state;

    if (global->count > 0) {
        if (state->target_mode == TARGET_RANDOM) {
            uint32_t candidates[HUB_MAX_PLAYERS];
            uint32_t count = 0;

            // Discover available candidates who are 1) alive & 2) not the attacker
            for (uint32_t i = 0; i < global->count && i < HUB_MAX_PLAYERS; i++) {
                if (global->ids[i] != attacker->player_id && global->alive[i]) {
                    candidates[count++] = global->ids[i];
                }
            }

            // If candidates exist, pick one at random
            if (count > 0)
                return candidates[(uint32_t)rand() % count];
        }

        // If KO retaliation is enabled, and the last attacker is still alive, retaliate
        if (state->target_mode == TARGET_KO && state->last_attacker_id != 0 && state->last_attacker_id != attacker->player_id) {
            for (uint32_t i = 0; i < global->count && i < HUB_MAX_PLAYERS; i++) {
                if (global->ids[i] == state->last_attacker_id && global->alive[i])
                    return state->last_attacker_id;
            }
        }
    }

    // Manual targets and every fallback resolve against the room roster
    return resolve_target_id(state, &session->roster);
}

void room_worker_run(int hub_fd, uint16_t tcp_port, uint32_t id_base, int room_index)
{
    room_no = room_index;

    // Get unique piece bags per room even if the workers all get forked at nearly the same moment
    // XOR'd with the PID so the same time will always get different seeds across all workers!
    srand((unsigned)(time(NULL) ^ (unsigned)getpid()));


    // Initialize the room's log client
    char log_name[32];
    snprintf(log_name, sizeof(log_name), "tetrisd-r%d", room_index);
    if (!log_client_init(&room_log, LOG_DEFAULT_IPC_PATH, log_name)) {
        printf("[room %d] Warning: could not set up logging client, continuing without it.\n", room_index);
    }

    // Initialize the room's server
    BRServer* server = NULL;
    if (brserver_init_room(&server, tcp_port, id_base) < 0) {
        room_log_msg(LOG_LEVEL_ERROR, "[room %d] Could not bind port %u, giving up.", room_index, tcp_port);
        exit(1);
    }
    if (brserver_open(server) < 0) {
        room_log_msg(LOG_LEVEL_ERROR, "[room %d] Could not open lobby!", room_index);
        exit(1);
    }

    // The master parks redirects until this arrives, so the bind must come first
    // [Race Condition Fix!]: There's a window where the worker process exists but isn't listening yet on spawn!
    // (need to call bind() on its own TCP port before it can listen/accept any connections), so clients might join during this!!
    // So, prevent master from sending anyone to this port until the port is ready, since hub_send only happens after room init
    // @see main.c/accept_client()
    HubMsg ready = {.type = HUB_READY};
    hub_send(hub_fd, &ready);
    room_log_msg(LOG_LEVEL_INFO, "[room %d] Lobby open on port %u.", room_index, tcp_port);

    // TODO: Separate into multiple functions
    #pragma region Lobby
    // Report every arrival, wait for the master's start order */
    uint32_t lobby_size = 0;
    uint32_t client_ids[MAX_LOBBY_SIZE] = {0};
    uint32_t reported = 0; // How many clients have reported in
    uint32_t expected = 0; // How many clients are expected to join (while the master is waiting)
    bool start_ordered = false; // Whether the master has ordered the start of the match
    time_t order_time = 0; // When the master ordered the start of the match, used for grace window
    HubRoster global = {0}; // The global roster of all players in the lobby

    while (1) {
        brserver_client_info(server, &lobby_size, client_ids);
        for (; reported < lobby_size; reported++) {
            HubMsg joined = {.type = HUB_JOINED, .player_id = client_ids[reported]};
            hub_send(hub_fd, &joined);
            room_log_msg(LOG_LEVEL_INFO, "[room %d] P%u joined (%u in room).", room_index, client_ids[reported], reported + 1);
        }

        HubMsg msg;
        int got;
        while ((got = hub_recv(hub_fd, &msg)) == 1) {
            if (msg.type == HUB_START) {
                start_ordered = true;
                expected = msg.expected_players;
                order_time = time(NULL);
            } else if (msg.type == HUB_ROSTER) {
                global = msg.roster;
            }
        }
        if (got < 0) {
            exit(0); // Master is gone, nobody left to play for
        }

        // Redirected stragglers get a grace window, then the match starts without them
        if (start_ordered && (lobby_size >= expected || time(NULL) - order_time >= START_GRACE_SECONDS)) {
            break;
        }
        usleep(100000);
    }

    if (lobby_size == 0 || brserver_start(server) < 0) {
        room_log_msg(LOG_LEVEL_ERROR, "[room %d] Failed to start game!", room_index);
        exit(1);
    }

    // Tell every client who they share this room with
    RosterPayload roster = {0};
    roster.count = lobby_size;
    for (uint32_t i = 0; i < lobby_size && i < MAX_LOBBY_SIZE; i++) {
        roster.ids[i] = client_ids[i];
    }
    unsigned char roster_buffer[512] = {0};
    pack_roster(roster_buffer, &roster);
    if (brserver_send_to_all(server, roster_buffer) < 0) {
        room_log_msg(LOG_LEVEL_WARN, "[room %d] Warning: failed to broadcast player roster.", room_index);
    }

    // Build the authoritative match, one real GameState per connected player
    GameSession session;
    init_session(&session, client_ids, lobby_size);
    room_log_msg(LOG_LEVEL_INFO, "[room %d] Match started with %d player(s).", room_index, session.count);
    #pragma endregion Lobby

    #pragma region Authoritative Tick Loop
    // Non-blocking, runs while anyone in this room is alive
    unsigned char buffer[1024] = {0};
    bool match_over = false;
    uint32_t winner_id = 0;

    while (!match_over) {
        // (1) Drain whatever the clients sent since the last tick
        // Bounded to max_msgs so a flood of packets can never starve the simulation below!
        // => prevent frozen/malicious clients from spamming
        for (int drained = 0; drained < MAX_MSGS_PER_TICK; drained++) {
            if (brserver_get_app_msg(buffer) != 1) {
                break; // Queue is empty
            }

            // Read the tag so the receiver knows what to do with the packet
            uint32_t tag = read_packet_tag(buffer);
            if (tag == PACKET_INPUT) {
                InputPayload input;
                unpack_input(buffer, &input);

                // Perform the requested action on that player's real board
                PlayerSlot* slot = find_player(&session, input.player_id);
                apply_action(&session, slot, (PlayerAction)input.action);
            }
            // Any other tag is ignored
        }

        // (2) Drain the hub: incoming garbage, roster refreshes, global win/lose state
        HubMsg msg;
        int got;
        while ((got = hub_recv(hub_fd, &msg)) == 1) {
            switch (msg.type) {
            case HUB_GARBAGE:
                apply_remote_garbage(server, &session, &msg.attack);
                break;
            case HUB_ROSTER:
                global = msg.roster;
                break;
            case HUB_GAMEOVER:
                winner_id = msg.winner_id;
                match_over = true;
                break;
            default:
                break;
            }
        }

        // Master died, wind the room down cleanly
        if (got < 0)
            match_over = true;

        // Get out of loop
        if (match_over)
            break;

        // Advance every board by one tick + log
        tick_session(&session);
        log_client_drain(&room_log, LOG_CLIENT_DRAIN_BATCH);

        // Report eliminations to the master exactly once per player
        for (int i = 0; i < session.count; i++) {
            PlayerSlot* slot = &session.players[i];
            if ((slot->state.game_over || !slot->active) && !slot->elim_reported) {
                slot->elim_reported = true;
                room_log_msg(LOG_LEVEL_INFO, "[room %d] P%u has been eliminated.", room_index, slot->player_id);

                HubMsg elim = {.type = HUB_ELIMINATED, .player_id = slot->player_id};
                hub_send(hub_fd, &elim);
            }
        }

        // Route any garbage the tick produced, locally or across rooms
        for (int i = 0; i < session.count; i++) {
            PlayerSlot* attacker = &session.players[i];

            // Not sending any garbage, skip
            if (attacker->state.outgoing_garbage == 0)
                continue; 

            uint32_t lines = attacker->state.outgoing_garbage;
            attacker->state.outgoing_garbage = 0; // Consume garbage

            // Resolve the victim using the server's roster
            uint32_t victim_id = resolve_victim(&session, attacker, &global);
            PlayerSlot* victim = find_player(&session, victim_id);

            // (1) Victim found and in this room! Apply direction
            if (victim != NULL && victim != attacker && !victim->state.game_over) {
                queue_garbage(&victim->state, (int)lines);
                victim->state.last_attacker_id = attacker->player_id;
                victim->dirty = true;

                room_log_msg(LOG_LEVEL_INFO, " <!> [room %d] EVENT ROUTED: P%u attacked P%u with %u lines!", room_index, attacker->player_id, victim_id, lines);
                broadcast_feed(server, attacker->player_id, victim_id, lines);
            }
            // (2) Victim not found, but victim_id is valid! Send over to the master
            else if (victim == NULL && victim_id != 0 && victim_id != attacker->player_id) {
                HubMsg attack = {.type = HUB_ATTACK, .attack = {.attacker_id = attacker->player_id, .victim_id = victim_id, .lines = lines}};
                hub_send(hub_fd, &attack);

                room_log_msg(LOG_LEVEL_INFO, " <!> [room %d] outgoing: P%u attacked P%u with %u lines!", room_index, attacker->player_id, victim_id, lines);
                broadcast_feed(server, attacker->player_id, victim_id, lines);
            }
            // Anything else (self, dead local victim) is dropped
        }

        // Push state to anyone whose board changed
        // Dirty flag keeps traffic low by only sending on changed state, 
        // keepalive repairs missed updates from UDP
        for (int i = 0; i < session.count; i++) {
            PlayerSlot* slot = &session.players[i];

            // Nothing new to say about this player yet
            // We sent only if (1) Something changed or (2) It's been too long since we sent
            if (!slot->dirty && slot->idle_ticks < KEEPALIVE_TICKS) {
                slot->idle_ticks++;
                continue; // Skip sending!
            }

            // Reduces overall Gamestate into a smaller snapshot of relevant info 
            // (eg: only preview pieces shown, etc.)
            StatePayload snapshot;
            build_state_payload(&slot->state, &snapshot);

            // Serialise payload and transmit those bytes to all clients
            unsigned char state_buffer[512] = {0};
            pack_state(state_buffer, &snapshot);
            brserver_send_to_all(server, state_buffer);

            // Reset flag and idle timer
            slot->dirty = false;
            slot->idle_ticks = 0;
        }
        
        usleep(TICK_MICROSECONDS); // Advance at a fixed rate
    }
    #pragma endregion Authoritative Tick Loop

    #pragma region Room Over
    // Mark everyone finished and push one final state each so clients exit cleanly
    for (int i = 0; i < session.count; i++) {
        PlayerSlot* slot = &session.players[i];

        if (!slot->active)
            continue; // Already gone, nothing listening

        // Update winner
        slot->state.game_over = true;
        slot->state.winner_id = winner_id;

        // Send the end game state to everyone
        StatePayload snapshot;
        build_state_payload(&slot->state, &snapshot);

        unsigned char state_buffer[512] = {0};
        pack_state(state_buffer, &snapshot);
        brserver_send_to_all(server, state_buffer);
    }

    // Show relevant winning message
    if (winner_id != 0)
        room_log_msg(LOG_LEVEL_INFO, "[room %d] Match over, winner is P%u.", room_index, winner_id);
    else
        room_log_msg(LOG_LEVEL_INFO, "[room %d] Room finished with no local survivors.", room_index);

    log_client_drain(&room_log, LOG_CLIENT_DRAIN_BATCH);
    log_client_close(&room_log);

    // Give the final broadcast a moment to land before tearing down the sockets
    usleep(500000); // 500ms

    brserver_end(server);
    exit(0); // Hub fd closes here, which is how the master learns the room ended
    #pragma endregion Room Over
}
