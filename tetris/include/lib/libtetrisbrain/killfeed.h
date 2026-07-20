#ifndef TETRISBRAIN_KILLFEED_H
#define TETRISBRAIN_KILLFEED_H

// Max entries in the kill feed
#define KILL_FEED_MAX 6

// A single attack log entry
typedef struct
{
    int source; // Attacker
    int target; // Reciever
    int lines;  // Number of lines sent
} KillFeedEntry;

// Add an attack to the kill feed
void addKillFeed(int source_player, int target_player, int lines);

// Render the kill feed
void drawKillFeed(void);

#endif