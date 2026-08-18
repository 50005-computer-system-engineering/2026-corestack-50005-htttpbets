#include <stdarg.h>
#include <stddef.h>
#include <cmocka.h>
#include <stdlib.h>
#include "../lib/libbombbrain/bomb_control.h"
#include "../lib/libbombbrain/map.h"

#pragma region Test Group 1: place_bomb
/* Requires a manually constructed map, so we use Setup/Teardown
 * to set up the map and free it after each test
 *
 * 5x5 map layout used across bomb placement tests:
 *   0 1 2 3 4
 * 0 W W W W W
 * 1 W . . . W
 * 2 W . . W W <- wall at (3,2)
 * 3 W . . . W
 * 4 W W W W W
 * where W = WALL, . = EMPTY
 // Coordinates: map[x][y], x = column, y = row
*/

#define TEST_MAP_SIZE 5

// [SETUP]
static int setup_map(void **state) {
    (void)state;
    // Allocate map
    map_size = TEST_MAP_SIZE;
    map = malloc(TEST_MAP_SIZE * sizeof(TileType *));
    for (int i = 0; i < TEST_MAP_SIZE; i++) {
        map[i] = malloc(TEST_MAP_SIZE * sizeof(TileType));
        for (int j = 0; j < TEST_MAP_SIZE; j++)
            map[i][j] = EMPTY;
    }

    // Border walls
    for (int i = 0; i < TEST_MAP_SIZE; i++) {
        map[0][i] = WALL;
        map[TEST_MAP_SIZE - 1][i] = WALL;
        map[i][0] = WALL;
        map[i][TEST_MAP_SIZE - 1] = WALL;
    }

    // Interior wall at tile (3, 2)
    map[3][2] = WALL;

    init_bombs();
    return 0;
}

// [TEARDOWN]
static int teardown_map(void **state) {
    (void)state;
    map_free();
    return 0;
}

// Placing a bomb on an empty tile with stock available should succeed
static void test_place_bomb_succeeds_on_empty_tile(void **state) {
    (void)state;
    InventoryStock stock = {.remaining_bombs = 1, .num_bombs = 1, .num_fires = 1, .bomb_replenish_timer = 0.0f};
    bool placed = place_bomb((Vector2){1, 1}, &stock);
    assert_true(placed);
    assert_int_equal(map[1][1], BOMB);
}

// Placing a bomb should consume one from the inventory's remaining stock
static void test_place_bomb_consumes_inventory(void **state) {
    (void)state;
    InventoryStock stock = {.remaining_bombs = 2, .num_bombs = 2, .num_fires = 1, .bomb_replenish_timer = 0.0f};
    place_bomb((Vector2){1, 1}, &stock);
    assert_int_equal(stock.remaining_bombs, 1);
}

// Placing a bomb on a wall tile should fail and leave the tile untouched
static void test_place_bomb_fails_on_wall(void **state) {
    (void)state;
    InventoryStock stock = {.remaining_bombs = 1, .num_bombs = 1, .num_fires = 1, .bomb_replenish_timer = 0.0f};
    bool placed = place_bomb((Vector2){3, 2}, &stock);
    assert_false(placed);
    assert_int_equal(map[3][2], WALL);
}

// Placing a bomb with no remaining stock should fail even on an empty tile
static void test_place_bomb_fails_when_out_of_stock(void **state) {
    (void)state;
    InventoryStock stock = {.remaining_bombs = 0, .num_bombs = 1, .num_fires = 1, .bomb_replenish_timer = 0.0f};
    bool placed = place_bomb((Vector2){1, 1}, &stock);
    assert_false(placed);
    assert_int_equal(map[1][1], EMPTY);
}
#pragma endregion Test Group 1

int main(void) {
    const struct CMUnitTest place_bomb_tests[] = {
        cmocka_unit_test_setup_teardown(test_place_bomb_succeeds_on_empty_tile, setup_map, teardown_map),
        cmocka_unit_test_setup_teardown(test_place_bomb_consumes_inventory,     setup_map, teardown_map),
        cmocka_unit_test_setup_teardown(test_place_bomb_fails_on_wall,          setup_map, teardown_map),
        cmocka_unit_test_setup_teardown(test_place_bomb_fails_when_out_of_stock, setup_map, teardown_map),
    };

    int failed = 0;
    failed += cmocka_run_group_tests_name("place_bomb", place_bomb_tests, NULL, NULL);
    return failed;
}
