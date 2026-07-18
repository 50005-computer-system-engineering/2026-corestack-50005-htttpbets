#include <stdlib.h>
#include "targeting.h"

// Cycle target mode selection
void cycleTargetMode(GameState *player)
{
    player->target_mode = (player->target_mode + 1) % TARGET_MODE_COUNT;
}

// Resolve and return correct target ID based on attacker's mode
int resolveTargetID(GameState *attacker, GameState *all_players[], int total_players)
{
    switch (attacker->target_mode)
    {
    case TARGET_RANDOM:
        int random_target;
        do
        {
            random_target = (rand() % total_players) + 1;
        } while (random_target == attacker->player_id); // To prevent selecting ownself
        return random_target;
    case TARGET_KO:
        int target_id = 3 - attacker->player_id; // Default
        int highest_garbage = -1;
        // Find player with most pending garbage lines or highest placed piece activity
        for (int i = 0; i < total_players; i++)
        {
            if (all_players[i]->player_id == attacker->player_id)
                continue;

            // Target the player closest to topping out
            if (all_players[i]->pending_garbage > highest_garbage && !all_players[i]->game_over)
            {
                highest_garbage = all_players[i]->pending_garbage;
                target_id = all_players[i]->player_id;
            }
        }
        return target_id;
    case TARGET_MANUAL:
    default:
        // Use manually locked target selection
        return attacker->target_player_id;
    }
}