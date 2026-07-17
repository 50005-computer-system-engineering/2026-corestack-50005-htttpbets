/* FOR HANDLING USER INPUT; TERMINAL MANIPULATION AND NON-BLOCKING I/O */

#ifndef TETRISU_INPUT_H
#define TETRISU_INPUT_H

// Invoke terminal settings
void enableRawMode();

// Reset terminal back to normal; if not will remain broken
void disableRawMode(void);

// Checks if a key has been pressed (Non-blocking)
int kbhit(void);

#endif