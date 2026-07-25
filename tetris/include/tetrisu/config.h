#ifndef TETRISU_CONFIG_H
#define TETRISU_CONFIG_H

#include <stdint.h>

// Timings to make the game more playable
extern const int GRAVITY_THRESHOLD_START; // How many loop cycles before the piece drops 1 row
extern const int LOCK_THRESHOLD_START;
extern const int DELAY_MICROSECONDS;

#endif