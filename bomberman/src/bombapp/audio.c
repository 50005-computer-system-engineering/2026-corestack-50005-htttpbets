#include <raylib.h>
#include <stdio.h>
#include "audio.h"
#include "config.h"

Music bgm_tracks[BGM_COUNT] = {0};
Sound sfx_tracks[SFX_COUNT] = {0};

Music* currBgm = NULL;

void audio_init() {
    InitAudioDevice();

    // Get the current working directory
    const char *currentDir = GetWorkingDirectory();
    
    // Print it to the console/terminal
    printf("Current working directory: %s\n", currentDir);

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
}
    

void audio_update() {
    if (currBgm != NULL)
        UpdateMusicStream(*currBgm);
}

void play_sound(SFX sfx) {
    PlaySound(sfx_tracks[sfx]);
}

void play_bgm(BGM bgm) {
    if (currBgm != NULL)
        StopMusicStream(*currBgm);
    currBgm = &bgm_tracks[bgm];
    PlayMusicStream(*currBgm);
}

void audio_cleanup() {
    // Unload all bgm
    for (int i = 0; i < BGM_COUNT; i++)
        UnloadMusicStream(bgm_tracks[i]);
    
    // Unload all sfx
    for (int i = 0; i < SFX_COUNT; i++)
        UnloadSound(sfx_tracks[i]);

    CloseAudioDevice();
}