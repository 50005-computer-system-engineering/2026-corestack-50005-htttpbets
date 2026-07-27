#ifndef BOMBERMAN_H
#define BOMBERMAN_H
#include <raylib.h>
#include "lib/libbombbrain/collision.h"
#include "spritesheet.h"

typedef struct {
    int direction;
    bool is_moving;
    Spritesheet* curr_sprite;
    BoundBox box;
} Bomberman;

void bomberman_init(void);
Bomberman bomberman_create_default(Vector2 position);
void bomberman_update(Bomberman* bm);
void bomberman_draw(Bomberman* bm);
void bomberman_cleanup(void);
#endif