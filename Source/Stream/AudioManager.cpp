#include "AudioManager.h"

#include "Runtime/Core/Public/HAL/PlatformFilemanager.h"
#include "Runtime/Core/Public/Misc/FileHelper.h"
#include "Runtime/Core/Public/Misc/Paths.h"

// wav offset
static const int32 OFFSET = 44;

AudioPCM::AudioPCM(const FString& name) :
	mName(name)
{
	FString AudioFilePath = FPaths::ProjectDir() + "/ThirdParty/audio/" + name;
	IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
	IFileHandle* FileHandle = PlatformFile.OpenRead(*AudioFilePath);

	if (FileHandle)
	{
		mSize = FileHandle->Size();
		mBuffer = new uint8[mSize];
		if (!FileHandle->Seek(OFFSET))
		{
			// todo
		}
		if (!FileHandle->Read(mBuffer, sizeof(mSize)))
		{
			// todo
		}

		// Close the file again
		delete FileHandle;
	}
}

AudioPCM::AudioPCM(const AudioPCM& other) :
	mName(other.getName()),
	mSize(other.getSize()),
	mBuffer(mSize ? new uint8[mSize] : nullptr)
{
	//memcpy(mBuffer, other.getBuffer(), mSize);
	std::copy(other.getBuffer(), other.getBuffer() + mSize, mBuffer);
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
	delete[] mBuffer;
	mBuffer = nullptr;
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
		mAudioSet.Add(AudioPCM(v));
	}
}
