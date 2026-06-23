#ifndef TETRISU_H
#define TETRISU_H

#include "tetrisbrain.h"

/* ----- EVERYTHING ELSE ----- */
// Linux in-built terminal flags
struct termios orig_termios;

// Reset terminal back to normal; if not will remain broken
void disableRawMode(void);

// Invoke terminal settings
void enableRawMode();

// Checks if a key has been pressed (Non-blocking)
int kbhit(void);

// Wrapper to grab the character (input) from stdin once we know it's there
int getch(void);

/* ----- UI RENDERING ----- */
// Renders the board and active piece to the terminal
void drawBoard(GameState *state);

#endif