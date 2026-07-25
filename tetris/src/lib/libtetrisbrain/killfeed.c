#include <stdio.h>
#include <inttypes.h>
#include "killfeed.h"

// Internal State
static KillFeedEntry feed[KILL_FEED_MAX];
// Circular Buffer
static int feed_head = 0;  // Starting index
static int feed_count = 0; // Counter for entries currently stored

// Public Functions
void addKillFeed(uint32_t source, uint32_t target, uint32_t lines)
{
    feed[feed_head].source_player = source;
    feed[feed_head].target_player = target;
    feed[feed_head].lines = lines;

    feed_head = (feed_head + 1) % KILL_FEED_MAX; // Modulo Arithmetic
    if (feed_count < KILL_FEED_MAX)
    {
        feed_count++;
    }
}

void drawKillFeed(void)
{
    printf("                                           \n");
    printf("\n  <!> BATTLE LOG <!>\n");
    printf("  =========================================\n");
    // From oldest to newest
    for (int i = 0; i < feed_count; i++)
    {
        // Go to oldest entry, print to newest entry
        int index = (feed_head - feed_count + i + KILL_FEED_MAX) % KILL_FEED_MAX;
        KillFeedEntry *entry = &feed[index];
        printf("  \e[36mPlayer %u\e[0m sent \e[31m%u lines\e[0m to \e[33mPlayer %u\e[0m!\n", entry->source_player, entry->lines, entry->target_player);
    }

    // Padding so the terminal does not move around
    for (int i = feed_count; i < KILL_FEED_MAX; i++)
    {
        printf("                                           \n");
    }
    printf("  =========================================\n");
}