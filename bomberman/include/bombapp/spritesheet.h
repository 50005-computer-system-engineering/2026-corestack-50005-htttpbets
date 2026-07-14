#ifndef SPRITESHEET_H
#define SPRITESHEET_H

#include <raylib.h>
#include "lib/vector2int.h"

typedef struct {
    // Initialised
    Texture2D texture;
    bool should_loop; // whether we should loop the animation
    int columns; // No need to store rows since we can calculate everything from col
    float scale; // Scale factor

    // Dynamically calculated from above
    Vector2Int tile_size; // Size of a single tile from original spritesheet
    int last_frame;
    int curr_frame;
    double time_per_frame;
    double time_elapsed;
} Spritesheet;

/**
 * @brief Initialise a spritesheet
 *
 * @param s Spritesheet to initialise
 * @param texture Loaded raylib texture to use as the spritesheet
 * @param cols Number of sprite columns in the sheet
 * @param rows Number of sprite rows in the sheet
 * @param scale Scale factor, 1.0f is 100% default size
 * @param fps Frames per second for the animation
 * @param should_loop Whether we should loop the animation
 */
void spritesheet_init(Spritesheet *s, Texture2D texture, int cols, int rows, float scale, int fps,bool should_loop);
 
// Restart animation from the first frame
void spritesheet_restart(Spritesheet *s);
 
// Advance the animation
void spritesheet_update(Spritesheet *s);
 
/**
 * @brief Draw the current frame centred on a position.
 * @param position World position, sprite is centered on this
 * @param rotation Rotation in degrees (0 for none)
 * @param tint Color tint (WHITE for no tint)
 */
void spritesheet_draw(Spritesheet *s, Vector2 position, float rotation, Color tint);
 
// Unload the texture from GPU memory
// ALWAYS remember to call this!!! or memory leak :(
void spritesheet_free(Spritesheet *s);
#endif