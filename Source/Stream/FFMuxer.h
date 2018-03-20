// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "FFHeader.h"
#include "CoreMinimal.h"
#include "Runtime/Core/Public/Misc/Paths.h"
#include "Runtime/Engine/Public/UnrealClient.h"
#include <string>

/**
 * 
 */
class STREAM_API FFMuxer
{
public:	
	void Initialize();
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
	AVCodecContext* AudioCodecContext = nullptr;
	AVCodecContext* VideoCodecContext = nullptr;
	AVDictionary* Dictionary = nullptr;
	// FFmpeg variables end
private:
	// FFmpeg methods start
	void InitFFmpeg();
	bool InitOutputFormatContext();
	bool InitIOContext();
	bool InitCodecs();
	bool OpenCodecs();
	void SetCodecParams();
	bool InitStreams();
	bool WriteHeader();
	bool WriteTrailer();
	// FFmpeg methods end
private:
	FViewport * MuxViewport = nullptr;
	bool initialized = false;
	bool CanStream = false;
	static const int FPS = 30;
	int width = 1280;
	int height = 720;
	const char* OUTPUT_URL = "C:/screen/test.mp4";
	AVRational GetRational(int num, int den);
	void PrintError(int ErrorCode);
};
