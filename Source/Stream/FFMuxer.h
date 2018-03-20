// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "FFHeader.h"
#include "CoreMinimal.h"
#include "Runtime/Core/Public/Misc/Paths.h"
#include "Runtime/Engine/Public/UnrealClient.h"
#include <string>
#include "Runtime/Core/Public/Misc/FileHelper.h"
#include "Runtime/Core/Public/Misc/Paths.h"

enum class EFrameType
{
	Video,
	Audio
};

/**
 * 
 */
class STREAM_API FFMuxer
{
public:	
	void Initialize(int32 Width,int32 Height);
	bool IsInitialized() const;
	bool IsReadyToStream() const;
	void Mux(FViewport* Viewport);
	void Release();
private:
	// FFmpeg variables start
	AVFormatContext* OutputFormatContext = nullptr;
	AVOutputFormat* OutputFormat = nullptr;
	AVStream* OutputStream = nullptr;
	AVCodec* AudioCodec = nullptr;
	AVCodec* VideoCodec = nullptr;
	AVStream* AudioStream = nullptr;
	AVStream* VideoStream = nullptr;
	AVFrame* AudioFrame = nullptr;
	AVFrame* VideoFrame = nullptr;
	AVCodecContext* AudioCodecContext = nullptr;
	AVCodecContext* VideoCodecContext = nullptr;
	AVDictionary* Dictionary = nullptr;
	SwsContext* SamplerContext = nullptr;
	// FFmpeg variables end
private:
	// FFmpeg methods start
	void InitFFmpeg();
	bool InitOutputFormatContext();
	bool InitIOContext();
	bool InitCodecs();
	bool AllocateFrames();
	bool InitSampleScaler();
	bool OpenCodecs();
	void SetCodecParams();
	bool InitStreams();
	bool WriteHeader();
	bool WriteTrailer();
	bool WriteVideoFrame(FViewport* Viewport);
	bool WriteAudioFrame();
	bool Encode(AVFrame* Frame, EFrameType Type);
	// FFmpeg methods end
private:
	TArray<uint8> SingleFrameBuffer;
	TArray<uint8> AudioBuffer;
	FViewport * MuxViewport = nullptr;
	bool initialized = false;
	bool CanStream = false;
	static const int FPS = 30;
	int width = 0;
	int height = 0;
	const char* OUTPUT_URL = "C:/screen/test.mp4";
	FString AudioFile = "ThirdParty/audio/Ambient1.wav";
	int64_t CurrentVideoPTS = 0;
	int64_t CurrentAudioPTS = 0;
	int64_t FramesPushed = 0;
	AVRational GetRational(int num, int den);
	void PrintError(int ErrorCode);
};
