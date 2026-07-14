#include "bombapp/spritesheet.h"
#include <raylib.h>

void spritesheet_init(Spritesheet *s, const SpritesheetAsset asset) {
    s->texture = LoadTexture(asset.path);
    s->should_loop = asset.should_loop;
    s->columns = asset.cols;
    s->scale = asset.scale;
    s->tile_width = (int)(s->texture.width / asset.cols);
    s->tile_height = (int)(s->texture.height / asset.rows);
    s->last_frame = asset.cols * asset.rows;
    s->curr_frame = 0;
    s->time_per_frame = 1.0f / asset.fps;
    s->time_elapsed = 0.0f;
}

void spritesheet_restart(Spritesheet *s) {
    s->curr_frame = 0;
    s->time_elapsed = 0.0f;
}

void spritesheet_update(Spritesheet *s) {
    s->time_elapsed += GetFrameTime();

    // Wait till the next frame
    if (s->time_elapsed < s->time_per_frame)
        return;

    // Advance frame and reset
    s->curr_frame++;
    s->time_elapsed = 0.0f;

    // Reached end of animation!
    if (s->curr_frame >= s->last_frame) {
        if (s->should_loop) // Loop to beginning
            s->curr_frame = 0;
        else // Stop animation at previous (last) frame
            s->curr_frame = s->last_frame - 1;
    }
}

void spritesheet_draw(Spritesheet *s, Vector2 position, float rotation, Color tint) {
    int col = s->curr_frame % s->columns;
    int row = s->curr_frame / s->columns;
 
    // Source: the current frame's crop within the spritesheet texture
    Rectangle source = {
        .x      = col * s->tile_width,
        .y      = row * s->tile_height,
        .width  = s->tile_width,
        .height = s->tile_height
    };
 
    // Dest: where + how on actual screen to draw (world position)
    Rectangle dest = {
        .x      = position.x,
        .y      = position.y,
        .width  = s->tile_width* s->scale,
        .height = s->tile_height * s->scale,
    };
 
    // Center point of tile
    Vector2 origin = {
        .x = s->tile_width* 0.5f,
        .y = s->tile_height * 0.5f
    };
 
    // Finally, draw the texture
    DrawTexturePro(s->texture, source, dest, origin, rotation, tint);
}

void spritesheet_free(Spritesheet *s) {
    UnloadTexture(s->texture);
}