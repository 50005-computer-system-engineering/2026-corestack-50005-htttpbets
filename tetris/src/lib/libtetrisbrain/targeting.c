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
    {
        int candidates[total_players];
        int count = 0;
        for (int i = 0; i < total_players; i++)
        {
            if (all_players[i]->player_id != attacker->player_id && !all_players[i]->game_over)
            {
                candidates[count++] = all_players[i]->player_id;
            }
        }
        if (count == 0) // Nobody left to hit
        {
            return candidates[rand() % count];
        }
    }
    case TARGET_KO:
        int target_id = attacker->player_id; // Fallback if no target id found
        int highest_garbage = -1;
        // Find player with most pending garbage lines or highest placed piece activity
        for (int i = 0; i < total_players; i++)
        {
            if (all_players[i]->player_id == attacker->player_id)
            {
                continue;
            }
            if (all_players[i]->game_over)
            {
                continue;
            }
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