#include <raylib.h>
#include <stdio.h>
#include "lib/libeventbus.h"
#include "config.h"
#include "input.h"
#include "events.h"

void input_update(void)
{
    // If either movement keys are pressed, we want to broadcast movement state
    if (IsKeyDown(CONFIG.KEYS.MOVE_UP) || IsKeyDown(CONFIG.KEYS.MOVE_DOWN) || IsKeyDown(CONFIG.KEYS.MOVE_LEFT) || IsKeyDown(CONFIG.KEYS.MOVE_RIGHT)) {
        // Calculate movement direction, this allows players to move diagonally!
        MoveEventArgs args = {
            .player_id = 1,
            .x = IsKeyDown(CONFIG.KEYS.MOVE_RIGHT) ? 1 : IsKeyDown(CONFIG.KEYS.MOVE_LEFT) ? -1 :
                                                                                            0,
            .y = IsKeyDown(CONFIG.KEYS.MOVE_UP) ? 1 : IsKeyDown(CONFIG.KEYS.MOVE_DOWN) ? -1 :
                                                                                         0};

        // Broadcast movement
        event_bus_trigger(EVENT_INPUT_MOVE_PERFORMING, &args);

    } else {
        // Release movement state
        event_bus_trigger(EVENT_INPUT_MOVE_RELEASED, NULL);
    }

    // Sprint toggle
    if (IsKeyPressed(CONFIG.KEYS.SPRINT))
        event_bus_trigger(EVENT_INPUT_SPRINT_CHANGED, &(SprintEventArgs){.toggled = true});
    else if (IsKeyUp(CONFIG.KEYS.SPRINT))
        event_bus_trigger(EVENT_INPUT_SPRINT_CHANGED, &(SprintEventArgs){.toggled = false});

    // Bomb Placement
    if (IsKeyPressed(CONFIG.KEYS.BOMB))
        event_bus_trigger(EVENT_INPUT_BOMB_PRESSED, NULL);

    // Exit Pressed
    // TODO: Gate for Host vs Client
    if (IsKeyPressed(CONFIG.KEYS.EXIT))
        event_bus_trigger(EVENT_INPUT_EXIT_PRESSED, NULL);
}
