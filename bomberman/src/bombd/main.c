#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>
#include <signal.h>
#include <poll.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <ifaddrs.h>
#include <net/if.h>
#include "lib/libbattleroyale/server.h"
#include "lib/libbombprotocol/protocol.h"
#include "lib/libbombbrain/map.h"
#include "lib/libbombbrain/bomb_control.h"
#include "lib/libeventbus.h"
#include "events.h"
#include "session.h"

// Keep in sync with libbattleroyale's PORT_TCP: brclient_join always talks
// to this port first and expects a redirect to the room's real port
#define MASTER_PORT 6700

// Bomberman only ever runs one shared room, so the TCP port is fixed rather
// than dynamically assigned per-room like tetrisd's worker ports
#define ROOM_TCP_PORT 6810

// MUST equal libbattleroyale's (private) PORT_UDP_UNI: brclient_join() always
// connect()'s its unicast UDP socket to that exact constant (there is no
// handshake step that tells the client a custom per-room UDP port, unlike the
// TCP port which does get redirected), so the room's real UDP listener has to
// sit on the same fixed port the client library already assumes
#define ROOM_UDP_PORT 6701

#define WAIT_POLL_MS 100

static volatile sig_atomic_t stop_requested = 0;

static void on_sigint(int sig)
{
    (void)sig;
    stop_requested = 1;
}

/* ----- SETUP HELPERS ----- */
// Selects the first non-loopback IPv4 address, for display only for others to join
static void find_lan_ip(char host_ip[INET_ADDRSTRLEN])
{
    struct ifaddrs *ifaddr, *ifa;
    if (getifaddrs(&ifaddr) != 0) {
        return;
    }
    for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next) {
        if (ifa->ifa_addr == NULL || ifa->ifa_addr->sa_family != AF_INET || (ifa->ifa_flags & IFF_LOOPBACK)) {
            continue;
        }
        struct sockaddr_in* sa = (struct sockaddr_in*)ifa->ifa_addr;
        if (inet_ntop(AF_INET, &sa->sin_addr, host_ip, INET_ADDRSTRLEN) != NULL) {
            break;
        }
    }
    freeifaddrs(ifaddr);
}

static int master_listen(void)
{
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) {
        perror("[bombd] socket");
        return -1;
    }

    int reuse = 1;
    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));

    struct sockaddr_in addr = {
        .sin_family = AF_INET,
        .sin_port = htons(MASTER_PORT),
        .sin_addr.s_addr = INADDR_ANY};
    if (bind(fd, (struct sockaddr*)&addr, sizeof(addr)) < 0 || listen(fd, 100) < 0) {
        perror("[bombd] bind/listen");
        close(fd);
        return -1;
    }
    return fd;
}

// Every client's brclient_join() expects a redirect port before it ever
// talks to the real room, even though bombd only has the one room
static void send_redirect(int client_fd)
{
    uint32_t port_bytes = htonl(ROOM_TCP_PORT);
    send(client_fd, &port_bytes, sizeof(port_bytes), MSG_NOSIGNAL); // Best effort
    close(client_fd);
}

/* ----- BROADCAST HELPERS ----- */
static void broadcast_tile_update(BRServer* server, int x, int y, TileType tile)
{
    TileUpdatePayload payload = {.x = (uint32_t)x, .y = (uint32_t)y, .tile = (uint32_t)tile};
    unsigned char buffer[512] = {0};
    pack_tile_update(buffer, &payload);
    brserver_send_to_all(server, buffer);
}

// Diffs the live map against the last-broadcast snapshot, sending only the
// tiles that actually changed (breakables destroyed, powerups consumed)
static void diff_and_broadcast_map(BRServer* server, uint8_t prev_map[BOMBD_MAX_MAP_DIM][BOMBD_MAX_MAP_DIM])
{
    for (int i = 0; i < map_size && i < BOMBD_MAX_MAP_DIM; i++) {
        for (int j = 0; j < map_size && j < BOMBD_MAX_MAP_DIM; j++) {
            uint8_t current = (uint8_t)map[i][j];
            if (current != prev_map[i][j]) {
                broadcast_tile_update(server, i, j, (TileType)current);
                prev_map[i][j] = current;
            }
        }
    }
}

static void broadcast_state(BRServer* server, const BombSession* session)
{
    StatePayload payload = {0};
    payload.player_count = (uint32_t)session->count;
    for (int i = 0; i < session->count; i++) {
        const BombPlayer* p = &session->players[i];
        PlayerStateEntry* entry = &payload.players[i];
        entry->player_id = p->player_id;
        entry->x = p->box.position.x;
        entry->y = p->box.position.y;
        entry->direction = (uint32_t)p->direction;
        entry->is_moving = p->is_moving ? 1 : 0;
        entry->alive = p->alive ? 1 : 0;
        entry->num_bombs = (uint32_t)p->inventory.num_bombs;
        entry->remaining_bombs = (uint32_t)p->inventory.remaining_bombs;
        entry->num_fires = (uint32_t)p->inventory.num_fires;
    }

    payload.bomb_count = 0;
    for (int i = bombs_queue.front + 1; i < bombs_queue.rear && payload.bomb_count < BOMBD_MAX_BOMBS; i++) {
        BombInfo* bomb = &bombs_queue.items[i];
        BombStateEntry* entry = &payload.bombs[payload.bomb_count++];
        entry->x = bomb->position.x;
        entry->y = bomb->position.y;
        entry->timer = bomb->timer;
    }

    unsigned char buffer[512] = {0};
    pack_state(buffer, &payload);
    brserver_send_to_all(server, buffer);
}

// Explosion animations are cosmetic-only, so they go out over UDP broadcast
static void on_explosion_triggered(const ExplosionInfo* explosion, void* user_data)
{
    BRServer* server = (BRServer*)user_data;

    ExplosionPayload payload;
    payload.center_x = explosion->center.x;
    payload.center_y = explosion->center.y;
    for (int i = 0; i < 4; i++) {
        payload.spread_amt[i] = (uint32_t)explosion->spread_amt[i];
    }

    unsigned char buffer[512] = {0};
    pack_explosion(buffer, &payload);
    brserver_send_broadcast(server, buffer);
}

/* ----- MAIN ----- */
int main(void)
{
    setvbuf(stdout, NULL, _IOLBF, 0);
    signal(SIGPIPE, SIG_IGN);
    signal(SIGINT, on_sigint);

    char host_ip[INET_ADDRSTRLEN] = {0};
    find_lan_ip(host_ip);
    if (host_ip[0] != '\0') {
        printf("[bombd] Players on this network should enter: %s\n", host_ip);
    } else {
        printf("[bombd] No non-loopback IPv4 found, local play only (127.0.0.1).\n");
    }
    printf("[bombd] Players on this machine can use the default 127.0.0.1\n");

    // libbombbrain's trigger_explosion() (called from tick_bombs()) fires
    // EVENT_BOMB_EXPLODED unconditionally. bombd never listens for it (it
    // reports explosions to clients via its own ExplosionCallback instead),
    // but the bus still has to exist or event_bus_trigger() dereferences a
    // NULL slots array and crashes the moment the first bomb goes off
    event_bus_init(EVENT_COUNT);

    int listen_fd = master_listen();
    if (listen_fd < 0) {
        fprintf(stderr, "[bombd] Could not create master listener!\n");
        return -1;
    }

    // Bring the single room up before accepting any redirects to it
    BRServer* server = NULL;
    if (brserver_init_room(&server, ROOM_TCP_PORT, ROOM_UDP_PORT, 1) < 0) {
        fprintf(stderr, "[bombd] Could not bind room TCP %u / UDP %u.\n", ROOM_TCP_PORT, ROOM_UDP_PORT);
        close(listen_fd);
        return -1;
    }
    if (brserver_open(server) < 0) {
        fprintf(stderr, "[bombd] Could not open lobby!\n");
        close(listen_fd);
        return -1;
    }

    printf("[bombd] Master listening on port %d, room open on TCP %d / UDP %d.\n", MASTER_PORT, ROOM_TCP_PORT, ROOM_UDP_PORT);
    printf("[bombd] Waiting for players (up to %d). Press ENTER to start early.\n", BOMBD_MAX_PLAYERS);

    /* ----- LOBBY ----- */
    uint32_t lobby_size = 0;
    uint32_t client_ids[BOMBD_MAX_PLAYERS] = {0};
    bool stdin_open = true;
    bool start_requested = false;

    while (!stop_requested && !start_requested) {
        struct pollfd pfds[2] = {
            {.fd = listen_fd, .events = POLLIN, .revents = 0},
            {.fd = STDIN_FILENO, .events = POLLIN, .revents = 0}};
        int nfds = stdin_open ? 2 : 1;

        if (poll(pfds, (nfds_t)nfds, WAIT_POLL_MS) < 0) {
            continue;
        }

        if (pfds[0].revents & POLLIN) {
            int client_fd = accept(listen_fd, NULL, NULL);
            if (client_fd >= 0) {
                send_redirect(client_fd);
            }
        }

        if (stdin_open && (pfds[1].revents & POLLIN)) {
            char input[64];
            ssize_t got = read(STDIN_FILENO, input, sizeof(input));
            if (got <= 0) {
                stdin_open = false;
            } else if (memchr(input, '\n', (size_t)got) != NULL) {
                start_requested = true;
            }
        }

        brserver_client_info(server, &lobby_size, client_ids);
        if (lobby_size >= BOMBD_MAX_PLAYERS) {
            start_requested = true; // Full lobby, no need to wait for ENTER
        }
    }
    close(listen_fd); // No more joins accepted once we leave the lobby loop

    if (stop_requested) {
        printf("[bombd] Shutting down before match start.\n");
        return 0;
    }

    brserver_client_info(server, &lobby_size, client_ids);
    if (lobby_size == 0) {
        fprintf(stderr, "[bombd] No players joined, exiting.\n");
        return -1;
    }
    if (brserver_start(server) < 0) {
        fprintf(stderr, "[bombd] Failed to start match!\n");
        return -1;
    }
    printf("[bombd] Match starting with %u player(s).\n", lobby_size);

    // brserver_start() only flips a state flag; libbattleroyale's background
    // thread notices it on its own ~50ms poll cycle and is the one that
    // actually sends MSG_START. A client still in LOBBY silently drops any
    // message that isn't MSG_START, so our own roster/map broadcasts below
    // (sent from this thread, with no synchronization against that one) must
    // not race ahead of it
    usleep(150000);

    /* ----- SESSION SETUP ----- */
    BombSession session;
    init_session(&session, client_ids, lobby_size);

    RosterPayload roster = {0};
    roster.count = lobby_size;
    for (uint32_t i = 0; i < lobby_size && i < BOMBD_MAX_PLAYERS; i++) {
        roster.ids[i] = client_ids[i];
    }
    unsigned char roster_buffer[512] = {0};
    pack_roster(roster_buffer, &roster);
    brserver_send_to_all(server, roster_buffer);

    MapInitPayload map_init = {0};
    map_init.map_size = (uint32_t)map_size;
    uint8_t prev_map[BOMBD_MAX_MAP_DIM][BOMBD_MAX_MAP_DIM] = {0};
    for (int i = 0; i < map_size && i < BOMBD_MAX_MAP_DIM; i++) {
        for (int j = 0; j < map_size && j < BOMBD_MAX_MAP_DIM; j++) {
            map_init.tiles[i][j] = (uint8_t)map[i][j];
            prev_map[i][j] = (uint8_t)map[i][j];
        }
    }
    unsigned char map_buffer[512] = {0};
    pack_map_init(map_buffer, &map_init);
    brserver_send_to_all(server, map_buffer);

    /* ----- AUTHORITATIVE TICK LOOP ----- */
    unsigned char msg_buffer[1024] = {0};
    bool match_over = false;
    uint32_t winner_id = 0;
    float delta_time = (float)TICK_MICROSECONDS / 1000000.0f;

    while (!match_over && !stop_requested) {
        // (1) Drain whatever clients sent since the last tick. Bounded so a
        // flood of packets can never starve the simulation
        for (int drained = 0; drained < MAX_MSGS_PER_TICK; drained++) {
            if (brserver_get_app_msg(msg_buffer) != 1) {
                break;
            }

            uint32_t tag = read_bomb_packet_tag(msg_buffer);
            if (tag == PACKET_MOVE) {
                MovePayload move;
                unpack_move(msg_buffer, &move);
                apply_move(find_bomb_player(&session, move.player_id), &move);
            } else if (tag == PACKET_BOMB_PLACE) {
                BombPlacePayload bomb_place;
                unpack_bomb_place(msg_buffer, &bomb_place);
                apply_bomb_place(find_bomb_player(&session, bomb_place.player_id));
            }
        }

        // (2) Advance the shared simulation by one tick
        tick_session(&session, delta_time, on_explosion_triggered, server);

        // (3) Broadcast whatever changed: tile updates are reliable (TCP),
        // player/bomb state is reliable, explosions already went out (UDP).
        // A ticking bomb's timer alone doesn't mark the session dirty (only
        // movement/pickups/deaths do), so force a resend while any bomb is
        // armed or the client's red-tint countdown would freeze between moves
        diff_and_broadcast_map(server, prev_map);
        if (!Bombs_empty(&bombs_queue)) {
            session.dirty = true;
        }
        if (session.dirty) {
            broadcast_state(server, &session);
            session.dirty = false;
        }

        // (4) Check for match end: last one standing wins, or a lone player dying ends it
        int alive = count_alive_players(&session);
        if (session.count > 1 && alive <= 1) {
            match_over = true;
            for (int i = 0; i < session.count; i++) {
                if (session.players[i].alive) {
                    winner_id = session.players[i].player_id;
                    break;
                }
            }
        } else if (session.count == 1 && alive == 0) {
            match_over = true; // Solo player died
        }

        usleep(TICK_MICROSECONDS);
    }

    /* ----- MATCH OVER ----- */
    GameOverPayload over = {.winner_id = winner_id};
    unsigned char over_buffer[512] = {0};
    pack_game_over(over_buffer, &over);
    brserver_send_to_all(server, over_buffer);

    if (winner_id != 0) {
        printf("\n[bombd] MATCH OVER! Winner: P%u\n", winner_id);
    } else {
        printf("\n[bombd] MATCH OVER! No survivors.\n");
    }

    usleep(500000); // Give the final broadcast a moment to land before tearing down
    free_session(&session);
    if (!stop_requested) {
        brserver_end(server);
    }
    event_bus_free();
    return 0;
}
