// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

/**
 * 
 */
class STREAM_API WaveFileManager
{
private:
	int32 SampleRate = 44100;
	int32 NumChannels = 2;
	FString SavePath = "C:/screen/audio_record.wav"; // todo:ashe23 remove hardcode later
	FString SavePathPCM = "C:/screen/audio_record.pcm";
public:
	void SetSampleRate(int32 SR);
	void SetNumChannels(int32 NC);
	void Serialize(TArray<uint8>& OutWaveFileData, const uint8* InPCMData, const int32 NumBytes);
};
