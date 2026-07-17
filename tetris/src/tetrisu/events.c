// TODO: CHANGE THIS TO HANDLE MORE FUNCTIONALITY + ONE PLAYER ONLY

#include "tetrisu/events.h"
#include "tetrisu.h"

// Spawn a game state => shifted to global for testing event routing
extern GameState player1;
extern GameState player2;

// Test Network Routing
void on_attack_generated(void *args)
{
    // Arm the payload
    AttackPayload *attack = (AttackPayload *) args;

    // Send payload
    if (attack->target_player == 1)
    {
        player1.pending_garbage += attack->lines;
    } else if (attack->target_player == 2)
    {
        player2.pending_garbage += attack->lines;
    }
}    