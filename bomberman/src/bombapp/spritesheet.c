#include "spritesheet.h"

void spritesheet_init(Spritesheet *s, Texture2D texture, int cols, int rows, float scale, int fps, bool should_loop) {
    s->texture = texture;
    s->should_loop = should_loop;
    s->columns = cols;
    s->scale = scale;
    s->tile_size = Create(texture.width / cols, texture.height / rows);
    s->last_frame = cols * rows;
    s->curr_frame = 0;
    s->time_per_frame = 1.0f / fps;
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
        .x      = col * s->tile_size.x,
        .y      = row * s->tile_size.y,
        .width  = s->tile_size.x,
        .height = s->tile_size.y
    };
 
    // Dest: where + how on actual screen to draw (world position)
    Rectangle dest = {
        .x      = position.x,
        .y      = position.y,
        .width  = s->tile_size.x * s->scale,
        .height = s->tile_size.y * s->scale,
    };
 
    // Center point of tile
    Vector2 origin = {
        .x = s->tile_size.x * 0.5f,
        .y = s->tile_size.y * 0.5f
    };
 
    // Finally, draw the texture
    DrawTexturePro(s->texture, source, dest, origin, rotation, tint);
}

void spritesheet_free(Spritesheet *s) {
    UnloadTexture(s->texture);
}