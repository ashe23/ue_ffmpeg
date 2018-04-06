#pragma once

#include "buffer.h"

class AudioPCM;

bool operator==(const AudioPCM& l, const AudioPCM& r);
uint32 GetTypeHash(const AudioPCM& obj);

class AudioPCM
{
public:
	AudioPCM() = default;
	AudioPCM(const FString& name);
	AudioPCM(const AudioPCM& other);
	AudioPCM& operator=(AudioPCM other);
	AudioPCM(AudioPCM&& other);
	~AudioPCM();
	friend void swap(AudioPCM& first, AudioPCM& second);

public:
	FString getName() const { return mName; }
	size_t getSize() const { return mSize; }
	TArray<uint8> getBuffer() const { return mBuffer; }

private:
	FString mName;
	size_t mSize = 0;
	TArray<uint8> mBuffer;
};

class AudioManager
{
public:
	~AudioManager() = default;
	static AudioManager& GetInstance() {
		static AudioManager instance;
		return instance;
	}

	void addAudioList(const TArray<FString>& filenames);
	AudioPCM getAudio(const FString& filename) const;
	void fillAudioInBuffer(const FString& filename);
	void Empty();

private:
	AudioManager() = default;

private:
	TMap<FString, AudioPCM> mAudioSet;

};