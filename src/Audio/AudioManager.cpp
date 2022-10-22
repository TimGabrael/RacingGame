#include "AudioManager.h"
#include "WavFile.h"
#include "NFDriver/NFDriver.h"
#include "stb_vorbis.h"
#include <atomic>
#undef min
#undef max


#define NUM_AUDIO_FILES_IN_LIST sizeof(uintptr_t) * 8
using namespace nativeformat;
using namespace driver;

struct AudioPlaybackContext
{
	union File
	{
		WavFile* wav;
		stb_vorbis* ogg;
	}file = { nullptr };
	int index = 0;
	int remaining = 0;
	float volume = 1.0f;
	bool repeat = false;
	bool isPlaying = false;
	bool inUse = false;
	bool isOgg = false;
};

struct AudioFileList
{
	AudioFileList* next;
	uintptr_t fillMask;
	WavFile files[NUM_AUDIO_FILES_IN_LIST];
};

static WavFile* AllocWavFile(AudioFileList* list)
{
	for (uint64_t i = 0; i < NUM_AUDIO_FILES_IN_LIST; i++)
	{
		uint64_t cur = 1LL << i;
		if (!(cur & list->fillMask))
		{
			list->fillMask |= cur;
			memset(&list->files[i], 0, sizeof(WavFile));
			return &list->files[i];
		}
	}
	if (list->next)
	{
		return AllocWavFile(list);
	}
	list->next = new AudioFileList;
	memset(list->next, 0, sizeof(AudioFileList));
	list->next->fillMask = 1;
	return &list->next->files[0];
}
static void DeallocWavFile(AudioFileList* list, WavFile* file)
{
	const size_t idx = (file - &list->files[0]) / sizeof(WavFile);
	if (idx < NUM_AUDIO_FILES_IN_LIST)
	{
		if (list->fillMask & (1LL << idx))
		{
			list->fillMask &= ~(1LL << idx);
			list->files[idx].~WavFile();
			memset(&list->files[idx], 0, sizeof(WavFile));
		}
	}
	else if(list->next)
	{
		DeallocWavFile(list->next, file);
	}
}
static void CleanUpAllInList(AudioFileList* list)
{
	AudioFileList* child = list->next;
	while (child)
	{
		AudioFileList* cur = child;
		child = child->next;
		delete cur;
	}
	for (size_t i = 0; i < NUM_AUDIO_FILES_IN_LIST; i++)
	{
		if (list->fillMask & (1LL << i))
		{
			list->files[i].~WavFile();
		}
	}
	memset(list, 0, sizeof(AudioFileList));
}


struct AudioManager
{
	NFDriver* nfdriver = nullptr;
	float* tempBuffer = nullptr;
	AudioFileList stored;
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
			if (cur->isPlaying && cur->file.wav)
			{
				if (!cur->isOgg)
				{
					const float* data = cur->file.wav->GetData();
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
							cur->remaining = cur->file.wav->GetNumSamples();
						}
						else
						{
							cur->isPlaying = false;
							cur->inUse = false;
							manager->currentNumPlaying -= 1;
						}
					}
				}
				else
				{
					stb_vorbis_seek(cur->file.ogg, cur->index);
					int added = stb_vorbis_get_samples_float_interleaved(cur->file.ogg, 2, manager->tempBuffer, numberOfFrames * 2);
					for (uint32_t i = 0; i < added * 2; i++)
					{
						frames[i] += manager->tempBuffer[i];
					}
					cur->index += added;
					cur->remaining -= added;
					if (added == 0 || cur->remaining <= 0)
					{
						if (cur->repeat)
						{
							cur->index = 0;
							cur->remaining = cur->file.ogg->total_samples;
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
	memset(&out->stored, 0, sizeof(AudioFileList));

	out->tempBuffer = new float[NF_DRIVER_SAMPLE_BLOCK_SIZE * 2];

	out->nfdriver->setPlaying(true);
	return out;
}
void AU_ShutdownAudioManager(struct AudioManager* manager)
{
	manager->nfdriver->setPlaying(false);
	manager->currentNumPlaying = 0;
	CleanUpAllInList(&manager->stored);
	delete manager->nfdriver;
	manager->nfdriver = nullptr;
	delete manager;
	manager = nullptr;
}

struct WavFile* AU_LoadFile(struct AudioManager* manager, const char* file)
{
	WavFile* out = AllocWavFile(&manager->stored);
	if (!out->Load(file))
	{
		DeallocWavFile(&manager->stored, out);
		return nullptr;
	}
	return out;
}
struct OggFile* AU_LoadOggFile(struct AudioManager* manager, const char* file)
{
	int err = 0;
	stb_vorbis* out = stb_vorbis_open_filename(file, &err, nullptr);
	if (err)
	{
		return 0;
	}
	stb_vorbis_seek_frame(out, 0);	// loads the vorbis file data into memory
	return (struct OggFile*)out;
}
void AU_FreeFile(struct AudioManager* manager, struct WavFile* file)
{
	DeallocWavFile(&manager->stored, file);
}
void AU_FreeOggFile(struct AudioManager* manager, struct OggFile* file)
{
	stb_vorbis_close((stb_vorbis*)file);
}


AudioPlaybackContext* AU_PlayAudio(struct AudioManager* manager, WavFile* file, float volume)
{
	if (!file) return nullptr;

	if (manager->currentNumPlaying >= NUM_CONCURRENT_AUDIO_STREAMS) return nullptr;
	
	AudioPlaybackContext* result = nullptr;
	for (int i = 0; i < NUM_CONCURRENT_AUDIO_STREAMS; i++)
	{
		AudioPlaybackContext* cur = &manager->active[i];
		if (!cur->inUse)
		{
			cur->file.wav = file;
			cur->remaining = cur->file.wav->GetNumSamples();
			cur->index = 0;
			cur->volume = volume;
			cur->inUse = true;
			result = cur;
			cur->isPlaying = true;
			cur->isOgg = false;
			break;
		}
	}
	manager->currentNumPlaying += 1;
	return result;
}
struct AudioPlaybackContext* AU_PlayOggAudio(struct AudioManager* manager, struct OggFile* file, float volume)
{
	if (!file) return nullptr;
	if (manager->currentNumPlaying >= NUM_CONCURRENT_AUDIO_STREAMS) return nullptr;

	AudioPlaybackContext* result = nullptr;
	for (int i = 0; i < NUM_CONCURRENT_AUDIO_STREAMS; i++)
	{
		AudioPlaybackContext* cur = &manager->active[i];
		if (!cur->inUse)
		{
			cur->file.ogg = (stb_vorbis*)file;
			cur->remaining = cur->file.ogg->total_samples;
			cur->index = 0;
			cur->volume = volume;
			cur->inUse = true;
			result = cur;
			cur->isPlaying = true;
			cur->isOgg = true;
			break;
		}
	}
	manager->currentNumPlaying += 1;
	return result;
}
struct AudioPlaybackContext* AU_PlayAudioOnRepeat(struct AudioManager* manager, WavFile* file, float volume)
{
	if (!file) return nullptr;
	if (manager->currentNumPlaying >= NUM_CONCURRENT_AUDIO_STREAMS) return nullptr;

	AudioPlaybackContext* result = nullptr;
	for (int i = 0; i < NUM_CONCURRENT_AUDIO_STREAMS; i++)
	{
		AudioPlaybackContext* cur = &manager->active[i];
		if (!cur->inUse)
		{
			cur->file.wav = file;
			cur->remaining = cur->file.wav->GetNumSamples();
			cur->index = 0;
			cur->volume = volume;
			cur->repeat = true;
			cur->inUse = true;
			result = cur;
			cur->isPlaying = true;
			cur->isOgg = false;
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
struct AudioPlaybackContext* AU_PlayOggAudioOnRepeat(struct AudioManager* manager, struct OggFile* file, float volume)
{
	if (!file) return nullptr;
	if (manager->currentNumPlaying >= NUM_CONCURRENT_AUDIO_STREAMS) return nullptr;

	AudioPlaybackContext* result = nullptr;
	for (int i = 0; i < NUM_CONCURRENT_AUDIO_STREAMS; i++)
	{
		AudioPlaybackContext* cur = &manager->active[i];
		if (!cur->inUse)
		{
			cur->file.ogg = (stb_vorbis*)file;
			cur->remaining = cur->file.ogg->total_samples;
			cur->index = 0;
			cur->volume = volume;
			cur->inUse = true;
			result = cur;
			cur->isPlaying = true;
			cur->isOgg = true;
			cur->repeat = true;
			break;
		}
	}
	manager->currentNumPlaying += 1;
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