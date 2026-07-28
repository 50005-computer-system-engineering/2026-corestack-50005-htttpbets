#include "map.h"
#include "collision.h"

Vector4 get_all_overlapping_tiles(float x, float y, Vector2 size) {
    int left = (int)floorf(x);
    int right = (int)ceilf(x + size.x - 0.001f);
    int top = (int)floorf(y);
    int bottom = (int)ceilf(y + size.y - 0.001f);

    return (Vector4){left, right, top, bottom};
}

bool aabb_collided(float x, float y, Vector2 size) {
    Vector4 tiles = get_all_overlapping_tiles(x, y, size);

    // Loop through all overlapping grid tiles and check if they are solid
    for (int ty = tiles.z; ty < tiles.w; ty++) {
        for (int tx = tiles.x; tx < tiles.y; tx++) {
            if (tile_is_solid(tx, ty))
                return true;
        }
    }

    return false;
}

void move_box(BoundBox *box, Vector2 moveAmt) {
    Vector2 new_pos = Vector2Add(box->position, moveAmt);

    // Validate horizontal movement
    if (!aabb_collided(new_pos.x, box->position.y, box->size))
        box->position.x = new_pos.x;
    // Validate vertical movement
    if (!aabb_collided(box->position.x, new_pos.y, box->size))
        box->position.y = new_pos.y;
}

Vector2 get_center_box(BoundBox *box) {
    // Position + Center of sprite
    int grid_x = (int)floorf(box->position.x + box->size.x * 0.5f);
    int grid_y = (int)floorf(box->position.y + box->size.y * 0.5f);
    return (Vector2){grid_x, grid_y};
}