#ifndef TETRISBRAIN_TARGETING_H
#define TETRISBRAIN_TARGETING_H

#include "state.h"

// Resolve and return correct target ID based on attacker's mode
uint32_t resolveTargetID(GameState *attacker, GameState *all_players[], int total_players);

// Cycle target mode selection
void cycleTargetMode(GameState *player);

// Cycle manual target selection
void cycleManualTarget(GameState *attacker, GameState *all_players[], int total_players);

#endif