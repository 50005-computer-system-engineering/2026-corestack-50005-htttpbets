#include <raylib.h>
#include <raymath.h>
#include <stdio.h>
#include "audio.h"
#include "config.h"
#include "events.h"
#include "lib/libeventbus.h"
#include "camera.h"

Music bgm_tracks[BGM_COUNT] = {0};
Sound sfx_tracks[SFX_COUNT] = {0};

Music* curr_bgm = NULL;

static void set_sound_position(Camera2D listener, Sound sound, Vector2 sound_pos, float max_dist)
{
    max_dist *= CONFIG.PHYSICS.TILE_SIZE;

    // Vector from listener to sound source
    Vector2 dir = Vector2Subtract(Vector2Scale(sound_pos, CONFIG.PHYSICS.TILE_SIZE), listener.target);
    float distance = Vector2Length(dir);

    // Calculate volume attenuation (clamp between 0.0f and 1.0f)
    float volume = 1.0f / (1.0f + (distance / max_dist));
    if (volume > 1.0f)
        volume = 1.0f;
    if (volume < 0.0f)
        volume = 0.0f;

    // Calculate stereo pan based on horizontal X offset (-maxDist to +maxDist)
    float pan_offset = dir.x / max_dist;
    float pan = 0.5f + (pan_offset * 0.5f);
    if (pan > 1.0f)
        pan = 1.0f;
    if (pan < 0.0f)
        pan = 0.0f;

    // Apply values to raylib sound
    SetSoundVolume(sound, volume);
    SetSoundPan(sound, pan);
}

void on_bomb_exploded(void* args)
{
    TileEventArgs* a = (TileEventArgs*)args;
    set_sound_position(camera, sfx_tracks[SFX_BOMB], (Vector2){a->x, a->y}, 10.0f);
    play_sound(SFX_BOMB);
}

void audio_init()
{
    InitAudioDevice();

    // Get the current working directory
    const char* current_dir = GetWorkingDirectory();

    // Print it to the console/terminal
    printf("Current working directory: %s\n", current_dir);

    // Initialise BGM tracks
    bgm_tracks[BGM_TITLE] = LoadMusicStream("../../assets/audio/bgm/01_Title.mp3");
    bgm_tracks[BGM_PREPARE] = LoadMusicStream("../../assets/audio/bgm/02_GetReady.mp3");
    bgm_tracks[BGM_BATTLE] = LoadMusicStream("../../assets/audio/bgm/03_Battle.mp3");
    bgm_tracks[BGM_WIN] = LoadMusicStream("../../assets/audio/bgm/04_BattleWin.mp3");
    bgm_tracks[BGM_LOSE] = LoadMusicStream("../../assets/audio/bgm/04_BattleLose.mp3");

    // Initialise SFX tracks
    sfx_tracks[SFX_PLACEBOMB] = LoadSound("../../assets/audio/sfx/bomb_place.wav");
    sfx_tracks[SFX_BOMB] = LoadSound("../../assets/audio/sfx/bomb_alt.wav");
    sfx_tracks[SFX_POWERUP_BOMB] = LoadSound("../../assets/audio/sfx/powerup_bomb.wav");
    sfx_tracks[SFX_POWERUP_FIRE] = LoadSound("../../assets/audio/sfx/powerup_fire.wav");

    // Set volumes
    for (int i = 0; i < BGM_COUNT; i++)
        SetMusicVolume(bgm_tracks[i], CONFIG.SETTINGS.MUSIC_VOLUME);
    for (int i = 0; i < SFX_COUNT; i++)
        SetSoundVolume(sfx_tracks[i], CONFIG.SETTINGS.SFX_VOLUME);

    // Listen for events
    event_bus_listen(EVENT_BOMB_EXPLODED, on_bomb_exploded);
}

void audio_update()
{
    if (curr_bgm != NULL)
        UpdateMusicStream(*curr_bgm);
}

void play_sound(SFX sfx)
{
    PlaySound(sfx_tracks[sfx]);
}

void play_bgm(BGM bgm)
{
    if (curr_bgm != NULL)
        StopMusicStream(*curr_bgm);
    curr_bgm = &bgm_tracks[bgm];
    PlayMusicStream(*curr_bgm);
}

void audio_cleanup()
{
    // Unload all bgm
    for (int i = 0; i < BGM_COUNT; i++)
        UnloadMusicStream(bgm_tracks[i]);

    // Unload all sfx
    for (int i = 0; i < SFX_COUNT; i++)
        UnloadSound(sfx_tracks[i]);

    event_bus_stop_listening(EVENT_BOMB_EXPLODED, on_bomb_exploded);

    CloseAudioDevice();
}
