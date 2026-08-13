#ifndef VIUER_H
#define VIUER_H
/*
A C Implementation of viuer (Ref: https://github.com/atanunq/viuer)
Disclaimer: The code was first translated from Rust to C using https://app.codeconvert.ai/code-converter,
then cleaned up & modified to fit the needs of the project.

Turns image pixels into something the terminal emulator displays. Supports:
(1) Kitty Graphics Protocol (Kitty or Ghostty)
Streamed to the terminal as raw RGBA and scaled by terminal itself

(2) iTerm inline images (Apple)
Original encoded file is handed over and the terminal decodes it

(3) Unicode Half Blocks (Alacritty, GNOME, VSCode, and other barebones terminals)
Each character cell renders 2 stacked pixels. Top pixel is the cell's background color,
and bottom pixel is the foreground color of the "lower half block" glyph.
Doubles vertical resolution, so each character cell counts as 2 pixels tall

Rendering Steps for All:
1) Fill in ViuerConfig
2) Provide image
3) Call viuer_print()

Other helpers:
- Capability detection (which terminal printer we should use)
- Image scaling
- tmux escaping & cursor placement

ERROR CONVENTIONS:
1) Failable functions return an int:
0 on success, -1 on failure.
2) Failable Functions that reutnr a pointer:
NULL on failure.
Both ways will have their string available from viu_last_error()
*/

#include "vector2.h"
#include <stdbool.h>
#include <stddef.h>
#pragma region Terminal Detection
typedef enum {
    HALF_BLOCKS = 0, // Barebones Terminals
    KITTY,           // Kitty or Ghostty
    ITERM,           // Apple
} TerminalType;

/**
 * @brief Automatically determine the current terminal emulator type
 * @see viuer_is_kitty_supported()
 * @return TerminalType
 */
TerminalType viuer_get_terminal_type(void);

/**
 * @brief Determine if the terminal advertises iTerm2-style inline image support
 */
bool viuer_is_iterm_supported(void);

/**
 * @brief Detects Kitty graphics support
 */
bool viuer_is_kitty_supported(void);

/**
 * @brief Check if the terminal advertises 24-bit color.
 * Decides whether half-blocks are painted with exact RGB values, (true)
 * or mapped onto the 256-color palette (false)
 */
bool viuer_is_truecolor(void);

/**
 * @brief Whether the terminal is running inside a tmux session.
 * If so, we need to wrap tmux's passthrough envelope around the image,
 * if not it automatically discards escape sequences it does not recognise
 * (including our graphics sequence!)
 * See README.md for tmux configuration!
 */
bool viuer_is_tmux(void);

/**
 * @brief Get the current terminal size in character cells
 * Falls back to 80x24 (default terminal size)
 * @param size prealloc'd vector to store the size
 * @return 0 on success, -1 on failure
 */
int viuer_terminal_size(Vector2* size);
#pragma endregion Terminal Detection

#pragma region Rendering
typedef struct {
    Vector2 pos;              // row/column offset, may be negative
    Vector2 size;             // target width & height in cells, 0 = derive
    bool absolute_offset;     // whether to use absolute (top left of terminal) or relative offset (from cursor pos)
    bool transparent;         // whether to honor the alpha channel instead of painting over it
    TerminalType render_type; // which renderer to use
    int image_id; // Identifies repeatedly redrawn sprites, left at 0 for single-frame output.
} ViuerConfig;

/**
 * @brief Init a default config:
 * Pos: {0,0} | Size: {0,0} | Offset: True | Transparent: True | Render_Type: Auto-Detect
 * @param cfg Config to init
 */
void viuer_config_default(ViuerConfig* cfg);

/**
 * @brief Calculates the final on-screen size in character cells for an image.
 *
 * @param size Width and Height of image
 * @param cfg Config to use
 * @return On-screen Dimensions
 */
void viuer_fit_dimensions(Vector2 size, const ViuerConfig* cfg, Vector2* out_size);
#pragma endregion Rendering

#pragma region Images

// In-memory image, 8-bit RGBA
// Always heap-allocated, release with viu_image_free()
typedef struct {
    // { Width, Height } of image
    Vector2 size;

    // width*height*4 bytes in row-major order (RGBA)
    // (eg: R, G, B, A for pixel [0,0], then pixel [1,0], and so on)
    unsigned char* pixels;
} Image;

/**
 * @brief Allocates a fully transparent width x height image
 * @param size Size of image
 * @return Image* Allocated image, NULL if dimensions are negative/memory ran out
 */
Image* viuer_image_new(Vector2 size);

Image* viuer_image_load_memory(const unsigned char* data, size_t len);

// Frees memory of image. Remember to call or memory leak!!! :(
void viuer_image_free(Image* img);

// Returns a new image scaled to exactly [size] pixels
Image* viuer_resize(const Image* src, Vector2 size);

// Returns a new image containing the rectangle of [size] pixels at [pos] of image
// Utilised for spritesheets. Similar to RayLib's DrawImageTexturePro
Image* viuer_image_crop(const Image* src, Vector2 pos, Vector2 size);
#pragma endregion Images

#pragma region Printers
/**
 * @brief Draws a decoded image using the best protocol detected
 * On success, writes the size actually drawn in character cells to *out_size
 * @param img  Image to draw
 * @param cfg Rendering config
 * @param out_size Rendered size
 * @return int 0 on success, -1 on failure
 */
int viuer_print(const Image* img, const ViuerConfig* cfg, Vector2* out_size);

 /**
  * @brief Hands raw encoded bytes of an image to iTerm2, 
  * which decodes and displays it itself.
  * The terminal will do its own decoding and scaling, which is faster & sharper.
  * 
  * @param data Raw original file of the image
  * @param len Byte Length of data
  * @param cfg Rendering config
  * @param img_size Pixel dimensions of encoded image
  * @param out_size Character cells size it should occupy
  * @return int 0 on success, -1 on failure
  */
int viuer_print_iterm_bytes(const unsigned char* data, size_t len, const ViuerConfig* cfg, Vector2 img_size, Vector2* out_size);

/**
 * @brief Loads the file at path and draws it.
 * @param path Path to image
 * @param cfg Rendering configuration
 * @param out_size Character cells size it should occupy
 * @return int 0 on success, -1 on failure
 */
int viuer_print_from_file(const char* path, const ViuerConfig* cfg, Vector2* out_size);

/**
 * @brief Erases a previously drawn image of size character cells,
 * positioned at the same x, y and absolute_offset used to draw it.
 *
 * How it does this depends on how the image was drawn, which is why it
 * takes the config rather than just a rectangle (see implementation comments)
 * @param cfg Rendering config
 * @param size Character cells to erase
 * @return int 0 on success, -1 on failure
 */
int viuer_erase(const ViuerConfig* cfg, Vector2* size);
#pragma endregion Printers

#pragma region Error Reporting
// Message left by most recent failing call on this thread.
// Never NULL. "No Error" before anything has failed.
const char* viuer_last_error(void);

// Records an error message (printf-style) and returns -1
// So a failing branch can be written as a single line:
//     if (!thing) return viuer_set_error("no thing: %s", why);
int viuer_set_error(const char* fmt, ...);
#pragma endregion Error Reporting
#endif
