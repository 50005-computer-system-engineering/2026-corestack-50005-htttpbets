#ifndef CONFIG_H
#define CONFIG_H

// Controls
typedef struct {
    const int MOVE_UP;
    const int MOVE_DOWN;
    const int MOVE_LEFT;
    const int MOVE_RIGHT;
    const int BOMB;
    const int EXIT;
} Keybindings;

// TODO: Add more config types here!

typedef struct {
    Keybindings KEYS;
} Config;

// Single global instance
extern const Config CONFIG;
#endif