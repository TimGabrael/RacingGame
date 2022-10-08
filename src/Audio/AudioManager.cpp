#include "AudioManager.h"
#include "NFDriver/NFDriver.h"
#include <atomic>
#undef min
#undef max

using namespace nativeformat;
using namespace driver;

struct AudioPlaybackContext
{
	WavFile* file = nullptr;
	int index = 0;
	int remaining = 0;
	float volume = 1.0f;
	bool repeat = false;
	bool isPlaying = false;
	bool inUse = false;
};

struct AudioManager
{
	NFDriver* nfdriver = nullptr;
	std::vector<WavFile> stored;
	AudioPlaybackContext active[NUM_CONCURRENT_AUDIO_STREAMS];
	std::atomic<int> currentNumPlaying = 0;
};



static void AudioStutterCallback(void* clientData)
{
}
static int AudioRenderCallback(void* clientData, float* frames, int numberOfFrames)
{
	AudioManager* manager = (AudioManager*)clientData;
	memset(frames, 0, sizeof(float) * numberOfFrames * 2);
	if (manager->currentNumPlaying > 0)
	{
		float max = 0.0f;
		for (int i = 0; i < NUM_CONCURRENT_AUDIO_STREAMS; i++)
		{
			AudioPlaybackContext* cur = &manager->active[i];
			if (cur->isPlaying && cur->file)
			{
				const float* data = cur->file->GetData();
				const int end = std::min(numberOfFrames, cur->remaining);
				for (int j = 0; j < end; j++)
				{
					frames[j * 2] += data[(j + cur->index) * 2] * cur->volume;
					frames[j * 2 + 1] += data[(j + cur->index) * 2 + 1] * cur->volume;
				}
				cur->index += end;
				cur->remaining -= end;
				if (cur->remaining <= 0)
				{
					if (cur->repeat)
					{
						cur->index = 0;
						cur->remaining = cur->file->GetNumSamples();
					}
					else
					{
						cur->isPlaying = false;
						cur->inUse = false;
						manager->currentNumPlaying -= 1;
					}
				}
			}
		}
		for (int i = 0; i < numberOfFrames * 2; i++)
		{
			if (std::abs(frames[i]) > max) max = std::abs(frames[i]);
		}
		if (max > 1.0f)
		{
			for (int i = 0; i < numberOfFrames * 2; i++)
			{
				frames[i] /= max;
			}
		}
	}
	return numberOfFrames;
}
static void AudioErrorCallback(void* clientData, const char* errorMessage, int errorCode)
{

}
static void AudioWillRenderCallback(void* clientData)
{

}
static void AudioDidRenderCallback(void* clientData)
{

}

struct AudioManager* AU_CreateAudioManager()
{
	AudioManager* out = new AudioManager;
	out = new AudioManager;
	memset(out->active, 0, sizeof(AudioPlaybackContext));
	NFDriver* driver = NFDriver::createNFDriver(out,
			AudioStutterCallback,AudioRenderCallback,
			AudioErrorCallback, AudioWillRenderCallback,
			AudioDidRenderCallback, OutputType::OutputTypeSoundCard);

	out->nfdriver = driver;
	out->currentNumPlaying = 0;
	out->stored.resize(NUM_AVAILABLE_AUDIO_FILES);
	out->nfdriver->setPlaying(true);
}
void AU_ShutdownAudioManager(struct AudioManager* manager)
{
	manager->nfdriver->setPlaying(false);
	manager->currentNumPlaying = 0;
	delete manager->nfdriver;
	manager->nfdriver = nullptr;
	delete manager;
	manager = nullptr;
}

AudioPlaybackContext* AU_PlayAudio(struct AudioManager* manager, AUDIO_FILE audio, float volume)
{
	if (audio >= NUM_AVAILABLE_AUDIO_FILES && audio < 0) return nullptr;

	if (manager->currentNumPlaying >= NUM_CONCURRENT_AUDIO_STREAMS) return nullptr;

	AudioPlaybackContext* result = nullptr;
	for (int i = 0; i < NUM_CONCURRENT_AUDIO_STREAMS; i++)
	{
		AudioPlaybackContext* cur = &manager->active[i];
		if (!cur->inUse)
		{
			cur->file = &manager->stored.at(audio);
			cur->remaining = cur->file->GetNumSamples();
			cur->index = 0;
			cur->volume = volume;
			cur->inUse = true;
			result = cur;
			cur->isPlaying = true;
			break;
		}
	}
	manager->currentNumPlaying += 1;
	return result;
}
struct AudioPlaybackContext* AU_PlayAudioOnRepeat(struct AudioManager* manager, AUDIO_FILE audio, float volume)
{
	if (audio >= NUM_AVAILABLE_AUDIO_FILES && audio < 0) return nullptr;
	if (manager->currentNumPlaying >= NUM_CONCURRENT_AUDIO_STREAMS) return nullptr;

	AudioPlaybackContext* result = nullptr;
	for (int i = 0; i < NUM_CONCURRENT_AUDIO_STREAMS; i++)
	{
		AudioPlaybackContext* cur = &manager->active[i];
		if (!cur->inUse)
		{
			cur->file = &manager->stored.at(audio);
			cur->remaining = cur->file->GetNumSamples();
			cur->index = 0;
			cur->volume = volume;
			cur->repeat = true;
			cur->inUse = true;
			result = cur;
			cur->isPlaying = true;
			break;
		}
	}
	if (result)
	{
		manager->currentNumPlaying += 1;
		return result;
	}
	return nullptr;
}
void AU_StopAudio(struct AudioManager* manager, struct AudioPlaybackContext* audioCtx)
{
	AU_PauseAudio(manager, audioCtx);
	audioCtx->inUse = false;
}
void AU_PauseAudio(struct AudioManager* manager, struct AudioPlaybackContext* audioCtx)
{
	if (audioCtx->isPlaying)
	{
		audioCtx->isPlaying = false;
		manager->currentNumPlaying -= 1;
	}
}
void AU_ResumeAudio(struct AudioManager* manager, struct AudioPlaybackContext* audioCtx)
{
	if (!audioCtx->isPlaying)
	{
		audioCtx->isPlaying = true;
		manager->currentNumPlaying += 1;
	}

}