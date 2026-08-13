// (Ref: https://github.com/atanunq/viuer)
#include "viuer.h"

#include <errno.h>
#include <math.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <threads.h>
#include "vector2.h"
#include "utils/encoder.h"

#pragma region stb_image config
#define STB_IMAGE_IMPLEMENTATION // Pulls the function bodies in
// Only the PNG, JPEG, BMP, and TGA decoders are needed
#define STBI_ONLY_PNG
#define STBI_ONLY_JPEG
#define STBI_ONLY_BMP
#define STBI_ONLY_TGA
#include "stb_image.h"
#pragma endregion stb_image config

#pragma region Terminal Detection
// True if environment variable 'var' is set and contains 'needle'.
static bool env_contains(const char *var, const char *needle)
{
    // Get value of env variable
    const char *v = getenv(var);
    // Find first occurance of needle in value
    return v != NULL && strstr(v, needle) != NULL;
}


TerminalType viuer_get_terminal_type(void) {
    if (viuer_is_kitty_supported())
        return KITTY;
    else if (viuer_is_iterm_supported())
        return ITERM;
    else
        return HALF_BLOCKS;
}


// Taken straight from viuer
bool viuer_is_iterm_supported(void) {
    return env_contains("TERM_PROGRAM", "iTerm") ||
           env_contains("LC_TERMINAL", "iTerm") ||
           env_contains("TERM_PROGRAM", "WezTerm");
}

// Cached in case we're utilising this for animation which asks once per frame.
// -1 means not yet computed.
bool viuer_is_tmux(void)
{
    static int cached = -1;
    if (cached < 0)
        cached = (getenv("TMUX") != NULL) || env_contains("TERM", "tmux");
    return cached != 0;
}


// Same as above, cached
bool viuer_is_kitty_supported(void)
{
    static int cached = -1; // Cached between sessios
    if (cached >= 0) 
        return (bool)cached;

    // Check if we're utilising Kitty or Ghostty using $TERM env check
    cached = getenv("KITTY_WINDOW_ID") != NULL ||
             env_contains("TERM", "kitty") || env_contains("TERM", "ghostty");

    // HOWEVER, in tmux, $TERM usually says tmux instead of the terminal behind it
    // So the checks above usually fail!
    // So we need to check for the actual outer terminal's identity if it's Kitty-compatible
    if (!cached && viuer_is_tmux()) {
        cached = env_contains("TERM_PROGRAM", "ghostty") ||
                 env_contains("TERM_PROGRAM", "kitty") ||
                 env_contains("LC_TERMINAL", "kitty") ||
                 getenv("KITTY_WINDOW_ID") != NULL;
    }

    return (bool)cached;
}

// Use $COLORTERM to detect truecolor support
bool viuer_is_truecolor(void)
{
    return env_contains("COLORTERM", "truecolor") ||
           env_contains("COLORTERM", "24bit");
}


// NOT cached, queries every run
// as terminal can be resized dynamically
int viuer_terminal_size(Vector2* size)
{
    // Default Terminal Size
    *size = (Vector2){.x = 80, .y = 24};

    struct winsize ws;
    // ioctl call to get the current terminal size using TIOCGWINSZ and store it in ws
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == 0 
        && ws.ws_col > 0 && ws.ws_row > 0) {
            *size = (Vector2){.x = ws.ws_col, .y = ws.ws_row};
    }

    return 0;
}
#pragma endregion Terminal Detection

#pragma region Rendering

void viuer_config_default(ViuerConfig* cfg)
{
    // Fill whole struct with zeros
    memset(cfg, 0, sizeof* cfg);

    // Defaults
    cfg->pos = VECTOR2_ZERO;
    cfg->size = VECTOR2_ZERO;
    cfg->absolute_offset = true;
    cfg->transparent = true;
    cfg->render_type = viuer_get_terminal_type();
}

// A character cell is roughly twice as tall as it is wide, 
// and half-block rendering stacks 2 pixels inside 1 cell.
// Both give 2 pixel rows per character row.
#define CELL_PIXEL_RATIO 2
void viuer_fit_dimensions(Vector2 img_size, const ViuerConfig* cfg, Vector2* out_size)
{
    *out_size = VECTOR2_ZERO;

    // (1) Both dimensions specified
    if (cfg->size.x > 0 && cfg->size.y > 0) {
        // Obey exactly even if aspect ratio distorts
        *out_size = cfg->size;
    } 
    // (2) One dimension specified
    else if (cfg->size.x > 0) {
        // Scale to fit width
        out_size->x =  cfg->size.x;
        out_size->y = (int)lround((double)img_size.y * out_size->x / img_size.x / CELL_PIXEL_RATIO);
    } else if (cfg->size.y > 0) {
        // Scale to fit height
        out_size->y = cfg->size.y;
        out_size->x = (int)lround((double)img_size.x * out_size->y * CELL_PIXEL_RATIO / img_size.y);
    } 
    // (3) Nothing specified
    else {
        // Shrink to fit, never enlarge
        // Max size is the size of actual image.
        // We purposely hold back 1 row as a buffer so that it doesn't scroll the terminal
        // We fill in this blank row after the shell prompt appears after.
        Vector2 term_size;
        viuer_terminal_size(&term_size);
        int avail_rows = term_size.y > 1 ? term_size.y - 1 : 1; // Reserve row!

        double natural_width = img_size.x;
        double natural_height = img_size.y / CELL_PIXEL_RATIO;
        
        // Scale to fit size of terminal
        double scale = 1.0;
        if (natural_width > term_size.x)
            scale = term_size.x / natural_width;
        if (natural_height * scale > avail_rows)
            scale = avail_rows / natural_height;

        // Calculate scaled dimensions
        out_size->x = (int)(natural_width * scale);
        out_size->y = (int)(natural_height * scale);
    }

    // Cap to minimum size
    if (out_size->x < 1)
        out_size->x = 1;
    if (out_size->y < 1)
        out_size->y = 1;
}
#pragma endregion Rendering

#pragma region Images
Image* viuer_image_new(Vector2 size)
{
    if (size.x <= 0 || size.y <= 0) {
        viuer_set_error("Invalid image size %dx%d", size.x, size.y);
        return NULL;
    }

    Image* img = calloc(1, sizeof* img);
    if (!img) {
        viuer_set_error("Out of memory!");
        return NULL;
    }

    // calloc zeroes the buffer, so this is fully transparent by default!
    // RGBA (4-byte)
    img->pixels = calloc((size_t)size.x * (size_t)size.y, 4);
    if (!img->pixels) {
        free(img);
        viuer_set_error("Out of memory!");
        return NULL;
    }

    img->size = size;
    return img;
}

Image* viuer_image_load_memory(const unsigned char* data, size_t len)
{
    Vector2 size = VECTOR2_ZERO;
    int channels_in_file = 0; // Grayscale? RGB? RGBA?

    // Decode image into pixel data
    unsigned char* px = stbi_load_from_memory(
        data, 
        (int)len,
        &size.x,
        &size.y, 
        &channels_in_file, 
        4
    );

    if (!px) {
        viuer_set_error("Could not decode image: %s", stbi_failure_reason());
        return NULL;
    }

    // Create image
    Image* img = calloc(1, sizeof* img);
    if (!img) {
        stbi_image_free(px);
        viuer_set_error("Out of memory!");
        return NULL;
    }

    img->size = size;
    img->pixels = px;
    return img;
}

// Returns a new image scaled to exactly [size] pixels
Image* viuer_resize(const Image* src, Vector2 size)
{
    if (!src) {
        viuer_set_error("Resize: No source image!");
        return NULL;
    }
    if (size.x <= 0) size.x = 1;
    if (size.y <= 0) size.y = 1;

    // (1) Already the right size
    // Return copy of source image so ownership is uniforrm (nvr an alias!)
    if (size.x == src->size.x && size.y == src->size.y) {
        Image* copy = viuer_image_new(size);
        if (!copy) return NULL;
        memcpy(copy->pixels, src->pixels, (size_t)size.x * size.y * 4);
        return copy;
    }

    // (2) Resize image
    Image* dst = viuer_image_new(size);
    if (!dst) return NULL;
    
    // Walk every ROW of the output image
    for (int dy = 0; dy < size.y; dy++) {
        // Figure out which slice of source rows [sy0, sy1) this output row covers
        int sy0 = (int)((long long)dy * src->size.y / size.y);
        int sy1 = (int)((long long)(dy + 1) * src->size.y / size.y);
        if (sy1 <= sy0) sy1 = sy0 + 1; // guarantee the band is never empty
        if (sy1 > src->size.y) sy1 = src->size.y;   // clamp to image bounds

        // Walk every COLUMN of the output image
        for (int dx = 0; dx < size.x; dx++) {
            // Same idea as above, but for the column slice [sx0, sx1).
            int sx0 = (int)((long long)dx * src->size.x / size.x);
            int sx1 = (int)((long long)(dx + 1) * src->size.x / size.x);
            if (sx1 <= sx0) sx1 = sx0 + 1;
            if (sx1 > src->size.x) sx1 = src->size.x;

            // Accumulate every source pixel in the sy0..sy1 x sx0..sx1 rectangle
            // Sums are unsigned longs as a large band can easily exceed the range of an int once 255-valued channels are added up
            unsigned long r = 0, g = 0, b = 0, a = 0, n = 0;
            for (int sy = sy0; sy < sy1; sy++) {
                const unsigned char* row = src->pixels + (size_t)sy * src->size.x * 4;
                for (int sx = sx0; sx < sx1; sx++) {
                    const unsigned char* p = row + (size_t)sx * 4;
                    r += p[0]; g += p[1]; b += p[2]; a += p[3];
                    n++;   // count of pixels summed, used as the averaging divisor
                }
            }

            // Average the accumulated rectangle and write it as the single output pixel
            unsigned char* out = dst->pixels + ((size_t)dy * size.x + dx) * 4;
            out[0] = (unsigned char)(r / n);
            out[1] = (unsigned char)(g / n);
            out[2] = (unsigned char)(b / n);
            out[3] = (unsigned char)(a / n);
        }
    }

    return dst;
}

// Lifts a rectangle out of an image for spritesheets
Image* viuer_image_crop(const Image* src, Vector2 pos, Vector2 size)
{
    if (!src) {
        viuer_set_error("Crop: No source image!");
        return NULL;
    }

    // Clip the requested rectangle to what actually exists
    if (pos.x < 0) { 
        size.x += pos.x; 
        pos.x = 0; 
    }
    if (pos.y < 0) { 
        size.y += pos.y; 
        pos.y = 0; 
    }
    if (pos.x >= src->size.x || pos.y >= src->size.y || size.x <= 0 || size.y <= 0) {
        viuer_set_error("Crop: Rectangle %dx%d at (%d,%d) lies outside a %dx%d image",
                        size.x, size.y, pos.x, pos.y, src->size.x, src->size.y);
        return NULL;
    }
    if (pos.x + size.x > src->size.x)  
        size.x = src->size.x - pos.x;
    if (pos.y + size.y > src->size.y)  
        size.y = src->size.y - pos.y;

    // Create new image
    Image* dst = viuer_image_new(size);
    if (!dst) 
        return NULL;

    // Copy in the rectangle
    size_t row_bytes = (size_t)size.x * 4;
    for (int row = 0; row < size.y; row++) {
        const unsigned char* from = src->pixels + ((size_t)(pos.y + row) * src->size.x + pos.x) * 4;
        memcpy(dst->pixels + (size_t)row * row_bytes, from, row_bytes);
    }
    return dst;
}

// Frees memory of image. Remember to call or memory leak!!! :(
void viuer_image_free(Image* img)
{
    if (!img) 
        return;
    free(img->pixels);
    free(img);
}
#pragma endregion Images

#pragma region Growable Output Buffer
// (mostly ported over from rust viuer)
// Half-block rendering emits roughly forty bytes of escape sequence per

/**
 * @brief Half-block rendering emits ~40 bytes of escape sequences
 * per character cell, so a full-screen frame is tens of kilobytes.
 * To prevent visible animation tears, each frame is assembled in memory
 * and pushed out with a single write.
 */
typedef struct {
    char* data;
    size_t len;
    size_t cap;
} Buf;

static void buf_init(Buf* b) { b->data = NULL; b->len = 0; b->cap = 0; }
static void buf_free(Buf* b) { free(b->data); buf_init(b); }

// Guarantees room for `extra` more bytes plus a terminating NUL.
// Capacity doubles rather than growing by a fixed step, so appending N bytes one piece
// at a time costs O(N) in total rather than O(N^2).
static int buf_reserve(Buf* b, size_t extra)
{
    // Already enough room, nothing to do.
    if (b->len + extra + 1 <= b->cap)
        return 0;

    // Double the capacity (starting from 8KB) until it fits.
    size_t want = b->cap ? b->cap * 2 : 8192;
    while (want < b->len + extra + 1)
        want *= 2;

    // Grow the backing allocation to the new capacity.
    char* p = realloc(b->data, want);
    if (!p)
        return viuer_set_error("Out of memory while building output");

    // Commit the new buffer and capacity.
    b->data = p;
    b->cap = want;
    return 0;
}

static int buf_append(Buf* b, const char* s, size_t n)
{
    if (buf_reserve(b, n) != 0)
        return -1;
    memcpy(b->data + b->len, s, n);
    b->len += n;
    b->data[b->len] = '\0';
    return 0;
}

static int buf_puts(Buf* b, const char* s) { return buf_append(b, s, strlen(s)); }

// Formats into the buffer. Only ever used for escape sequences, which are a
// handful of bytes, so a small fixed scratch area is enough and an overflow
// would mean a bug rather than large input.
static int buf_printf(Buf* b, const char* fmt, ...)
{
    char tmp[160];
    va_list ap;
    va_start(ap, fmt);
    int n = vsnprintf(tmp, sizeof tmp, fmt, ap);
    va_end(ap);
    if (n < 0 || (size_t)n >= sizeof tmp)
        return viuer_set_error("Internal: Escape sequence too long");
    return buf_append(b, tmp, (size_t)n);
}

// Pushes the buffer to stdout and empties it.
// A closed output pipe (EPIPE, as in `prog | head`) is reported to the caller
// rather than treated as fatal here, so the caller can decide whether to stop
// quietly or complain.
static int buf_flush(Buf* b)
{
    if (b->len == 0)
        return 0;
    size_t wrote = fwrite(b->data, 1, b->len, stdout);
    if (wrote != b->len || fflush(stdout) != 0)
        return viuer_set_error("Write failed: %s", strerror(errno));
    b->len = 0;
    return 0;
}

#pragma endregion Growable Output Buffer

#pragma region Cursor Placement
/**
 * @brief 2 positioning modes: Relative & Absolute
 * Absolute treats the whole terminal as coordinates, top-left being (0,0).
 * Relative treats the offset as a nudge from wherever the cursor already sits.
 * @param b buffer to write to
 * @param cfg current config
 * @return int whether move was successful
 */
static int apply_offset(Buf* b, const ViuerConfig* cfg)
{
    // Absolute mode: move to the top-left corner of the image
    if (cfg->absolute_offset) {
        int row = cfg->pos.y < 0 ? 0 : cfg->pos.y;
        return buf_printf(b, "\x1b[%d;%dH", row + 1, cfg->pos.x + 1);
    }

    // Relative mode: move to the cursor's current position plus the offset
    if (cfg->pos.y > 0 && buf_printf(b, "\x1b[%dB", cfg->pos.y) != 0) return -1;   // down  
    if (cfg->pos.y < 0 && buf_printf(b, "\x1b[%dA", -cfg->pos.y) != 0) return -1;  // up    
    if (cfg->pos.x > 0 && buf_printf(b, "\x1b[%dC", cfg->pos.x) != 0) return -1;   // right
    return 0;
}

/**
 * @brief Positions the cursor at the start of the row of the image, counting from 0 at the top.
 * 
 * Absolute Mode: each row is addressed by coordinate. 
 * Avoids the newline at the bottom edge of the screen that would scroll the whole display, 
 * means a partially off-screen image degrades by clipping rather than by dragging everything upward!
 *
 * Relative Mode: Carriage Return + Line Feed moves down, and the horizontal offset is re-applied 
 * so a non-zero x indents every row rather than just the first. 
 * @param b  buffer to write to
 * @param cfg current config
 * @param row_index current row index
 * @return int  whether move was successful
 */
static int position_row(Buf* b, const ViuerConfig* cfg, int row_index)
{
    // Absolute mode: move to the top-left corner of the image
    if (cfg->absolute_offset) {
        int row = (cfg->pos.y < 0 ? 0 : cfg->pos.y) + row_index;
        return buf_printf(b, "\x1b[%d;%dH", row + 1, cfg->pos.x + 1);
    }

    // Relative mode: move to the cursor's current position plus the offset
    if (row_index == 0) 
        return apply_offset(b, cfg);
    if (buf_puts(b, "\r\n") != 0) 
        return -1;
    if (cfg->pos.x > 0) 
        return buf_printf(b, "\x1b[%dC", cfg->pos.x);
    return 0;
}
#pragma endregion Cursor Placement

#pragma region Unicode Half-blocks
/**
 * @brief Maps a 24-bit color onto the 256-color palette, 
 * used when the terminal does not support truecolor.
 * 
 * The palette's upper region is a 6x6x6 color cube at indices 16 to 231,
 * followed by a 24-step grey ramp at 232 to 255. 
 * Greys are routed to the ramp because it is much finer than the three grey-ish points the cube offers, 
 * and banding in a greyscale image is the most visible kind.
 */
static int rgb_to_ansi256(int r, int g, int b)
{
    if (r == g && g == b) {
        if (r < 8) return 16;
        if (r > 248) return 231;
        return 232 + (r - 8) * 24 / 247;
    }
    return 16 + 36 * (r * 5 / 255) + 6 * (g * 5 / 255) + (b * 5 / 255);
}

// Appends a "set foreground" or "set background" escape for one pixel, in
// whichever color depth the terminal supports.
static int put_color(Buf* b, bool truecolor, bool foreground, const unsigned char* px) 
{
    int layer = foreground ? 38 : 48;
    if (truecolor)
        return buf_printf(b, "\x1b[%d;2;%d;%d;%dm", layer, px[0], px[1], px[2]);
    return buf_printf(b, "\x1b[%d;5;%dm", layer, rgb_to_ansi256(px[0], px[1], px[2]));
}

// A pixel counts as see-through only when transparency was requested;
// otherwise alpha is ignored and the color is drawn as-is.
#define ALPHA_CUTOFF 16
static bool is_clear(const ViuerConfig* cfg, const unsigned char* px)
{
    return cfg->transparent && px[3] < ALPHA_CUTOFF;
}
#pragma endregion Unicode Half-blocks

#pragma region Printers
/**
 * @brief Logic lifted from viuer.r
 * tmux swallows escape sequences it doesn't recognise (Kitty/iTerm graphics included),
 * so they must be wrapped in a DCS passthrough to reach the real terminal:
 * `ESC P tmux ; <payload> ESC \`, with each embedded ESC doubled so it isn't mistaken for the closing `ESC \`.
 * CSI sequences (cursor/color) are left unwrapped since tmux needs to track those itself,
 * and nothing is wrapped outside tmux. Each graphics escape gets its own envelope rather than batching,
 * since tmux reads one DCS sequence at a time and long ones can be truncated.
 * @param b buffer to write to
 * @param seq sequence to wrap
 * @param len length of sequence
 * @return int whether wrap was successful
 */
static int emit_graphics_escape(Buf* b, const char* seq, size_t len)
{
    if (!viuer_is_tmux())
        return buf_append(b, seq, len);

    if (buf_puts(b, "\x1bPtmux;") != 0) return -1;

    // Copy in runs, doubling each ESC. Scanning for the next ESC and appending
    // the whole span before it keeps this to a few memcpy calls per sequence
    // rather than a function call per byte
    size_t start = 0;
    for (size_t i = 0; i < len; i++) {
        if (seq[i] != '\x1b') 
            continue;
        if (buf_append(b, seq + start, i - start) != 0) 
            return -1;
        if (buf_puts(b, "\x1b\x1b") != 0) 
            return -1;
        start = i + 1;
    }
    if (buf_append(b, seq + start, len - start) != 0) 
    return -1;

    return buf_puts(b, "\x1b\\");
}

/**
 * @brief Streams raw RGBA pixels to the terminal via the Kitty graphics protocol
 * (APC sequences: ESC _ G <key=value,...> ; <base64 payload> ESC \), which
 * scales them into a box of cols x rows character cells and composites
 * them over the text grid
 * 
 * @param img Image to draw
 * @param cfg Rendering config
 * @param out_size Final rendered size
 * @return int 0 on success, -1 on failure
 */
static int print_kitty(const Image* img, const ViuerConfig* cfg, Vector2 char_size) {
    size_t raw_len = (size_t)img->size.x * img->size.y * 4;
    size_t b64_len = BASE64_ENCODED_LEN(raw_len);
    char* b64 = malloc(b64_len + 1);
    if (!b64) 
        return viuer_set_error("Out of memory!");
    base64_encode(img->pixels, raw_len, b64);

    Buf out;   // what is finally written to the terminal
    Buf seq;   // one complete escape sequence, before tmux wrapping
    buf_init(&out);
    buf_init(&seq);

    int rc = 0;

    // Delete the sprite's previous placement first: reusing its id is not
    // guaranteed to clear a placement of a different size.
    if (cfg->image_id > 0) {
        rc = buf_printf(&seq, "\x1b_Ga=d,d=i,i=%d,q=2;\x1b\\", cfg->image_id);
        if (rc == 0) rc = emit_graphics_escape(&out, seq.data, seq.len);
        seq.len = 0;
    }

    if (rc == 0) 
        rc = apply_offset(&out, cfg);

    // i/p (image/placement id) are only sent for a reusable placement.
    char id_keys[48] = "";
    if (cfg->image_id > 0)
        snprintf(id_keys, sizeof id_keys, ",i=%d,p=%d", cfg->image_id, cfg->image_id);

    // a=T transmit+display, f=32 RGBA, s/v payload size, c/r cell size,
    // C=1 no cursor move, q=2 no ack, m=1/0 more-chunks flag. Chunked to a
    // few KB so each tmux passthrough envelope survives intact.
    const size_t CHUNK = 4096;
    size_t sent = 0;
    bool first = true;
    while (rc == 0 && sent < b64_len) {
        size_t n = b64_len - sent < CHUNK ? b64_len - sent : CHUNK;
        bool last = (sent + n == b64_len);

        seq.len = 0; // reuse the allocation across chunks
        if (first) {
            rc = buf_printf(&seq,
                            "\x1b_Ga=T,f=32,s=%d,v=%d,c=%d,r=%d,C=1,q=2%s,m=%d;",
                            img->size.x, img->size.y, char_size.x, char_size.y,
                            id_keys, last ? 0 : 1);
            first = false;
        } else {
            rc = buf_printf(&seq, "\x1b_Gm=%d;", last ? 0 : 1);
        }
        if (rc == 0) rc = buf_append(&seq, b64 + sent, n);
        if (rc == 0) rc = buf_puts(&seq, "\x1b\\");

        // Wrap for tmux if needed, then queue for output.
        if (rc == 0) rc = emit_graphics_escape(&out, seq.data, seq.len);
        sent += n;
    }

    // Emit the line feeds C=1 suppressed, so cursor math stays exact
    // instead of depending on the terminal's own rounding. Skipped in
    // absolute mode, where a feed at the bottom edge would scroll.
    if (!cfg->absolute_offset)
        for (int i = 0; rc == 0 && i < char_size.y; i++) rc = buf_puts(&out, "\r\n");

    if (rc == 0) 
        rc = buf_flush(&out);
    buf_free(&seq);
    buf_free(&out);
    free(b64);
    return rc;
}

/**
 * @brief Draws two stacked pixels per cell using the half-block glyphs U+2584/U+2580,
 * doubling the vertical resolution the character grid would otherwise give.
 * `img` must already be scaled to exactly cols x (rows*2) pixels.
 * 
 * @param img image to draw
 * @param cfg rendering config
 * @param char_size final rendered size
 * @return int 0 on success, -1 on failure
 */
static int print_blocks(const Image* img, const ViuerConfig* cfg, Vector2 char_size)
{
    static const char LOWER_HALF[] = "\xe2\x96\x84"; /* U+2584 */
    static const char UPPER_HALF[] = "\xe2\x96\x80"; /* U+2580 */
    bool truecolor = viuer_is_truecolor();

    Buf b;
    buf_init(&b);
    int rc = 0;

    for (int row = 0; rc == 0 && row < char_size.y; row++) {
        rc = position_row(&b, cfg, row);

        int top_y = row * 2;
        int bot_y = top_y + 1;

        for (int col = 0; rc == 0 && col < char_size.x; col++) {
            const unsigned char *top =
                img->pixels + ((size_t)top_y * img->size.x + col) * 4;
            // An odd pixel height leaves the final row with no bottom pixel.
            const unsigned char *bot =
                (bot_y < img->size.y)
                    ? img->pixels + ((size_t)bot_y * img->size.x + col) * 4
                    : NULL;

            bool top_clear = is_clear(cfg, top);
            bool bot_clear = (bot == NULL) || is_clear(cfg, bot);

            if (top_clear && bot_clear) {
                // Both transparent: reset and leave the cell blank.
                rc = buf_puts(&b, "\x1b[0m ");
            } else if (bot_clear) {
                // Top only: upper half block on the default background.
                if (rc == 0) rc = buf_puts(&b, "\x1b[49m");
                if (rc == 0) rc = put_color(&b, truecolor, true, top);
                if (rc == 0) rc = buf_puts(&b, UPPER_HALF);
            } else if (top_clear) {
                // Bottom only: lower half block on the default background.
                if (rc == 0) rc = buf_puts(&b, "\x1b[49m");
                if (rc == 0) rc = put_color(&b, truecolor, true, bot);
                if (rc == 0) rc = buf_puts(&b, LOWER_HALF);
            } else {
                // Both opaque: top as background, bottom as foreground, one glyph.
                if (rc == 0) rc = put_color(&b, truecolor, false, top);
                if (rc == 0) rc = put_color(&b, truecolor, true, bot);
                if (rc == 0) rc = buf_puts(&b, LOWER_HALF);
            }
        }

        // Reset before leaving the row so the last cell's background doesn't bleed.
        if (rc == 0) rc = buf_puts(&b, "\x1b[0m");
    }

    // Relative mode: end on a fresh line so later output doesn't overwrite the image.
    if (rc == 0 && !cfg->absolute_offset) rc = buf_puts(&b, "\r\n");

    if (rc == 0) rc = buf_flush(&b);
    buf_free(&b);
    return rc;
}

int viuer_print(const Image* img, const ViuerConfig* cfg, Vector2* out_size)
{
    if (!img)
        return viuer_set_error("Print: No image");

    Vector2 render_size = VECTOR2_ZERO;
    viuer_fit_dimensions(img->size, cfg, &render_size);

    int rc;
    // Choose the right renderer based on the config
    if (cfg->render_type == KITTY && viuer_is_kitty_supported()) {
        rc = print_kitty(img, cfg, render_size);
    } else {
        // Half-blocks. iTerm is not handled here because it wants an encoded file
        // This entry point only has raw pixels! So we handle resize + printing here
        // render_size is in character cells; each cell is 2 pixels tall, so the
        // pixel buffer handed to print_blocks must be twice as tall.
        Vector2 pixel_size = { render_size.x, render_size.y * CELL_PIXEL_RATIO };
        Image* scaled = viuer_resize(img, pixel_size);
        if (!scaled)
            return -1;
        rc = print_blocks(scaled, cfg, render_size);
        viuer_image_free(scaled);
    }

    if (render_size.x > 0)
        out_size->x = render_size.x;
    if (render_size.y > 0)
        out_size->y = render_size.y;
    return rc;
}

int viuer_print_from_file(const char* path, const ViuerConfig* cfg, Vector2* out_size)
{
    FILE *f = fopen(path, "rb");
    if (!f) 
        return viuer_set_error("%s: %s", path, strerror(errno));

    // The whole file is read in one go: the decoder needs a contiguous buffer
    if (fseek(f, 0, SEEK_END) != 0) { 
        fclose(f); 
        return viuer_set_error("%s: not seekable", path);
    }

    // Get the file size
    long size = ftell(f);
    if (size < 0) { 
        fclose(f); 
        return viuer_set_error("%s: %s", path, strerror(errno));
    }
    rewind(f); // rewind to the beginning

    // Allocate memory for the file
    unsigned char *data = malloc((size_t)size + 1);
    if (!data) { 
        fclose(f); 
        return viuer_set_error("Out of memory!"); 
    }

    // Read the file
    size_t got = fread(data, 1, (size_t)size, f);
    fclose(f);
    if (got != (size_t)size) { 
        free(data); 
        return viuer_set_error("%s: short read", path); 
    }

    // Decode the image
    Image *img = viuer_image_load_memory(data, got);
    if (!img) { 
        free(data); 
        return -1; 
    }

    // Print it
    int rc;
    if (cfg->render_type == ITERM && viuer_is_iterm_supported() && !viuer_is_kitty_supported()) {
        // Hand the file over untouched and let the terminal do the work
        rc = viuer_print_iterm_bytes(data, got, cfg, img->size, out_size);
    } else {
        // Do the work ourselves
        rc = viuer_print(img, cfg, out_size);
    }

    // Free the image
    viuer_image_free(img);
    free(data);
    return rc;
}

// Lifted directly from viuer.r
int viuer_print_iterm_bytes(const unsigned char* data, size_t len, const ViuerConfig* cfg, Vector2 img_size, Vector2* out_size)
{
    // Compute the terminal cell size the image should be drawn at
    Vector2 char_size = VECTOR2_ZERO;
    viuer_fit_dimensions(img_size, cfg, &char_size);

    // Base64-encode the raw file bytes for the OSC payload
    size_t b64_len = BASE64_ENCODED_LEN(len);
    char *b64 = malloc(b64_len + 1);
    if (!b64) 
        return viuer_set_error("Out of memory!");
    base64_encode(data, len, b64);

    // Compute the terminal cell size the image should be drawn at
    Buf out, seq;
    buf_init(&out);
    buf_init(&seq);

    // Position the cursor before drawing, per the configured offset
    int rc = apply_offset(&out, cfg);

    // Build the iTerm2 OSC 1337 "File=" escape sequence header
    if (rc == 0)
        rc = buf_printf(&seq,
                        "\x1b]1337;File=inline=1;size=%zu;width=%dc;height=%dc;"
                        "preserveAspectRatio=1:", len, char_size.x, char_size.y);
    if (rc == 0) rc = buf_append(&seq, b64, b64_len);
    if (rc == 0) rc = buf_puts(&seq, "\a");

    // Whole OSC sequence goes out as one unit (tmux-wrapped if needed);
    // it cannot be split since the terminal needs the full payload to decode
    if (rc == 0) rc = emit_graphics_escape(&out, seq.data, seq.len);

    // Move past the image's last row, unless absolute-offset mode is on
    // (same reasoning as the Kitty path)
    if (rc == 0 && !cfg->absolute_offset) rc = buf_puts(&out, "\r\n");
    if (rc == 0) rc = buf_flush(&out);

    // Release scratch buffers and the base64 copy
    buf_free(&seq);
    buf_free(&out);
    free(b64);

    // Report back the cell dimensions actually used
    if (out_size)
        *out_size = char_size;
    return rc;
}

// What this does depends on how the image was drawn, which is why it takes the
// config rather than just a rectangle:
//  (1) with a pixel-graphics placement (a non-zero image_id under Kitty), the
//     placement is deleted by id. Painting spaces over it would not remove it,
//     because the placement is layered above the text grid rather than stored
//     in it.
//  (2) otherwise the rectangle is overwritten with blanks at the default
//     background color.
//  Erase only the region that actually needs clearing -- the footprint of the
//  * previous frame, or just the part of it the next frame will not cover. Wiping
//  * the whole screen between frames forces the terminal to repaint everything and
//  * shows as flicker.
int viuer_erase(const ViuerConfig* cfg, Vector2* size)
{
    // Nothing to erase
    if (size->x <= 0 || size->y <= 0)
        return 0;

    // Init
    Buf out, seq;
    buf_init(&out);
    buf_init(&seq);
    int rc = 0;

    // (1) Pixel-graphics placement (object above the text grid, not contents of cell)
    if (cfg->image_id > 0 && cfg->render_type == KITTY && viuer_is_kitty_supported()) {
        // Delete the placement by id instead of painted over
        // d=i selects deletion by image id, so we don't leave sprites floating where they were
        int rc = buf_printf(&out, "\x1b_Ga=d,d=i,i=%d,q=2;\x1b\\", cfg->image_id);
        if (rc == 0) rc = emit_graphics_escape(&out, seq.data, seq.len);
        if (rc == 0) rc = buf_flush(&out);
        buf_free(&seq);
        buf_free(&out);
        return rc;
    }

    // (2) Half-block output, actual cell contents, so blanks erase it
    char* blanks = malloc((size_t)size->x + 1);
    if (!blanks) 
        return viuer_set_error("Out of memory!");
    memset(blanks, ' ', (size_t)size->x);
    blanks[size->x] = '\0';

    // Reset attributes once up front: the blanks must land on the default background,
    // not on whatever color was last set
    rc = buf_puts(&out, "\x1b[0m");
    for (int row = 0; rc == 0 && row < size->y; row++) {
        rc = position_row(&out, cfg, row);
        if (rc == 0) rc = buf_append(&out, blanks, (size_t)size->x);
    }
    if (rc == 0) 
        rc = buf_flush(&out);

    free(blanks);
    buf_free(&seq);
    buf_free(&out);
    return rc;
}
#pragma endregion Printers

#pragma region Error Reporting
// One error message per thread.
// Means 2 threads rendering at once will never overwrite each other's message!
static thread_local char viu_error[512] = "No Error.";
const char* viuer_last_error(void) 
{
    return viu_error;
}
int viuer_set_error(const char* fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(viu_error, sizeof viu_error, fmt, ap);
    va_end(ap);
    return -1; // lets callers write return viu_set_error(...) in one line 
}
#pragma endregion Error Reporting