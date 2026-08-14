#include "map.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define MIN_MAP_WIDTH 10
#define EMPTY_CHANCE 20 // % chance for empty tiles remain empty
#define WALL_CHANCE 30  // % chance for empty tiles to be walls

int map_size = MIN_MAP_WIDTH;
TileType** map;

int calc_map_size(int num_players)
{
    int size = ceil(sqrt(25 * num_players));
    return size > MIN_MAP_WIDTH ? size : MIN_MAP_WIDTH;
}

void calc_player_spawns(int num_players, int inner_size, int spawns[][2])
{
    // ** PHASE 1: Assign Corners in Diagonal Order** (TL, BR, TR, BL)
    int corners[4][2] = {{1, 1}, {inner_size, inner_size}, {1, inner_size}, {inner_size, 1}};
    int player = 0;
    for (int i = 0; i < 4 && player < num_players; i++) {
        spawns[player][0] = corners[i][0]; // x pos
        spawns[player][1] = corners[i][1]; // y pos
        player++;
    }

    // Check if we need to proceed to Phase 2
    if (player >= num_players)
        return;

    // ** PHASE 2: Assign remaining to inner spaces utilising Strip Algorithm **
    inner_size -= 2; // Remove corners
    if (inner_size < 1)
        inner_size = 1; // Prevent division by 0

    // (1) Calculate number of horizontal 'strips' of players
    int players_left = num_players - player;
    printf("players_left: %d\n", players_left);
    int num_strips = (int)round(sqrt((double)players_left));
    printf("num_strips: %d\n", num_strips);
    if (num_strips < 1)
        num_strips = 1; // prevent division by 0

    // Loop until finished all strips or finished spawning all players
    for (int s = 0; s < num_strips && player < num_players; s++) {
        players_left = num_players - player;
        int strips_left = num_strips - s;

        // (2) Calculate how many players should go in this strip
        int players_in_strip = ceil((double)players_left / strips_left);

        // (3) Find position of middle of strip
        int y0 = (s * inner_size) / num_strips;       // top of strip
        int y1 = ((s + 1) * inner_size) / num_strips; // bottom of strip
        int row = 2 + (y0 + y1 - 1) / 2;              // center of strip, +2 for border & corner offset

        // (4) Place players across strip
        for (int i = 0; i < players_in_strip; i++) {
            int x0 = (i * inner_size) / players_in_strip;       // left of player
            int x1 = ((i + 1) * inner_size) / players_in_strip; // right of player
            int col = 2 + (x0 + x1 - 1) / 2;                    // center of player, +2 for border & corner offset

            spawns[player][0] = row;
            spawns[player][1] = col;
            player++;
        }
    }
}

void map_generate(int num_players)
{
    // (1) Allocate map size
    map_size = calc_map_size(num_players) + 2; // Extra 2 for borders
    // Allocate space for array of row pointers
    map = (TileType**)malloc(map_size * sizeof(TileType*));
    // Allocate space for each individual row
    for (int i = 0; i < map_size; i++) {
        map[i] = (TileType*)malloc(map_size * sizeof(TileType));

        // Initialise all values to -1
        for (int j = 0; j < map_size; j++)
            map[i][j] = -1;
    }

    // (2) Generate borders
    for (int i = 0; i < map_size; ++i) {
        // Top and bottom border
        map[0][i] = WALL;
        map[map_size - 1][i] = WALL;

        // Left and right border
        map[i][0] = WALL;
        map[i][map_size - 1] = WALL;
    }

    int inner_size = map_size - 2;
    // (3) Assign space for players
    int (*spawns)[2] = malloc(num_players * sizeof(int[2]));
    calc_player_spawns(num_players, inner_size, spawns);
    for (int i = 0; i < num_players; i++) {
        map[spawns[i][0]][spawns[i][1]] = PLAYER;

        // Create '+' shape of empty spots around player, unless at border
        if (spawns[i][0] > 1)
            map[spawns[i][0] - 1][spawns[i][1]] = EMPTY; // Left
        if (spawns[i][0] < map_size - 2)
            map[spawns[i][0] + 1][spawns[i][1]] = EMPTY; // Right
        if (spawns[i][1] > 1)
            map[spawns[i][0]][spawns[i][1] - 1] = EMPTY; // Top
        if (spawns[i][1] < map_size - 2)
            map[spawns[i][0]][spawns[i][1] + 1] = EMPTY; // Bottom
    }
    free(spawns);

    // (4) Assign random tiles for remaining spaces
    // 20% Empty, 30% Indestructible, 50% Breakable
    srand(time(NULL)); // Seed random number generator
    for (int i = 0; i < map_size; ++i) {
        for (int j = 0; j < map_size; ++j) {
            if ((int)map[i][j] == -1) {
                int rng = rand() % 100;
                if (rng < EMPTY_CHANCE)
                    map[i][j] = EMPTY;
                // Spawning wall: We don't want to "block in" players,
                // so prevent walls from spawning directly next to border
                else if (rng < EMPTY_CHANCE + WALL_CHANCE && i > 1 && i < inner_size - 1 && j > 1 && j < inner_size - 1)
                    map[i][j] = WALL;
                else
                    map[i][j] = BREAKABLE;
            }
        }
    }
}

void map_free()
{
    if (map == NULL)
        return;

    // Free each row
    for (int i = 0; i < map_size; i++)
        free(map[i]);

    // Free array of row pointers
    free(map);
    map = NULL;
}

void map_debugprint()
{
    if (map == NULL) {
        printf("[Map]: EMPTY!");
        return;
    }

    printf("[Map]:\n");
    for (int i = 0; i < map_size; ++i) {
        for (int j = 0; j < map_size; ++j)
            printf("%4d ", map[i][j]); // Padding for grid structure
        printf("\n");                  // Next line after completing row
    }
}

bool tile_is_solid(int tile_x, int tile_y)
{
    TileType tile = map[tile_x][tile_y];
    return tile == WALL || tile == BREAKABLE;
}
