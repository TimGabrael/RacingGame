#pragma once
#include <vector>
#include "WavFile.h"

#define NUM_CONCURRENT_AUDIO_STREAMS 20


enum AUDIO_FILE
{
	NUM_AVAILABLE_AUDIO_FILES,
};


struct AudioManager* AU_CreateAudioManager();
void AU_ShutdownAudioManager(struct AudioManager* manager);

struct AudioPlaybackContext* AU_PlayAudio(struct AudioManager* manager, AUDIO_FILE audio, float volume = 1.0f);
struct AudioPlaybackContext* AU_PlayAudioOnRepeat(struct AudioManager* manager, AUDIO_FILE audio, float volume = 1.0f);
void AU_StopAudio(struct AudioManager* manager, struct AudioPlaybackContext* audioCtx);		// will free the audio context to be used by other samples
void AU_PauseAudio(struct AudioManager* manager, struct AudioPlaybackContext* audioCtx);		// will keep the audio context alive
void AU_ResumeAudio(struct AudioManager* manager, struct AudioPlaybackContext* audioCtx);