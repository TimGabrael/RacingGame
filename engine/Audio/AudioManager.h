#pragma once


struct AudioManager* AU_CreateAudioManager(int numConcurrent);
void AU_ShutdownAudioManager(struct AudioManager* manager);

struct WavFile* AU_LoadFile(struct AudioManager* manager, const char* file);
struct OggFile* AU_LoadOggFile(struct AudioManager* manager, const char* file);

void AU_FreeFile(struct AudioManager* manager, struct WavFile* file);
void AU_FreeOggFile(struct AudioManager* manager, struct OggFile* file);


struct AudioPlaybackContextList* AU_CreateAudioPlaybackContextList(int num);
void AU_DestroyAudioPlaybackContextList(struct AudioPlaybackContextList* list);
struct AudioPlaybackContext* AU_AllocContext(struct AudioPlaybackContextList* list, int num);
void AU_FreeContext(struct AudioManager* manager, struct AudioPlaybackContext* ctx);

bool AU_PlayAudio(struct AudioManager* manager, struct AudioPlaybackContext* ctx, struct WavFile* file, float volume = 1.0f);
bool AU_PlayOggAudio(struct AudioManager* manager, struct AudioPlaybackContext* ctx, struct OggFile* file, float volume = 1.0f);
bool AU_PlayAudioOnRepeat(struct AudioManager* manager, struct AudioPlaybackContext* ctx, struct WavFile* file, float volume = 1.0f);
bool AU_PlayOggAudioOnRepeat(struct AudioManager* manager, struct AudioPlaybackContext* ctx, struct OggFile* file, float volume = 1.0f);


struct AudioPlaybackContext* AU_PlayAudio(struct AudioManager* manager, struct WavFile* file, float volume = 1.0f);
struct AudioPlaybackContext* AU_PlayOggAudio(struct AudioManager* manager, struct OggFile* file, float volume = 1.0f);
struct AudioPlaybackContext* AU_PlayAudioOnRepeat(struct AudioManager* manager, struct WavFile* file, float volume = 1.0f);
struct AudioPlaybackContext* AU_PlayOggAudioOnRepeat(struct AudioManager* manager, struct OggFile* file, float volume = 1.0f);



void AU_StopAudio(struct AudioManager* manager, struct AudioPlaybackContext* audioCtx);		// will free the audio context to be used by other samples
void AU_PauseAudio(struct AudioManager* manager, struct AudioPlaybackContext* audioCtx);		// will keep the audio context alive
bool AU_ResumeAudio(struct AudioManager* manager, struct AudioPlaybackContext* audioCtx);

bool AU_IsPlaying(struct AudioPlaybackContext* ctx);
bool AU_IsOgg(struct AudioPlaybackContext* ctx);
struct WavFile* AU_GetWavFile(struct AudioPlaybackContext* ctx);
struct stb_vorbis* AU_GetOggFile(struct AudioPlaybackContext* ctx);
int AU_GetSampleCount(struct AudioPlaybackContext* ctx);
int AU_GetSampleIndex(struct AudioPlaybackContext* ctx);
void AU_SetSampleIndex(struct AudioManager* manager, struct AudioPlaybackContext* ctx, int index);
float AU_GetVolume(struct AudioPlaybackContext* ctx);
void AU_SetVolume(struct AudioPlaybackContext* ctx, float volume);
