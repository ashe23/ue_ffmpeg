#include "AudioManager.h"

#include "Runtime/Core/Public/HAL/PlatformFilemanager.h"
#include "Runtime/Core/Public/Misc/FileHelper.h"
#include "Runtime/Core/Public/Misc/Paths.h"

#include "buffer.h"

// wav offset
static const int32 OFFSET = 44;

bool operator==(const AudioPCM& l, const AudioPCM& r)
{
	return (l.getName() == r.getName());
}

uint32 GetTypeHash(const AudioPCM& obj)
{
	return GetTypeHash(obj.getName());
}

AudioPCM::AudioPCM(const FString& name) :
	mName(name)
{
	FString AudioFilePath = FPaths::ProjectDir() + "/ThirdParty/audio/" + name;
	IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
	IFileHandle* FileHandle = PlatformFile.OpenRead(*AudioFilePath);

	if (FileHandle)
	{
		mSize = FileHandle->Size();
		mSize -= OFFSET;
		uint8* Buffer = new uint8[mSize];
		if (!FileHandle->Seek(OFFSET))
		{
			UE_LOG(LogTemp, Warning, TEXT("FileHandle->Seek failed"));
		}
		if (!FileHandle->Read(Buffer, mSize))
		{
			UE_LOG(LogTemp, Warning, TEXT("FileHandle->Read failed"));
		}

		mBuffer.Reserve(mSize);
		for (int i = 0 ; i < mSize; ++i)
		{
			mBuffer.Add(Buffer[i]);
		}

		// Close the file again
		delete FileHandle;
		delete[] Buffer;
	}
}

AudioPCM::AudioPCM(const AudioPCM& other) :
	mName(other.getName()),
	mSize(other.getSize()),
	mBuffer(other.getBuffer())
{
}

AudioPCM& AudioPCM::operator=(AudioPCM other)
{
	swap(*this, other);
	return *this;
}

AudioPCM::AudioPCM(AudioPCM&& other)
{
	swap(*this, other);
}

AudioPCM::~AudioPCM()
{
}

void swap(AudioPCM& first, AudioPCM& second)
{
	using std::swap;
	swap(first.mName, second.mName);
	swap(first.mSize, second.mSize);
	swap(first.mBuffer, second.mBuffer);
}

void AudioManager::addAudioList(const TArray<FString>& filenames)
{
	for (auto v : filenames)
	{
		mAudioSet.Add(v, AudioPCM(v));
	}
}

AudioPCM AudioManager::getAudio(const FString & filename) const
{
	check(mAudioSet.Contains(filename));
	return mAudioSet[filename];
}

void AudioManager::fillAudioInBuffer(const FString& filename)
{
	AudioPCM obj = this->getAudio(filename);
	TArray<uint8> buffer = obj.getBuffer();
	
}
