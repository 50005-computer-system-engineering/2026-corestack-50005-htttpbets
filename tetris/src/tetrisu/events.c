// TODO: CHANGE THIS TO HANDLE MORE FUNCTIONALITY + ONE PLAYER ONLY

#include "events.h"
#include "lib/libtetrisbrain/state.h"

// Test Network Routing
void on_attack_generated(void *args)
{
    // Arm the payload
    AttackPayload *attack = (AttackPayload *) args;

    // Send payload
    if (attack->target_player == 1)
    {
        gamestate_p1.pending_garbage += attack->lines;
    } else if (attack->target_player == 2)
    {
        gamestate_p2.pending_garbage += attack->lines;
    }
}    