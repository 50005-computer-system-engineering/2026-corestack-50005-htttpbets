#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>
#include <stdbool.h>
#include <limits.h> // Required for extreme boundary testing

// Include the game engine headers
#include "lib/libbombbrain/map.h"
#include "lib/libbombbrain/collision.h"
#include "lib/libbombbrain/inventory.h"
#include "lib/libbombbrain/bomb_control.h"
#include "lib/libbombbrain/powerup_control.h"

#define FUZZ_ITERATIONS 100000

int main() {
    printf("[*] Starting targeted fuzzing...\n");
    srand(time(NULL));

    /* --- INITIALIZATION --- */
    int num_players = 4;
    map_generate(num_players); 
    powerup_init(); 
    init_bombs(); 

    printf("[*] State initialized. Beginning bombardment of %d iterations...\n", FUZZ_ITERATIONS);

    /* --- THE ADVERSARIAL BOMBARDMENT LOOP --- */
    for (int i = 0; i < FUZZ_ITERATIONS; i++) {
        
        // Automated Test Generation: Combining Randomness with Boundary Value Analysis
        // Instead of pure randoms, we have a 20% chance to inject an extreme edge case (INT_MAX, INT_MIN, 0).
        float hostile_x, hostile_y;
        if (rand() % 100 < 20) {
            // Test extreme boundaries that could cause integer overflow/underflow
            hostile_x = (rand() % 2 == 0) ? (float)INT_MAX : (float)INT_MIN;
            hostile_y = (rand() % 2 == 0) ? 0.0f : (float)(map_size + 1); // Exactly 1 out of bounds
        } else {
            // Standard random automated fuzzing (-2000 to +2000)
            hostile_x = (float)((rand() % 4000) - 2000); 
            hostile_y = (float)((rand() % 4000) - 2000); 
        }

        float hostile_width = (float)(rand() % 100);
        float hostile_height = (float)(rand() % 100);
        float hostile_delta_time = (float)((rand() % 200) - 100) / 10.0f; 
        
        Vector2 hostile_pos = { hostile_x, hostile_y };
        Vector2 hostile_size = { hostile_width, hostile_height };
        
        InventoryStock hostile_stock;
        hostile_stock.remaining_bombs = (rand() % 20) - 10; 
        hostile_stock.num_bombs = (rand() % 10);
        hostile_stock.num_fires = (rand() % 50); 
        hostile_stock.bomb_replenish_timer = hostile_delta_time;

        // Attack the engines with the generated data
        BoundBox hostile_box = { hostile_pos, hostile_size };
        move_box(&hostile_box, hostile_pos); 
        place_bomb(hostile_pos, &hostile_stock);
        inventory_update(&hostile_stock, hostile_delta_time);
        tick_bombs(hostile_delta_time);
        tick_explosions(hostile_delta_time);
    }

    /* --- TEARDOWN --- */
    powerup_free(); 
    map_free(); 

    printf("[*] Fuzzing complete. Exiting gracefully.\n");
    return 0;
}