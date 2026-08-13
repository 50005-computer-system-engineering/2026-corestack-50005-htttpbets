#ifndef TETRISBRAIN_GARBAGE_H
#define TETRISBRAIN_GARBAGE_H

#include "state.h"

// Inject garbage lines at the bottom of the board
void addGarbage(GameState *state, int lines);

// Queue incoming garbage so it appears in the pending meter and is applied later
void queueGarbage(GameState *state, int lines);

// Calculate garbage based on lines cleared (following TETR.IO guideline rules)
int calculateGarbage(GameState *state, int lines_cleared, bool is_t_spin);

#endif