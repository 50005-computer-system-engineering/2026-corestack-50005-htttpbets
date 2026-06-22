#ifndef TETRISU_H
#define TETRISU_H

#include "tetrisbrain.h"

/* ----- NCURSES ENVIRONMENT ----- */
void setup();

/* ----- UI RENDERING ----- */
// Renders the board and active piece to the terminal
void drawBoard(GameState *state);

#endif