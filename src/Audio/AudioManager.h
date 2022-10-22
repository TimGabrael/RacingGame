#pragma once
#include <vector>

#define NUM_CONCURRENT_AUDIO_STREAMS 20


enum AUDIO_FILE
{
	NUM_AVAILABLE_AUDIO_FILES,
};


struct AudioManager* AU_CreateAudioManager();
void AU_ShutdownAudioManager(struct AudioManager* manager);

struct WavFile* AU_LoadFile(struct AudioManager* manager, const char* file);
struct OggFile* AU_LoadOggFile(struct AudioManager* manager, const char* file);
void AU_FreeFile(struct AudioManager* manager, struct WavFile* file);
void AU_FreeOggFile(struct AudioManager* manager, struct OggFile* file);


struct AudioPlaybackContext* AU_PlayAudio(struct AudioManager* manager, struct WavFile* file, float volume = 1.0f);
struct AudioPlaybackContext* AU_PlayOggAudio(struct AudioManager* manager, struct OggFile* file, float volume = 1.0f);
struct AudioPlaybackContext* AU_PlayAudioOnRepeat(struct AudioManager* manager, struct WavFile* file, float volume = 1.0f);
struct AudioPlaybackContext* AU_PlayOggAudioOnRepeat(struct AudioManager* manager, struct OggFile* file, float volume = 1.0f);
void AU_StopAudio(struct AudioManager* manager, struct AudioPlaybackContext* audioCtx);		// will free the audio context to be used by other samples
void AU_PauseAudio(struct AudioManager* manager, struct AudioPlaybackContext* audioCtx);		// will keep the audio context alive
void AU_ResumeAudio(struct AudioManager* manager, struct AudioPlaybackContext* audioCtx);