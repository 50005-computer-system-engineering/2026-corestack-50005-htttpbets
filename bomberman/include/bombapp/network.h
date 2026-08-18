#ifndef NETWORK_H
#define NETWORK_H
#include <stdint.h>
#include <stdbool.h>
#include "bomberman.h"

#define NETWORK_LOBBY_WAIT_TIMEOUT_MS (300 * 1000)
#define NETWORK_MAP_WAIT_TIMEOUT_MS (5 * 1000)

// Connects to bombd at server_ip and blocks until the match starts and this
// player's server-assigned ID is known. Returns false on any failure
bool network_connect(const char* server_ip);

// Blocks (bounded) until the server's starting map has arrived, so the
// camera/renderer never sizes itself against the throwaway local map.
// Call after network_connect() succeeds
bool network_wait_for_map(void);

// Reads held movement/bomb keys and sends the appropriate packet: UDP for
// movement (frequent, loss-tolerant), TCP for bomb placement (must land).
// While the local player is dead, this instead flies a free-roaming
// spectator camera around the map with the same keys and sends nothing
void network_send_input(void);

// Drains incoming packets and applies them to the shared render state
void network_poll(void);

// Advances the purely-cosmetic client-side explosion animation timers.
// Never touches the map: only libbombbrain's authoritative tick_explosions()
// (server-side) does that, via TILE_UPDATE packets instead
void network_tick_explosions(float delta_time);

uint32_t network_local_player_id(void);
int network_local_slot(void); // Index into network_get_player()/ids for the local player, or -1
int network_player_count(void);
Bomberman* network_get_player(int index);
uint32_t network_get_player_id(int index);
bool network_is_player_alive(int index);
bool network_game_over(void);
uint32_t network_winner_id(void);

// What the camera should be looking at this frame: the local player's real
// position while alive, or the free-flying spectator position once dead
Vector2 network_get_camera_target(void);

#endif
