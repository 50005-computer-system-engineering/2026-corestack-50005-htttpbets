#ifndef TETRISBRAIN_TARGETING_H
#define TETRISBRAIN_TARGETING_H

#include "state.h"

// Resolve and return correct target ID based on attacker's mode
int resolveTargetID(GameState *attacker, GameState *all_players[], int total_players);

// Cycle target mode selection
void cycleTargetMode(GameState *player);

#endif