#pragma once

struct WavFile
{
	WavFile() { numChannels = 2; sampleRate = 44100; }
	WavFile(WavFile&& w) { numChannels = w.numChannels; sampleRate = w.sampleRate; data = w.data; numSamples = w.numSamples; w.data = nullptr; }
	~WavFile() { if (data) { delete[] data; data = nullptr; } }

	// the data is always interleaved
	void SetDataMode(int numChannels, int sampleRate);

	bool Load(const char* filename);
	bool LoadFromMemory(const void* data, int size);


	int GetNumChannels() const;
	int GetNumSamples() const;
	const float* GetData() const;

private:
	float* data = nullptr;
	int numSamples = 0;
	int numChannels = 0;
	int sampleRate = 0;
};