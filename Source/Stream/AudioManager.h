#pragma once

#include "buffer.h"

class AudioPCM;

bool operator==(const AudioPCM& l, const AudioPCM& r);
uint32 GetTypeHash(const AudioPCM& obj);

class AudioPCM
{
public:
	AudioPCM(const FString& name);
	AudioPCM(const AudioPCM& other);
	AudioPCM& operator=(AudioPCM other);
	AudioPCM(AudioPCM&& other);
	~AudioPCM();
	friend void swap(AudioPCM& first, AudioPCM& second);

public:
	FString getName() const { return mName; }
	size_t getSize() const { return mSize; }
	const uint8* getBuffer() const { return mBuffer; }

private:
	FString mName;
	size_t mSize = 0;
	uint8* mBuffer = nullptr;
};

class AudioManager
{
public:
	AudioManager() = default;
	~AudioManager() = default;

	void addAudioList(const TArray<FString>& filenames);

private:
	TSet<AudioPCM> mAudioSet;

};

bool operator==(const AudioPCM& l, const AudioPCM& r)
{
	return (l.getName() == r.getName());
}

uint32 GetTypeHash(const AudioPCM& obj)
{
	return GetTypeHash(obj.getName());
}