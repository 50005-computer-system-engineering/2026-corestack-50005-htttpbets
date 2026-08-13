#include "spritesheet.h"
#include "config.h"
#include "raylib.h"

void spritesheet_init(Spritesheet* s, const SpritesheetAsset ASSET)
{
    s->texture = LoadTexture(ASSET.path);
    s->should_loop = ASSET.should_loop;
    s->columns = ASSET.cols;
    s->scale = ASSET.scale;
    s->tile_width = (int)(s->texture.width / ASSET.cols);
    s->tile_height = (int)(s->texture.height / ASSET.rows);
    s->last_frame = ASSET.cols * ASSET.rows;
    s->curr_frame = 0;
    s->time_per_frame = 1.0f / ASSET.fps;
    s->time_elapsed = 0.0f;
}

void spritesheet_restart(Spritesheet* s)
{
    s->curr_frame = 0;
    s->time_elapsed = 0.0f;
}

void spritesheet_update(Spritesheet* s)
{
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

void spritesheet_draw(Spritesheet* s, Vector2 position, Vector2 origin_scale, float rotation, Color tint)
{
    int col = s->curr_frame % s->columns;
    int row = s->curr_frame / s->columns;

    // Source: the current frame's crop within the spritesheet texture
    Rectangle source = {
        .x = col * s->tile_width,
        .y = row * s->tile_height,
        .width = s->tile_width,
        .height = s->tile_height};

    // Dest: where + how on actual screen to draw (world position)
    Rectangle dest = {
        .x = position.x * CONFIG.PHYSICS.TILE_SIZE,
        .y = position.y * CONFIG.PHYSICS.TILE_SIZE,
        .width = s->tile_width * s->scale,
        .height = s->tile_height * s->scale,
    };

    // Center point of tile
    Vector2 origin = {
        .x = s->tile_width * origin_scale.x,
        .y = s->tile_height * origin_scale.y};

    // Finally, draw the texture
    DrawTexturePro(s->texture, source, dest, origin, rotation, tint);
}

void spritesheet_free(Spritesheet* s)
{
    UnloadTexture(s->texture);
}
