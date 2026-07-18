#include <stdio.h>
#include "killfeed.h"

// Internal State
static KillFeedEntry feed[KILL_FEED_MAX];
// Circular Buffer
static int feed_head = 0; // Starting index
static int feed_count = 0; // Counter for entries currently stored

// Public Functions
void addKillFeed(int source, int target, int lines)
{
    feed[feed_head].source = source;
    feed[feed_head].target = target;
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
        printf("  \e[36mPlayer %d\e[0m sent \e[31m%d lines\e[0m to \e[33mPlayer %d\e[0m!\n", entry->source, entry->lines, entry->target);
    }

    // Padding so the terminal does not move around
    for (int i = feed_count; i < KILL_FEED_MAX; i++)
    {
        printf("                                           \n");
    }
    printf("  =========================================\n");
}