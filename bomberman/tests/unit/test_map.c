#include <stdarg.h>
#include <stddef.h>
#include <cmocka.h>
#include <stdlib.h>
#include "../lib/libbombbrain/map.h"
#include "./lib/libbombbrain/collision.h"
#include "stdio.h"

#pragma region Test Group 1: calc_map_size
// Pure function, no setup/teardown needed

// [Boundary] 1 player should return the minimum map size
// ceil(sqrt(25 * 1)) = ceil(5) = 5, below MIN_MAP_WIDTH; should clamp to 10!
static void test_calc_map_size_minimum(void** state)
{
    (void)state;
    int size = calc_map_size(1);
    assert_int_equal(size, 10);
}

// [Boundary] Exactly enough players to exceed MIN_MAP_WIDTH
// ceil(sqrt(25 * 5)) = ceil(sqrt(125)) = ceil(11.18) = 12 > 10
static void test_calc_map_size_exceeds_minimum(void** state)
{
    (void)state;
    int size = calc_map_size(5);
    assert_int_equal(size, 12);
}

// [Scaling] Larger player counts should produce larger maps
static void test_calc_map_size_scales_with_players(void** state)
{
    (void)state;
    int small = calc_map_size(10);
    int large = calc_map_size(100);
    assert_true(large > small);
}

// [Correctness] Verify the formula directly for a known value
// ceil(sqrt(25 * 16)) = ceil(sqrt(400)) = ceil(20) = 20
static void test_calc_map_size_known_value(void** state)
{
    (void)state;
    int size = calc_map_size(16);
    assert_int_equal(size, 20);
}
#pragma endregion Test Group 1

#pragma region Test Group 2: aabb_collided & move_box
/* Requires a manually constructed map, so we use Setup/Teardown
 * to set up the map and free it after each test
 *
 * 5x5 map layout used across collision tests:
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
static int setup_map(void** state)
{
    (void)state;
    // Allocate map
    map_size = TEST_MAP_SIZE;
    map = malloc(TEST_MAP_SIZE * sizeof(TileType*));
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
    return 0;
}

// [TEARDOWN]
static int teardown_map(void** state)
{
    (void)state;
    map_free();
    return 0;
}

// Empty tile should not collide
static void test_aabb_no_collision_on_empty(void** state)
{
    (void)state;
    // Small box fully inside tile (1,1). Known empty
    bool hit = aabb_collided(1.1f, 1.1f, (Vector2){0.4f, 0.4f});
    assert_false(hit);
}

// Box overlapping a wall tile should collide
static void test_aabb_collision_on_wall(void** state)
{
    (void)state;
    // Box positioned to overlap the border wall at x=0
    bool hit = aabb_collided(0.0f, 1.0f, (Vector2){0.5f, 0.5f});
    assert_true(hit);
}

// Box overlapping the interior wall at (3,2) should collide
static void test_aabb_collision_on_interior_wall(void** state)
{
    (void)state;
    bool hit = aabb_collided(2.7f, 1.7f, (Vector2){0.5f, 0.5f});
    assert_true(hit);
}

// move_box: movement into a wall should be blocked
static void test_move_box_blocked_by_wall(void** state)
{
    (void)state;
    BoundBox box = {
        .position = {3.5f, 1.5f}, // near right wall at x = 4
        .size = {0.4f, 0.4f}};

    // Move right into the border wall at x = 4
    // From x = 3.5, a rightward step should be blocked
    float original_x = box.position.x;
    move_box(&box, (Vector2){0.2f, 0.0f});
    assert_float_equal(box.position.x, original_x, 0.001f);
}

// move_box: movement into empty space should succeed
static void test_move_box_succeeds_in_empty(void** state)
{
    (void)state;
    BoundBox box = {
        .position = {1.1f, 1.1f},
        .size = {0.4f, 0.4f}};

    move_box(&box, (Vector2){0.3f, 0.0f});
    assert_float_equal(box.position.x, 1.4f, 0.001f);
}

// move_box diagonal: one axis blocked should not block the other
static void test_move_box_diagonal_partial(void** state)
{
    (void)state;
    // Box in open area, only bottom wall nearby
    // Moving down (+y) should be blocked, moving right should be free
    BoundBox box = {
        .position = {1.5f, 3.55f}, // bottom edge at 3.55+0.4=3.95, near bottom wall at y=4
        .size = {0.4f, 0.4f}};

    float original_y = box.position.y;

    // x: moving right into open space (should succeed)
    // y: moving down into wall (should be blocked)
    move_box(&box, (Vector2){0.2f, 0.5f});

    assert_float_equal(box.position.x, 1.5f + 0.2f, 0.001f); // x succeeded
    assert_float_equal(box.position.y, original_y, 0.001f);  // y blocked
}
#pragma endregion Test Group 2

int main(void)
{
    const struct CMUnitTest MAP_SIZE_TESTS[] = {
        cmocka_unit_test(test_calc_map_size_minimum),
        cmocka_unit_test(test_calc_map_size_exceeds_minimum),
        cmocka_unit_test(test_calc_map_size_scales_with_players),
        cmocka_unit_test(test_calc_map_size_known_value),
    };

    const struct CMUnitTest COLLISION_TESTS[] = {
        cmocka_unit_test_setup_teardown(test_aabb_no_collision_on_empty, setup_map, teardown_map),
        cmocka_unit_test_setup_teardown(test_aabb_collision_on_wall, setup_map, teardown_map),
        cmocka_unit_test_setup_teardown(test_aabb_collision_on_interior_wall, setup_map, teardown_map),
        cmocka_unit_test_setup_teardown(test_move_box_blocked_by_wall, setup_map, teardown_map),
        cmocka_unit_test_setup_teardown(test_move_box_succeeds_in_empty, setup_map, teardown_map),
        cmocka_unit_test_setup_teardown(test_move_box_diagonal_partial, setup_map, teardown_map),
    };

    int failed = 0;
    failed += cmocka_run_group_tests_name("calc_map_size", MAP_SIZE_TESTS, NULL, NULL);
    failed += cmocka_run_group_tests_name("collision", COLLISION_TESTS, NULL, NULL);
    return failed;
}
