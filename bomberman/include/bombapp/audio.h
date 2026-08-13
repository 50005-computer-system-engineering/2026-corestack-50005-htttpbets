#ifndef BOMBAPP_AUDIO_H
#define BOMBAPP_AUDIO_H
#include "raylib.h"

typedef enum {
    BGM_TITLE,
    BGM_PREPARE,
    BGM_BATTLE,
    BGM_WIN,
    BGM_LOSE,
    BGM_COUNT
} BGM;

typedef enum {
    SFX_PLACEBOMB,
    SFX_BOMB,
    SFX_POWERUP_BOMB,
    SFX_POWERUP_FIRE,
    SFX_COUNT
} SFX;

void audio_init();
void audio_update();
void play_sound(SFX sound);
void play_bgm(BGM music);
void audio_cleanup();

#endif
