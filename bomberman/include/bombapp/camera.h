#ifndef CAMERA_H
#define CAMERA_H
#include <raylib.h>

extern Camera2D camera;

void camera_init(Vector2 target);
void camera_update(Vector2 target);
#endif