#include <stdlib.h>
#include "targeting.h"

// Cycle target mode selection
void cycleTargetMode(GameState *player)
{
    player->target_mode = (player->target_mode + 1) % TARGET_MODE_COUNT;
}

// Cycle manual target selection
// Find current target in the lobby => move forward one at at time, wrap with total players
// Skips own player ID and anyone who is eliminated => lands on first valid hit
void cycleManualTarget(GameState *attacker, GameState *all_players[], int total_players)
{
    int current_index = -1;
    for (int i = 0; i < total_players; i++)
    {
        if (all_players[i]->player_id == attacker->target_player_id)
        {
            current_index = i;
            break;
        }
    }
    for (int step = 1; step <= total_players; step++)
    {
        int index = (current_index + step + total_players) % total_players;
        if (all_players[index]->player_id != attacker->player_id && !all_players[index]->game_over)
        {
            attacker->target_player_id = all_players[index]->player_id;
            return;
        }
    }
}

// Resolve and return correct target ID based on attacker's target mode
uint32_t resolveTargetID(GameState *attacker, GameState *all_players[], int total_players)
{
    switch (attacker->target_mode)
    {
    case TARGET_RANDOM:
    {
        uint32_t candidates[total_players]; // To hold valid target IDs
        int count = 0;
        for (int i = 0; i < total_players; i++)
        {
            // Add all living opponents into candidates array
            if (all_players[i]->player_id != attacker->player_id && !all_players[i]->game_over)
            {
                candidates[count++] = all_players[i]->player_id;
            }
        }
        if (count == 0) // Nobody left to hit
        {
            return attacker->player_id;
        }
        return candidates[rand() % count]; // Select random candidate
    }
    case TARGET_KO: // Search all active oponents to find who is closest to losing
        uint32_t target_id = attacker->player_id; // Fallback if no target id found
        uint32_t highest_garbage = 0;
        bool found_target = false;
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
            if (!found_target || all_players[i]->pending_garbage > highest_garbage)
            {
                highest_garbage = all_players[i]->pending_garbage;
                target_id = all_players[i]->player_id;
                found_target = true;
            }
        }
        return target_id;
    case TARGET_MANUAL:
    default:
        // Use manually locked target selection
        return attacker->target_player_id;
    }
}